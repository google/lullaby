/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "redux/systems/model/gltf_asset.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "draco/attributes/geometry_attribute.h"
#include "draco/attributes/geometry_indices.h"
#include "draco/attributes/point_attribute.h"
#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"
#include "draco/core/draco_types.h"
#include "draco/mesh/mesh.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/graphics/vertex_attribute.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/transform.h"
#include "redux/modules/math/vector.h"
#include "redux/systems/model/model_asset.h"
#include "redux/systems/model/model_asset_factory.h"
#include "tinygltf/tiny_gltf.h"

namespace redux {

// A simple struct to share asset loading between FileExists and ReadWholeFile.
// See the comments for |FileExists|.
struct GltfLoadFileContext {
  using Data = std::vector<unsigned char>;

  Registry* registry = nullptr;
  absl::flat_hash_map<std::string, Data> file_data;
};

// TinyGLTF follows a successful call to FileExists with a call to
// ReadWholeFile. To avoid forcing clients to provide both functions, we bundle
// the two into this function by using the AssetLoader's LoadFileFn, then cache
// the result and data to be used in ReadWholeFile.
static bool FileExists(const std::string& filepath, void* user_data) {
  auto* context = reinterpret_cast<GltfLoadFileContext*>(user_data);
  auto* asset_loader = context->registry->Get<AssetLoader>();
  CHECK(asset_loader != nullptr)  << "No AssetLoader present.";

  auto res = asset_loader->OpenNow(filepath.c_str());
  if (!res.ok()) {
      LOG(FATAL) << "Cannot open file: " << filepath;
  }

  const size_t num_bytes = res.value().GetTotalLength();
  GltfLoadFileContext::Data& data = context->file_data[filepath];
  data.resize(num_bytes);
  res.value().Read(data.data(), num_bytes);
  return true;
}

// See comments for |FileExists|.
static bool ReadWholeFile(std::vector<unsigned char>* out, std::string* err,
                          const std::string& filepath, void* user_data) {
  auto* context = reinterpret_cast<GltfLoadFileContext*>(user_data);
  auto iter = context->file_data.find(filepath);
  if (iter != context->file_data.end()) {
    *out = std::move(iter->second);
  }
  return iter != context->file_data.end();
}

static vec4 ToVec4(absl::Span<const double> data) {
  if (data.size() == 3) {
    return vec4(data[0], data[1], data[2], 1.0f);
  } else if (data.size() == 4) {
    return vec4(data[0], data[1], data[2], data[3]);
  } else {
    LOG(FATAL) << "Unsupported data size: " << data.size();
  }
}

static VertexType GetVertexType(const tinygltf::Accessor& accessor) {
  switch (accessor.type) {
    case TINYGLTF_TYPE_SCALAR:
      switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          return VertexType::Scalar1f;
      }
      break;
    case TINYGLTF_TYPE_VEC2:
      switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          return VertexType::Vec2us;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          return VertexType::Vec2f;
      }
      break;
    case TINYGLTF_TYPE_VEC3:
      switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          return VertexType::Vec3f;
      }
      break;
    case TINYGLTF_TYPE_VEC4:
      switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
          return VertexType::Vec4ub;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          return VertexType::Vec4us;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          return VertexType::Vec4f;
      }
      break;
    default:
      break;
  }
  LOG(FATAL) << "Unsupported GLTF vertex type, type: << " << accessor.type
             << " component type: " << accessor.componentType;
}

static VertexType GetVertexType(const draco::PointAttribute& in) {
  const draco::DataType data_type = in.data_type();
  const int num_components = in.num_components();

  switch (data_type) {
    case draco::DT_INT8:
      break;
    case draco::DT_UINT8:
      switch (num_components) {
        case 4:
          return VertexType::Vec4ub;
      }
      break;
    case draco::DT_INT16:
      break;
    case draco::DT_UINT16:
      switch (num_components) {
        case 2:
          return VertexType::Vec2us;
        case 4:
          return VertexType::Vec4us;
      }
      break;
    case draco::DT_FLOAT32:
      switch (num_components) {
        case 1:
          return VertexType::Scalar1f;
        case 2:
          return VertexType::Vec2f;
        case 3:
          return VertexType::Vec3f;
        case 4:
          return VertexType::Vec4f;
      }
      break;
    default:
      break;
  }
  LOG(FATAL) << "Unsupported draco vertex type, type: << " << data_type
             << "num components: " << num_components;
}

static VertexUsage GetVertexUsage(const draco::PointAttribute& attrib) {
  const auto type = attrib.attribute_type();
  switch (type) {
    case draco::GeometryAttribute::POSITION:
      return VertexUsage::Position;
    case draco::GeometryAttribute::NORMAL:
      return VertexUsage::Normal;
    case draco::GeometryAttribute::COLOR:
      return VertexUsage::Color0;
    case draco::GeometryAttribute::TEX_COORD:
      return VertexUsage::TexCoord0;
    case draco::GeometryAttribute::GENERIC:
      // It seems like the version of Draco supported by GLTF stored tangent
      // data here.
      return VertexUsage::Tangent;
    case draco::GeometryAttribute::TANGENT:
      return VertexUsage::Tangent;
    case draco::GeometryAttribute::JOINTS:
      return VertexUsage::BoneIndices;
    case draco::GeometryAttribute::WEIGHTS:
      return VertexUsage::BoneWeights;
    default:
      break;
  }
  LOG(FATAL) << "Unknown attribute type: " << type;
  return VertexUsage::Invalid;
}

static MeshIndexType GetMeshIndexType(const tinygltf::Accessor& accessor) {
  switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      return MeshIndexType::U16;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      return MeshIndexType::U32;
    default:
      break;
  }

  LOG(FATAL) << "Unsupported component type " << accessor.componentType;
  return MeshIndexType::U16;
}

static MeshPrimitiveType GetMeshPrimitiveType(int mode) {
  switch (mode) {
    case TINYGLTF_MODE_POINTS:
      return MeshPrimitiveType::Points;
    case TINYGLTF_MODE_LINE:
      return MeshPrimitiveType::Lines;
    case TINYGLTF_MODE_TRIANGLES:
      return MeshPrimitiveType::Triangles;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
      return MeshPrimitiveType::TriangleStrip;
    case TINYGLTF_MODE_TRIANGLE_FAN:
      return MeshPrimitiveType::TriangleFan;
    default:
      break;
  }
  LOG(FATAL) << "Unsupported primitive mode " << mode;
  return MeshPrimitiveType::Points;
}

static const tinygltf::Accessor* GetAccessor(
    const std::string& name, const tinygltf::Model& model,
    const tinygltf::Primitive& primitive) {
  const auto iter = primitive.attributes.find(name);
  if (iter == primitive.attributes.cend()) {
    return nullptr;
  }
  return &model.accessors[iter->second];
}

static Box GetAccessorBounds(const tinygltf::Accessor& accessor) {
  Box bounds;
  if (accessor.minValues.size() == 3 && accessor.maxValues.size() == 3) {
    bounds.min = vec3(accessor.minValues[0], accessor.minValues[1],
                      accessor.minValues[2]);
    bounds.max = vec3(accessor.maxValues[0], accessor.maxValues[1],
                      accessor.maxValues[2]);
  }
  return bounds;
}

static Box TransformBounds(const Box& box, const mat4& transform) {
  const vec4 corners[] = {
      vec4(box.min.x, box.min.y, box.min.z, 1.0f) * transform,
      vec4(box.max.x, box.min.y, box.min.z, 1.0f) * transform,
      vec4(box.min.x, box.max.y, box.min.z, 1.0f) * transform,
      vec4(box.min.x, box.min.y, box.max.z, 1.0f) * transform,
      vec4(box.max.x, box.max.y, box.max.z, 1.0f) * transform,
      vec4(box.min.x, box.max.y, box.max.z, 1.0f) * transform,
      vec4(box.max.x, box.min.y, box.max.z, 1.0f) * transform,
      vec4(box.max.x, box.max.y, box.min.z, 1.0f) * transform,
  };
  return Box({
    vec3(corners[0]),
    vec3(corners[1]),
    vec3(corners[2]),
    vec3(corners[3]),
    vec3(corners[4]),
    vec3(corners[5]),
    vec3(corners[6]),
    vec3(corners[7]),
  });
}

static const tinygltf::Accessor* GetIndexAccessor(
    const tinygltf::Model& model, const tinygltf::Primitive& primitive) {
  return primitive.indices >= 0 ? &model.accessors[primitive.indices] : nullptr;
}

static int GetBufferIndex(const tinygltf::Model& model,
                          const tinygltf::Accessor& accessor) {
  CHECK_NE(accessor.bufferView, -1)
      << accessor.name << " accessor has invalid buffer view index.";
  return model.bufferViews[accessor.bufferView].buffer;
}

static absl::Span<const std::byte> GetBufferBytes(
    const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
  const int buffer_index = GetBufferIndex(model, accessor);
  const tinygltf::Buffer& buffer = model.buffers[buffer_index];
  const auto* data = reinterpret_cast<const std::byte*>(buffer.data.data());
  return {data, buffer.data.size()};
}

std::shared_ptr<draco::Mesh> DecodeDracoMesh(const tinygltf::Model& model,
                                             int buffer_view_index) {
  const tinygltf::BufferView& view = model.bufferViews[buffer_view_index];
  const tinygltf::Buffer& buffer = model.buffers[view.buffer];
  const size_t offset = view.byteOffset;
  const size_t length = view.byteLength;

  draco::DecoderBuffer decoder_buffer;
  decoder_buffer.Init(
      reinterpret_cast<const char*>(buffer.data.data()) + offset, length);

  draco::Decoder decoder;
  auto decoder_result = decoder.DecodeMeshFromBuffer(&decoder_buffer);
  if (!decoder_result.ok()) {
    return nullptr;
  }
  return std::shared_ptr<draco::Mesh>(std::move(decoder_result).value());
}

void GltfAsset::ProcessData() {
  std::string directory = "";
  std::string err = "";
  std::string warn = "";

  tinygltf::TinyGLTF gltf;
  gltf.SetStoreOriginalJSONForExtrasAndExtensions(true);

  // Use custom filesystem callbacks to ensure an app's custom LoadFileFn is
  // respected.
  GltfLoadFileContext context;
  context.registry = registry_;
  const tinygltf::FsCallbacks fs = {
      // Checks if the file exists using the AssetLoader's LoadFileFn and caches
      // the results for ReadWholeFile.
      &FileExists,
      // Doesn't perform any file I/O, so TinyGLTF's implementation is fine.
      &tinygltf::ExpandFilePath,
      // Returns the results from FileExists.
      &ReadWholeFile,
      // WriteWholeFile should never be called.
      nullptr,
      // A bundle containing the Registry and a data cache.
      &context};
  gltf.SetFsCallbacks(fs);

  model_ = std::make_shared<tinygltf::Model>();

  bool is_binary = false;
  if (asset_data_->GetNumBytes() > 4 &&
      memcmp(asset_data_->GetBytes(), "glTF", 4) == 0) {
    is_binary = true;
  }
  if (is_binary) {
    if (!gltf.LoadBinaryFromMemory(
            model_.get(), &err, &warn,
            reinterpret_cast<const unsigned char*>(asset_data_->GetBytes()),
            asset_data_->GetNumBytes(), directory)) {
      LOG(FATAL) << "GLB parsing failure: " << err << " " << warn;
    }
  } else {
    if (!gltf.LoadASCIIFromString(
            model_.get(), &err, &warn,
            reinterpret_cast<const char*>(asset_data_->GetBytes()),
            asset_data_->GetNumBytes(), directory)) {
      LOG(FATAL) << "GLTF parsing failure: " << err << " " << warn;
    }
  }

  if (!warn.empty()) {
    LOG(WARNING) << "GLTF parsing warnings: " << warn;
  }

  PrepareMeshes();
  PrepareTextures();
  PrepareMaterials();
}

void GltfAsset::AppendAttribute(VertexFormat& vertex_format,
                                const tinygltf::Accessor& accessor,
                                VertexUsage usage, VertexType valid_types) {
  VertexAttribute attrib(usage, GetVertexType(accessor));
  CHECK(attrib.type == valid_types)
      << ToString(usage) << " must be " << ToString(valid_types);

  const tinygltf::BufferView& view = model_->bufferViews[accessor.bufferView];
  const std::size_t offset = view.byteOffset + accessor.byteOffset;
  std::size_t byte_stride = view.byteStride;
  if (byte_stride == 0) {
    byte_stride = VertexFormat::GetVertexTypeSize(attrib.type);
  }
  vertex_format.AppendAttribute(attrib, offset, byte_stride);
}

static mat4 GetNodeTransform(const tinygltf::Node& node) {
  if (!node.matrix.empty()) {
    const double* arr = node.matrix.data();
    return mat4(arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7],
                arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14],
                arr[15]);
  } else {
    Transform transform;
    if (!node.translation.empty()) {
      transform.translation.x = node.translation[0];
      transform.translation.y = node.translation[1];
      transform.translation.z = node.translation[2];
    }
    if (!node.rotation.empty()) {
      transform.rotation.x = node.rotation[0];
      transform.rotation.y = node.rotation[1];
      transform.rotation.z = node.rotation[2];
      transform.rotation.w = node.rotation[3];
    }
    if (!node.scale.empty()) {
      transform.scale.x = node.scale[0];
      transform.scale.y = node.scale[1];
      transform.scale.z = node.scale[2];
    }
    return TransformMatrix(transform);
  }
}

void GltfAsset::ProcessNodeRecursive(mat4 transform, int node_index) {
  const tinygltf::Node& node = model_->nodes[node_index];

  transform *= GetNodeTransform(node);

  const int mesh_id = node.mesh;
  if (mesh_id != -1) {
    const tinygltf::Mesh& mesh = model_->meshes[mesh_id];

    int index = 0;
    for (const tinygltf::Primitive& primitive : mesh.primitives) {
      MeshPrimitiveData part;
      part.transform = transform;
      part.material_index = primitive.material;
      mesh_primitives_.push_back(part);

      auto mesh_data = std::make_shared<MeshData>();
      mesh_data->SetName(
          Hash(absl::StrCat(node.name, node_index, mesh_id, index)));
      ++index;

      DracoBuffer draco_buffer;
      UpdateDracoMeshData(mesh_data, draco_buffer, primitive, transform);
      if (draco_buffer.mesh == nullptr) {
        UpdateMeshData(mesh_data, primitive, transform);
      } else {
        draco_buffers_.push_back(std::move(draco_buffer));
      }

      meshes_.push_back(std::move(mesh_data));
    }
  }

  for (const int& child : node.children) {
    ProcessNodeRecursive(transform, child);
  }
}

void GltfAsset::PrepareMeshes() {
  CHECK_EQ(model_->scenes.size(), 1);
  const tinygltf::Scene& scene = model_->scenes[0];

  for (const int n : scene.nodes) {
    ProcessNodeRecursive(mat4::Identity(), n);
  }
}

void GltfAsset::UpdateMeshData(std::shared_ptr<MeshData> mesh_data,
                               const tinygltf::Primitive& primitive,
                               const mat4& transform) {
  auto positions = GetAccessor("POSITION", *model_, primitive);
  auto normals = GetAccessor("NORMAL", *model_, primitive);
  auto tangents = GetAccessor("TANGENT", *model_, primitive);
  auto uv0s = GetAccessor("TEXCOORD_0", *model_, primitive);
  auto uv1s = GetAccessor("TEXCOORD_1", *model_, primitive);
  auto joints0 = GetAccessor("JOINTS_0", *model_, primitive);
  auto weights0 = GetAccessor("WEIGHTS_0", *model_, primitive);
  auto index_accessor = GetIndexAccessor(*model_, primitive);

  CHECK(positions != nullptr) << "Must have POSITION attribute.";
  const int position_index = GetBufferIndex(*model_, *positions);

  // Build the vertex format from each valid accessor.
  VertexFormat vertex_format;

  AppendAttribute(vertex_format, *positions, VertexUsage::Position,
                  VertexType::Vec3f);
  if (normals) {
    CHECK_EQ(position_index, GetBufferIndex(*model_, *normals));
    AppendAttribute(vertex_format, *normals, VertexUsage::Normal,
                    VertexType::Vec3f);
  }
  if (tangents) {
    CHECK_EQ(position_index, GetBufferIndex(*model_, *tangents));
    AppendAttribute(vertex_format, *tangents, VertexUsage::Tangent,
                    VertexType::Vec4f);
  }
  if (uv0s) {
    CHECK_EQ(position_index, GetBufferIndex(*model_, *uv0s));
    AppendAttribute(vertex_format, *uv0s, VertexUsage::TexCoord0,
                    VertexType::Vec2f);
  }
  if (uv1s) {
    CHECK_EQ(position_index, GetBufferIndex(*model_, *uv1s));
    AppendAttribute(vertex_format, *uv1s, VertexUsage::TexCoord1,
                    VertexType::Vec2f);
  }
  if (joints0) {
    CHECK_EQ(position_index, GetBufferIndex(*model_, *joints0));
    if (joints0->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      AppendAttribute(vertex_format, *joints0, VertexUsage::BoneIndices,
                      VertexType::Vec4us);
    } else {
      AppendAttribute(vertex_format, *joints0, VertexUsage::BoneIndices,
                      VertexType::Vec4ub);
    }
  }
  if (weights0) {
    CHECK_EQ(position_index, GetBufferIndex(*model_, *weights0));
    AppendAttribute(vertex_format, *weights0, VertexUsage::BoneWeights,
                    VertexType::Vec4f);
  }

  // We've checked that all vertex attributes are pointing to the same buffer,
  // so we'll just use the data from the positions buffer.
  const ByteSpan vertex_data = GetBufferBytes(*model_, *positions);
  DataContainer vertices =
      DataContainer::WrapDataInSharedPtr(vertex_data, model_);
  Box bounds = GetAccessorBounds(*positions);
  mesh_data->SetVertexData(vertex_format, std::move(vertices), positions->count,
                           TransformBounds(bounds, transform));

  if (index_accessor) {
    const MeshIndexType index_format = GetMeshIndexType(*index_accessor);
    const MeshPrimitiveType primitive_type =
        GetMeshPrimitiveType(primitive.mode);

    // Indices are assumed to start at byte 0 (unlike vertex attributes (which
    // each have their own individual offsets). So, we need to find the
    // "subspan" within the buffer that stores the indices.
    const std::size_t offset =
        model_->bufferViews[index_accessor->bufferView].byteOffset +
        index_accessor->byteOffset;
    const std::size_t num_bytes =
        index_accessor->count * GetMeshIndexTypeSize(index_format);

    ByteSpan index_data = GetBufferBytes(*model_, *index_accessor);
    ByteSpan sub_index_data =
        absl::MakeSpan(index_data.data() + offset, num_bytes);

    DataContainer indices =
        DataContainer::WrapDataInSharedPtr(sub_index_data, model_);
    mesh_data->SetIndexData(index_format, primitive_type, std::move(indices),
                            index_accessor->count);
  }
}

void GltfAsset::UpdateDracoMeshData(std::shared_ptr<MeshData> mesh_data,
                                    DracoBuffer& draco_buffer,
                                    const tinygltf::Primitive& primitive,
                                    const mat4& transform) {
  auto extension = primitive.extensions.find("KHR_draco_mesh_compression");
  if (extension == primitive.extensions.end()) {
    return;
  }

  auto buffer_view_value = extension->second.Get("bufferView");
  CHECK(buffer_view_value.IsInt());
  const int buffer_view_index = buffer_view_value.Get<int>();
  draco_buffer.mesh = DecodeDracoMesh(*model_, buffer_view_index);
  CHECK(draco_buffer.mesh != nullptr);

  auto attributes_value = extension->second.Get("attributes");
  CHECK(attributes_value.IsObject());
  auto attrib_ids = attributes_value.Get<tinygltf::Value::Object>();

  std::vector<const draco::PointAttribute*> draco_attribs;
  draco_attribs.reserve(attrib_ids.size());

  VertexFormat vertex_format;
  std::size_t offset = 0;
  for (const auto& iter : attrib_ids) {
    CHECK(iter.second.IsInt());
    const int attrib_id = iter.second.Get<int>();
    const draco::PointAttribute* attrib =
        draco_buffer.mesh->GetAttributeByUniqueId(attrib_id);
    CHECK(attrib != nullptr);
    draco_attribs.push_back(attrib);

    const VertexType vertex_type = GetVertexType(*attrib);
    const VertexUsage usage = GetVertexUsage(*attrib);
    const VertexAttribute vertex_attrib(usage, vertex_type);
    const auto stride = VertexFormat::GetVertexTypeSize(vertex_attrib.type);
    vertex_format.AppendAttribute(vertex_attrib, offset, stride);
    offset += (draco_buffer.mesh->num_points() * stride);
  }

  auto positions = GetAccessor("POSITION", *model_, primitive);
  const auto num_vertices = draco_buffer.mesh->num_points();

  // Draco stores each attribute in its own buffer, but we need to have a single
  // contiguous buffer that contains all the data, so we need to copy it all
  // over.
  DataBuilder vertex_builder(num_vertices * vertex_format.GetVertexSize());
  for (const draco::PointAttribute* attrib : draco_attribs) {
    const size_t stride =
        VertexFormat::GetVertexTypeSize(GetVertexType(*attrib));

    if (attrib->is_mapping_identity()) {
      // Copy the entire draco buffer onto the end of our vertex buffer.
      auto buffer = attrib->buffer();
      auto dst = vertex_builder.GetAppendPtr(buffer->data_size());
      CHECK_EQ(draco_buffer.mesh->num_points() * stride, buffer->data_size());
      memcpy(dst, buffer->data(), buffer->data_size());
    } else {
      // Copy points one-by-one.
      for (draco::PointIndex i(0); i < draco_buffer.mesh->num_points(); ++i) {
        std::byte* dst = vertex_builder.GetAppendPtr(stride);
        const uint8_t* src = attrib->GetAddressOfMappedIndex(i);
        memcpy(dst, src, stride);
      }
    }
  }
  draco_buffer.vertex_buffer = vertex_builder.Release();

  ByteSpan vertex_data = draco_buffer.vertex_buffer.GetByteSpan();
  DataContainer vertices =
      DataContainer::WrapDataInSharedPtr(vertex_data, model_);
  Box bounds = GetAccessorBounds(*positions);
  mesh_data->SetVertexData(vertex_format, std::move(vertices), num_vertices,
                           TransformBounds(bounds, transform));

  if (draco_buffer.mesh->num_faces() > 0) {
    const auto num_indices = draco_buffer.mesh->num_faces() * 3;
    draco_buffer.index_buffer = DataContainer::WrapDataInSharedPtr(
        absl::MakeSpan(&draco_buffer.mesh->face(draco::FaceIndex(0)),
                       draco_buffer.mesh->num_faces()),
        draco_buffer.mesh);

    ByteSpan index_data = draco_buffer.index_buffer.GetByteSpan();
    DataContainer indices =
        DataContainer::WrapDataInSharedPtr(index_data, model_);

    // Draco meshes are always triangles with uint32_t indices.
    mesh_data->SetIndexData(MeshIndexType::U32, MeshPrimitiveType::Triangles,
                            std::move(indices), num_indices);
  }
}

static std::string GetTextureNameFromIndex(int index) {
  return absl::StrCat("texture", index);
}

void GltfAsset::PrepareTextures() {
  for (int index = 0; index < model_->textures.size(); ++index) {
    const tinygltf::Texture& gltf_texture = model_->textures[index];
    const tinygltf::Image& gltf_image = model_->images[gltf_texture.source];

    const HashValue key = Hash(GetTextureNameFromIndex(index));
    TextureData& data = textures_[key];

    if (gltf_image.uri.empty()) {
      const vec2i size(gltf_image.width, gltf_image.height);
      const ImageFormat format = ImageFormat::Rgba8888;
      DataBuilder builder(gltf_image.image.size());
      builder.Append(gltf_image.image.data(), gltf_image.image.size());
      data.image = std::make_shared<ImageData>(format, size, builder.Release());
    } else {
      data.uri = gltf_image.uri;
    }
  }
}

static const tinygltf::Parameter* GetMaterialParam(
    const tinygltf::Material& material, const std::string& key) {
  auto iter = material.values.find(key);
  if (iter != material.values.end()) {
    return &iter->second;
  }

  auto iter2 = material.additionalValues.find(key);
  if (iter2 != material.additionalValues.end()) {
    return &iter2->second;
  }

  return nullptr;
}

void GltfAsset::UpdateMaterialData(MaterialData& data,
                                   const tinygltf::Material& gltf_material) {
  data.shading_model = "metallic_roughness";

  const auto& base_color_texture =
      gltf_material.pbrMetallicRoughness.baseColorTexture;
  const auto& metallic_roughness_texture =
      gltf_material.pbrMetallicRoughness.metallicRoughnessTexture;
  const auto& occlusion_texture = gltf_material.occlusionTexture;
  const auto& normal_texture = gltf_material.normalTexture;
  const auto& emissive_texture = gltf_material.emissiveTexture;

  CHECK_EQ(gltf_material.alphaCutoff, 0.5);

  if (gltf_material.doubleSided) {
    data.properties[ConstHash("DoubleSided")] = true;
  }

  if (gltf_material.alphaMode == "OPAQUE") {
    // Default mode, ignore.
  } else if (gltf_material.alphaMode == "BLEND") {
    data.properties[ConstHash("Transparent")] = true;
  } else {
    LOG(FATAL) << "Unsupported alphaMode: " << gltf_material.alphaMode;
  }

  data.properties[ConstHash("BaseColor")] =
      ToVec4(gltf_material.pbrMetallicRoughness.baseColorFactor);
  data.properties[ConstHash("Metallic")] =
      static_cast<float>(gltf_material.pbrMetallicRoughness.metallicFactor);
  data.properties[ConstHash("Roughness")] =
      static_cast<float>(gltf_material.pbrMetallicRoughness.roughnessFactor);
  data.properties[ConstHash("AmbientOcclusionStrength")] =
      static_cast<float>(gltf_material.occlusionTexture.strength);
  data.properties[ConstHash("NormalScale")] =
      static_cast<float>(gltf_material.normalTexture.scale);
  data.properties[ConstHash("Emissive")] = ToVec4(gltf_material.emissiveFactor);

  if (base_color_texture.index != -1) {
    CHECK_EQ(base_color_texture.texCoord, 0);
    const std::string name = GetTextureNameFromIndex(base_color_texture.index);
    const TextureUsage usage({MaterialTextureType::BaseColor});
    data.textures.push_back({usage, name});
    textures_[Hash(name)].params.color_space = ColorSpace::sRGB;
  }

  if (normal_texture.index != -1) {
    CHECK_EQ(normal_texture.texCoord, 0);
    const std::string name = GetTextureNameFromIndex(normal_texture.index);
    const TextureUsage usage({MaterialTextureType::Normal});
    data.textures.push_back({usage, name});
  }

  if (emissive_texture.index != -1) {
    CHECK_EQ(emissive_texture.texCoord, 0);
    const std::string name = GetTextureNameFromIndex(emissive_texture.index);
    const TextureUsage usage({MaterialTextureType::Emissive});
    data.textures.push_back({usage, name});
    textures_[Hash(name)].params.color_space = ColorSpace::sRGB;
  }

  if (metallic_roughness_texture.index == occlusion_texture.index &&
      metallic_roughness_texture.index != -1) {
    const std::string name = GetTextureNameFromIndex(occlusion_texture.index);
    const TextureUsage usage({MaterialTextureType::Occlusion,
                              MaterialTextureType::Roughness,
                              MaterialTextureType::Metallic});
    data.textures.push_back({usage, name});
  } else {
    if (metallic_roughness_texture.index != -1) {
      CHECK_EQ(metallic_roughness_texture.texCoord, 0);
      const std::string name =
          GetTextureNameFromIndex(metallic_roughness_texture.index);
      const TextureUsage usage({MaterialTextureType::Unspecified,
                                MaterialTextureType::Roughness,
                                MaterialTextureType::Metallic});
      data.textures.push_back({usage, name});
    }
    if (occlusion_texture.index != -1) {
      CHECK_EQ(occlusion_texture.texCoord, 0);
      const std::string name = GetTextureNameFromIndex(occlusion_texture.index);
      const TextureUsage usage({MaterialTextureType::Occlusion});
      data.textures.push_back({usage, name});
    }
  }
}

void GltfAsset::PrepareMaterials() {
  std::vector<MaterialData> materials(model_->materials.size());
  for (size_t i = 0; i < model_->materials.size(); ++i) {
    UpdateMaterialData(materials[i], model_->materials[i]);
  }
  materials_.clear();
  materials_.reserve(mesh_primitives_.size());
  for (const MeshPrimitiveData& part : mesh_primitives_) {
    if (part.material_index == -1) {
      materials_.emplace_back();
    } else {
      CHECK_LT(part.material_index, materials.size());
      materials_.push_back(materials[part.material_index]);
    }
    materials_.back().properties[ConstHash("BaseTransform")] = part.transform;
  }
}
}  // namespace redux

REDUX_REGISTER_MODEL_ASSET(gltf, redux::GltfAsset);
REDUX_REGISTER_MODEL_ASSET(glb, redux::GltfAsset);
