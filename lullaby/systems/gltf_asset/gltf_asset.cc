/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/gltf_asset/gltf_asset.h"

#include "lullaby/generated/flatbuffers/vertex_attribute_def_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/render/tangent_generation.h"
#include "lullaby/modules/tinygltf/tinygltf_util.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "mathfu/constants.h"

namespace lull {

namespace {

// A simple struct to share asset loading between FileExists and ReadWholeFile.
// See the comments for |FileExists|.
struct LoadFileContext {
  Registry* registry = nullptr;
  bool success = false;
  std::vector<unsigned char> data;
};

// TinyGLTF follows a successful call to FileExists with a call to
// ReadWholeFile. To avoid forcing clients to provide both functions, we bundle
// the two into this function by using the AssetLoader's LoadFileFn, then cache
// the result and data to be used in ReadWholeFile.
bool FileExists(const std::string& filepath, void* user_data) {
  LoadFileContext* context = reinterpret_cast<LoadFileContext*>(user_data);
  auto* asset_loader = context->registry->Get<AssetLoader>();
  if (asset_loader) {
    auto load_file_fn = asset_loader->GetLoadFunction();
    std::string data;
    const bool res = load_file_fn(filepath.c_str(), &data);
    if (res) {
      const size_t num_bytes = data.length();
      context->data.resize(num_bytes);
      memcpy(context->data.data(), data.data(), num_bytes);
      context->success = true;
      return true;
    }
  }
  LOG(DFATAL) << "No AssetLoader present.";
  return false;
}

// See comments for |FileExists|.
bool ReadWholeFile(std::vector<unsigned char>* out, std::string* err,
                   const std::string& filepath, void* user_data) {
  LoadFileContext* context = reinterpret_cast<LoadFileContext*>(user_data);
  if (context->success) {
    *out = std::move(context->data);
  }
  return context->success;
}

mathfu::vec3 NodeTranslation(const tinygltf::Node& node) {
  if (node.translation.empty()) {
    return mathfu::kZeros3f;
  }
  return mathfu::vec3(static_cast<float>(node.translation[0]),
                      static_cast<float>(node.translation[1]),
                      static_cast<float>(node.translation[2]));
}

mathfu::quat NodeRotation(const tinygltf::Node& node) {
  if (node.rotation.empty()) {
    return mathfu::quat::identity;
  }
  // GLTF stores quaternions XYZW, mathfu quaternions are WXYZ.
  return mathfu::quat(static_cast<float>(node.rotation[3]),
                      static_cast<float>(node.rotation[0]),
                      static_cast<float>(node.rotation[1]),
                      static_cast<float>(node.rotation[2]));
}

mathfu::vec3 NodeScale(const tinygltf::Node& node) {
  if (node.scale.empty()) {
    return mathfu::kOnes3f;
  }
  return mathfu::vec3(static_cast<float>(node.scale[0]),
                      static_cast<float>(node.scale[1]),
                      static_cast<float>(node.scale[2]));
}

mathfu::mat4 NodeMatrix(const tinygltf::Node& node) {
  if (node.matrix.empty()) {
    return mathfu::mat4::Identity();
  }
  return mathfu::mat4(
      static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]),
      static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
      static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]),
      static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
      static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]),
      static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
      static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]),
      static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15]));
}

Sqt NodeSqt(const tinygltf::Node& node) {
  if (!node.matrix.empty()) {
    return CalculateSqtFromMatrix(NodeMatrix(node));
  }
  return Sqt(NodeTranslation(node), NodeRotation(node), NodeScale(node));
}

float FactorToFloat(const tinygltf::Parameter& parameter) {
  return static_cast<float>(parameter.Factor());
}

mathfu::vec4 ColorFactorToVec4(const tinygltf::Parameter& parameter) {
  const tinygltf::ColorValue color_value = parameter.ColorFactor();
  return mathfu::vec4(
      static_cast<float>(color_value[0]), static_cast<float>(color_value[1]),
      static_cast<float>(color_value[2]), static_cast<float>(color_value[3]));
}

VertexAttribute CreateVertexAttribute(const tinygltf::Accessor& accessor,
                                      VertexAttributeUsage usage) {
  if (usage == VertexAttributeUsage_Orientation) {
    return VertexAttribute(usage, VertexAttributeType_Vec4f);
  }
  switch (accessor.type) {
    case TINYGLTF_TYPE_SCALAR:
      if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return VertexAttribute(usage, VertexAttributeType_Scalar1f);
      }
      break;
    case TINYGLTF_TYPE_VEC2:
      if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        return VertexAttribute(usage, VertexAttributeType_Vec2us);
      } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return VertexAttribute(usage, VertexAttributeType_Vec2f);
      }
      break;
    case TINYGLTF_TYPE_VEC3:
      if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return VertexAttribute(usage, VertexAttributeType_Vec3f);
      }
      break;
    case TINYGLTF_TYPE_VEC4:
      if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        return VertexAttribute(usage, VertexAttributeType_Vec4ub);
      } else if (accessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        return VertexAttribute(usage, VertexAttributeType_Vec4us);
      } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        return VertexAttribute(usage, VertexAttributeType_Vec4f);
      }
      break;
    default:
      break;
  }
  LOG(DFATAL) << "Unsupported vertex attribute type.";
  return VertexAttribute(usage, VertexAttributeType_Empty);
}

MeshData::IndexType IndexTypeForComponentType(int component_type) {
  switch (component_type) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      return MeshData::kIndexU16;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      return MeshData::kIndexU32;
    default:
      LOG(DFATAL) << "Unsupported index type for component type "
                  << component_type;
      return MeshData::kIndexU16;
  }
}

// Gets an accessor for a specific vertex attribute |name| if it is in
// |attr_map|, which references model. Returns nullptr if the name isn't present
// in the map OR if the accessor is present, but has the wrong type, component
// type, or value count. If |expected_count| is 0, it will be ignored.
const tinygltf::Accessor* GetAndVerifyAttributeAccessor(
    const std::map<std::string, int>& attr_map, const tinygltf::Model& model,
    string_view name, const std::unordered_set<int>& valid_types,
    const std::unordered_set<int>& valid_component_types,
    size_t expected_count) {
  const auto iter = attr_map.find(name.c_str());
  if (iter == attr_map.cend()) {
    return nullptr;
  }

  const tinygltf::Accessor& accessor = model.accessors[iter->second];
  if (valid_types.find(accessor.type) == valid_types.cend()) {
    LOG(DFATAL) << name << " accessor has an invalid type: " << accessor.type;
    return nullptr;
  }
  if (valid_component_types.find(accessor.componentType) ==
      valid_types.cend()) {
    LOG(DFATAL) << name << " accessor has an invalid component type: "
                << accessor.componentType;
    return nullptr;
  }
  if (expected_count != 0 && accessor.count != expected_count) {
    LOG(DFATAL) << name << " accessor does not have the correct count: "
                << "expected " << expected_count << ", got " << accessor.count;
    return nullptr;
  }
  return &accessor;
}

// Encapsulates GLTF vertex attribute data from a model.
struct VertexAttributeInfo {
  // A pointer to the vertex attribute bytes.
  const uint8_t* data = nullptr;
  // The number of vertices referenced by the data.
  size_t count = 0;
  // The number of bytes between consecutive vertices.
  size_t byte_stride = 0;
  // The byte offset into the associated VertexFormat.
  size_t offset = 0;
};

// Creates a vertex attribute for by searching for |name| in |attr_map| and
// validating it with the data in |model|. If found, the attribute will be added
// to |vertex_format| with a specified |usage|. The remaining arguments are for
// validation purposes only.
Optional<VertexAttributeInfo> VerifyAndCreateVertexAttribute(
    string_view name, const std::map<std::string, int>& attr_map,
    const tinygltf::Model& model, VertexFormat* vertex_format,
    VertexAttributeUsage usage, const std::unordered_set<int>& valid_types,
    const std::unordered_set<int>& valid_component_types,
    size_t expected_count) {
  const tinygltf::Accessor* accessor =
      GetAndVerifyAttributeAccessor(attr_map, model, name, valid_types,
                                    valid_component_types, expected_count);
  if (accessor == nullptr) {
    return NullOpt;
  }
  const uint8_t* data = DataFromGltfAccessor<uint8_t>(model, *accessor);
  if (data == nullptr) {
    LOG(DFATAL) << "Failed to get vertex attribute data for " << name << ".";
    return NullOpt;
  }

  VertexAttributeInfo info;
  info.data = data;
  info.count = accessor->count;
  info.byte_stride = ByteStrideFromGltfAccessor(model, *accessor);

  VertexAttribute attribute = CreateVertexAttribute(*accessor, usage);
  vertex_format->AppendAttribute(attribute);
  info.offset = vertex_format->GetAttributeOffsetAt(
      vertex_format->GetNumAttributes() - 1);
  return info;
}

}  // namespace

GltfAsset::GltfAsset(Registry* registry, bool preserve_normal_tangent,
                     std::function<void()> finalize_callback)
    : registry_(registry),
      preserve_normal_tangent_(preserve_normal_tangent),
      finalize_callback_(std::move(finalize_callback)) {}

void GltfAsset::OnLoad(const std::string& filename, std::string* data) {
  id_ = Hash(filename);

  const std::string directory = GetDirectoryFromFilename(filename);
  const unsigned int num_bytes = static_cast<unsigned int>(data->length());
  const unsigned char* bytes =
      reinterpret_cast<const unsigned char*>(data->c_str());

  tinygltf::TinyGLTF gltf;

  // Use custom filesystem callbacks to ensure an app's custom LoadFileFn is
  // respected.
  LoadFileContext context;
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

  std::string err;
  std::string warn;
  // Don't store the tinygltf representation of the asset; just store the fully
  // parsed representations.
  tinygltf::Model model;

  if (EndsWith(filename, ".glb")) {
    if (!gltf.LoadBinaryFromMemory(&model, &err, &warn, bytes, num_bytes,
                                   directory)) {
      LOG(DFATAL) << "GLB parsing failure: " << err << " " << warn;
      return;
    }
  } else if (EndsWith(filename, ".gltf")) {
    if (!gltf.LoadASCIIFromString(&model, &err, &warn, data->c_str(), num_bytes,
                                  directory)) {
      LOG(DFATAL) << "GLTF parsing failure: " << err << " " << warn;
      return;
    }
  } else {
    LOG(DFATAL) << "GLTFs must end with .gltf or .glb";
    return;
  }

  if (!warn.empty()) {
    LOG(WARNING) << "GLTF parsing warnings: " << warn;
  }

  // Prepare data one type at a time. Order doesn't matter since all references
  // between data types are by index.
  PrepareNodes(model);
  PrepareMeshes(model);
  PrepareSkins(model);
  PrepareAnimations(model);
  PrepareTextures(model, directory);
  PrepareMaterials(model);
}

void GltfAsset::PrepareNodes(const tinygltf::Model& model) {
  // For each Node, create a NodeInfo that will result in an Entity.
  node_infos_.resize(model.nodes.size());
  for (size_t i = 0; i < model.nodes.size(); ++i) {
    const tinygltf::Node& gltf_node = model.nodes[i];
    NodeInfo& node_info = node_infos_[i];
    node_info.name = gltf_node.name;
    node_info.transform = NodeSqt(gltf_node);
    node_info.children = gltf_node.children;
    node_info.mesh = gltf_node.mesh;
    node_info.skin = gltf_node.skin;
    node_info.blend_shape_weights.assign(gltf_node.weights.cbegin(),
                                         gltf_node.weights.cend());
  }

  // Record which NodeInfos are root nodes and should be children of the Entity
  // the asset is created on. Use the default scene's nodes.
  root_nodes_ = model.scenes[std::max(model.defaultScene, 0)].nodes;
}

void GltfAsset::PrepareMeshes(const tinygltf::Model& model) {
  mesh_infos_.resize(model.meshes.size());
  for (size_t i = 0; i < model.meshes.size(); ++i) {
    PrepareMesh(&mesh_infos_[i], model.meshes[i], model);
  }
}

void GltfAsset::PrepareMesh(MeshInfo* mesh_info,
                            const tinygltf::Mesh& gltf_mesh,
                            const tinygltf::Model& model) {
  // TODO: support multiple primitives.
  if (gltf_mesh.primitives.size() > 1) {
    LOG(DFATAL) << "System currently only supports meshes with one primitive.";
    return;
  }

  // TODO.
  // For each primitive in the Mesh, create a MeshData.
  for (size_t prim = 0; prim < gltf_mesh.primitives.size(); ++prim) {
    // Create a MeshData for each primitive and store the material it uses by
    // index.
    const tinygltf::Primitive& gltf_primitive = gltf_mesh.primitives[prim];
    mesh_info->mesh_data = PreparePrimitive(gltf_primitive, model);
    mesh_info->material_index = gltf_primitive.material;

    // Create blend shapes if Morph Target data exists.
    if (!gltf_primitive.targets.empty()) {
      PrepareBlendShapes(mesh_info, gltf_primitive, model);
      if (mesh_info->HasBlendShapes()) {
        mesh_info->blend_shape_weights.assign(gltf_mesh.weights.cbegin(),
                                              gltf_mesh.weights.cend());
      }
    }
  }
}

MeshData GltfAsset::PreparePrimitive(const tinygltf::Primitive& gltf_primitive,
                                     const tinygltf::Model& model) {
  // Fetch pointers to all the supported attributes.
  const uint8_t* positions = nullptr;
  const uint8_t* normals = nullptr;
  const uint8_t* tangents = nullptr;
  const uint8_t* uvs_0 = nullptr;
  const uint8_t* bone_indices = nullptr;
  const uint8_t* bone_weights = nullptr;

  // Store strides to navigate each of the attributes.
  size_t positions_stride = 0;
  size_t normals_stride = 0;
  size_t tangents_stride = 0;
  size_t uvs_0_stride = 0;
  size_t bone_indices_stride = 0;
  size_t bone_weights_stride = 0;

  // Store offests for each attribute within a vertex.
  size_t positions_offset = 0;
  size_t uvs_0_offset = 0;
  size_t bone_indices_offset = 0;
  size_t bone_weights_offset = 0;

  // Either normals and tangents are used OR orientations are used depending
  // on |preserve_normal_tangent_|.
  size_t normals_offset = 0;
  size_t tangents_offset = 0;
  size_t orientations_offset = 0;

  // Create a vertex format for the mesh.
  VertexFormat vertex_format;
  size_t num_vertices = 0;

  // According to the spec, the position attribute is required unless an
  // extension specifies them. Since we currently support no extensions,
  // exit if there are no positions. They must be Vec3f.
  const auto positions_info = VerifyAndCreateVertexAttribute(
      "POSITION", gltf_primitive.attributes, model, &vertex_format,
      VertexAttributeUsage_Position, {TINYGLTF_TYPE_VEC3},
      {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
  if (!positions_info) {
    LOG(DFATAL) << "The POSITION attribute is required for primitives.";
    return MeshData();
  } else {
    positions = positions_info->data;
    positions_stride = positions_info->byte_stride;
    positions_offset = positions_info->offset;

    // Use the position accessor to determine the number of vertices, then
    // verify that all accessors have the same count.
    num_vertices = positions_info->count;
  }

  const VertexAttributeUsage normals_usage =
      preserve_normal_tangent_ ? VertexAttributeUsage_Normal
                               : VertexAttributeUsage_Orientation;

  // According to the spec, the normal attribute must be a Vec3f.
  const auto normals_info = VerifyAndCreateVertexAttribute(
      "NORMAL", gltf_primitive.attributes, model, &vertex_format, normals_usage,
      {TINYGLTF_TYPE_VEC3}, {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
  if (normals_info) {
    normals = normals_info->data;
    normals_stride = normals_info->byte_stride;
    if (preserve_normal_tangent_) {
      normals_offset = normals_info->offset;
    } else {
      orientations_offset = normals_info->offset;
    }
  } else {
    // TODO: compute flat normals.
  }

  // According to the spec, the tangent attribute must be a Vec4f.
  if (preserve_normal_tangent_) {
    const auto tangents_info = VerifyAndCreateVertexAttribute(
        "TANGENT", gltf_primitive.attributes, model, &vertex_format,
        VertexAttributeUsage_Tangent, {TINYGLTF_TYPE_VEC4},
        {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
    if (tangents_info) {
      tangents = tangents_info->data;
      tangents_stride = tangents_info->byte_stride;
      tangents_offset = tangents_info->offset;
    }
  } else {
    const auto* tangents_accessor = GetAndVerifyAttributeAccessor(
        gltf_primitive.attributes, model, "TANGENT", {TINYGLTF_TYPE_VEC4},
        {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
    if (tangents_accessor != nullptr) {
      // Tangents have no attribute associated with them: they are packed into
      // the Orientation attribute.
      tangents = DataFromGltfAccessor<uint8_t>(model, *tangents_accessor);
      tangents_stride = ByteStrideFromGltfAccessor(model, *tangents_accessor);
      if (tangents == nullptr) {
        LOG(DFATAL) << "Failed to fetch primitive TANGENT attribute data.";
      }
    }
  }

  // TODO: support Vec2ub and Vec2us.
  // For now, we only support Vec2f.
  const auto uvs_0_info = VerifyAndCreateVertexAttribute(
      "TEXCOORD_0", gltf_primitive.attributes, model, &vertex_format,
      VertexAttributeUsage_TexCoord, {TINYGLTF_TYPE_VEC2},
      {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
  if (uvs_0_info) {
    uvs_0 = uvs_0_info->data;
    uvs_0_stride = uvs_0_info->byte_stride;
    uvs_0_offset = uvs_0_info->offset;
  }

  // TODO: add support for Vec4us.
  // For now, we only support Vec4ub.
  const auto joints_info = VerifyAndCreateVertexAttribute(
      "JOINTS_0", gltf_primitive.attributes, model, &vertex_format,
      VertexAttributeUsage_BoneIndices, {TINYGLTF_TYPE_VEC4},
      {TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE}, num_vertices);
  if (joints_info) {
    bone_indices = joints_info->data;
    bone_indices_stride = joints_info->byte_stride;
    bone_indices_offset = joints_info->offset;
  }

  // TODO: add support for Vec4ub and Vec4us.
  // For now, we only support Vec4f.
  const auto weights_info = VerifyAndCreateVertexAttribute(
      "WEIGHTS_0", gltf_primitive.attributes, model, &vertex_format,
      VertexAttributeUsage_BoneWeights, {TINYGLTF_TYPE_VEC4},
      {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
  if (weights_info) {
    bone_weights = weights_info->data;
    bone_weights_stride = weights_info->byte_stride;
    bone_weights_offset = weights_info->offset;
  }

  // Allocate heap storage for the entire primitive.
  const size_t vertex_size = vertex_format.GetVertexSize();
  DataContainer vertices =
      DataContainer::CreateHeapDataContainer(num_vertices * vertex_size);
  uint8_t* vertex = reinterpret_cast<uint8_t*>(vertices.GetData());

  // Create the index buffer (if one exists).
  DataContainer indices;
  MeshData::IndexType index_type = MeshData::IndexType::kIndexU16;
  if (gltf_primitive.indices != kInvalidTinyGltfIndex) {
    const tinygltf::Accessor& accessor =
        model.accessors[gltf_primitive.indices];
    size_t indices_num_bytes = accessor.count * ElementSizeInBytes(accessor);
    const auto* index_buffer = DataFromGltfAccessor<uint8_t>(model, accessor);
    if (index_buffer == nullptr) {
      LOG(DFATAL) << "Failed to fetch index buffer data.";
      return MeshData();
    }

    // If the Gltf index buffer is unsigned byte, convert to unsigned short.
    std::vector<uint16_t> indices_converted_from_unsigned_byte;
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
      indices_converted_from_unsigned_byte.resize(accessor.count);
      for (size_t i = 0; i < accessor.count; ++i) {
        indices_converted_from_unsigned_byte[i] = index_buffer[i];
      }

      index_buffer = reinterpret_cast<uint8_t*>(
          indices_converted_from_unsigned_byte.data());
      indices_num_bytes *= 2;
    } else {
      index_type = IndexTypeForComponentType(accessor.componentType);
    }

    indices = DataContainer::CreateHeapDataContainer(indices_num_bytes);
    memcpy(indices.GetData(), index_buffer, indices_num_bytes);
    indices.Advance(indices_num_bytes);
  }

  // Generate tangent spaces if possible and if needed.
  std::vector<float> generated_tangents;
  if (positions && normals && uvs_0 && !tangents) {
    generated_tangents.resize(num_vertices * 4);
    tangents = reinterpret_cast<const uint8_t*>(generated_tangents.data());
    std::vector<mathfu::vec3_packed> bitangents;
    bitangents.resize(num_vertices);
    size_t sizeof_index;
    switch (index_type) {
      case MeshData::IndexType::kIndexU16:
        sizeof_index = 2;
        break;
      case MeshData::IndexType::kIndexU32:
        sizeof_index = 4;
        break;
      default:
        LOG(DFATAL) << "Unsupported vertex index type.";
    }

    if (indices.GetSize()) {
      ComputeTangentsWithIndexedTriangles(
          positions, positions_stride, normals, normals_stride, uvs_0,
          uvs_0_stride, num_vertices, indices.GetData(), sizeof_index,
          indices.GetSize() / sizeof_index / 3,
          reinterpret_cast<uint8_t*>(generated_tangents.data()),
          sizeof(mathfu::vec4_packed),
          reinterpret_cast<uint8_t*>(bitangents.data()),
          sizeof(mathfu::vec3_packed));
    } else {
      ComputeTangentsWithTriangles(
          positions, positions_stride, normals, normals_stride, uvs_0,
          uvs_0_stride, num_vertices, indices.GetSize() / sizeof_index / 3,
          reinterpret_cast<uint8_t*>(generated_tangents.data()),
          sizeof(mathfu::vec4_packed),
          reinterpret_cast<uint8_t*>(bitangents.data()),
          sizeof(mathfu::vec3_packed));
    }
  }

  // Copy each vertex into array-of-structs format.
  for (size_t i = 0; i < num_vertices; ++i) {
    if (positions) {
      memcpy(vertex + positions_offset, positions + positions_stride * i,
             sizeof(float) * 3);
    }
    if (uvs_0) {
      memcpy(vertex + uvs_0_offset, uvs_0 + uvs_0_stride * i,
             sizeof(float) * 2);
    }
    if (preserve_normal_tangent_) {
      if (normals) {
        memcpy(vertex + normals_offset, normals + normals_stride * i,
               sizeof(float) * 3);
      }
      if (tangents) {
        memcpy(vertex + tangents_offset, tangents + tangents_stride * i,
               sizeof(float) * 4);
      }
    } else if (normals && tangents) {
      // Create TBN quaternions using the available normals and tangents.
      const mathfu::vec3 normal = mathfu::vec3(
          reinterpret_cast<const float*>(normals + normals_stride * i));
      // TODO: respect the 4th component of the tangent.
      mathfu::vec3 tangent = mathfu::vec3(
          reinterpret_cast<const float*>(tangents + tangents_stride * i));
      mathfu::vec4 quat = OrientationForTbn(normal, tangent);
      if (quat[3] < 0.f) {
        quat *= -1.f;
      }
      memcpy(vertex + orientations_offset, &quat[0], sizeof(float) * 4);
    }
    if (bone_indices) {
      memcpy(vertex + bone_indices_offset,
             bone_indices + bone_indices_stride * i, sizeof(uint8_t) * 4);
    }
    if (bone_weights) {
      memcpy(vertex + bone_weights_offset,
             bone_weights + bone_weights_stride * i, sizeof(float) * 4);
    }
    vertex += vertex_size;
    vertices.Advance(vertex_size);
  }

  // If there is no index buffer, return the MeshData as-is.
  if (gltf_primitive.indices == kInvalidTinyGltfIndex) {
    return MeshData(MeshData::PrimitiveType::kTriangles, vertex_format,
                    std::move(vertices));
  }

  // Otherwise, include it in the MeshData.
  return MeshData(MeshData::PrimitiveType::kTriangles, vertex_format,
                  std::move(vertices), index_type, std::move(indices));
}

void GltfAsset::PrepareBlendShapes(MeshInfo* mesh_info,
                                   const tinygltf::Primitive& gltf_primitive,
                                   const tinygltf::Model& model) {
  // Since all morph targets must specify the same attributes in the same order,
  // use the first one to determine the shared blend format.
  VertexFormat blend_format;
  std::map<std::string, int> attr_map = gltf_primitive.targets.front();

  auto positions_iter = attr_map.find("POSITION");
  if (positions_iter != attr_map.end()) {
    VertexAttribute attribute(VertexAttributeUsage_Position,
                              VertexAttributeType_Vec3f);
    blend_format.AppendAttribute(attribute);
  }

  // Normals and tangents are converted into orientations.
  auto normals_iter = attr_map.find("NORMAL");
  auto tangents_iter = attr_map.find("TANGENT");
  if (preserve_normal_tangent_) {
    if (normals_iter != attr_map.end()) {
      VertexAttribute attribute(VertexAttributeUsage_Normal,
                                VertexAttributeType_Vec3f);
      blend_format.AppendAttribute(attribute);
    }
    if (tangents_iter != attr_map.end()) {
      VertexAttribute attribute(VertexAttributeUsage_Tangent,
                                VertexAttributeType_Vec3f);
      blend_format.AppendAttribute(attribute);
    }
  } else if (normals_iter != attr_map.end() ||
             tangents_iter != attr_map.end()) {
    VertexAttribute attribute(VertexAttributeUsage_Orientation,
                              VertexAttributeType_Vec4f);
    blend_format.AppendAttribute(attribute);
  }

  if (blend_format.GetNumAttributes() == 0) {
    LOG(WARNING) << "No supported blend shape attributes.";
    return;
  }
  mesh_info->blend_shape_format = blend_format;

  // Create a mapping between attributes in the main mesh and attributes in the
  // blend shapes. We will use this mapping to create a "base" blend shape.
  const VertexFormat& mesh_format = mesh_info->mesh_data.GetVertexFormat();
  std::vector<std::pair<const VertexAttribute*, const VertexAttribute*>>
      mesh_to_blend_map;
  for (size_t i = 0; i < mesh_format.GetNumAttributes(); ++i) {
    const VertexAttribute* mesh_attrib = mesh_format.GetAttributeAt(i);
    for (size_t j = 0; j < blend_format.GetNumAttributes(); ++j) {
      const VertexAttribute* blend_attrib = blend_format.GetAttributeAt(j);
      // According to the spec, positions, normals, and tangents can only be
      // floating point vectors. This means we only need to check usage, since
      // types are checked by the Mesh and Blend Shape parsing code.
      if (mesh_attrib->usage() == blend_attrib->usage()) {
        mesh_to_blend_map.emplace_back(mesh_attrib, blend_attrib);
      }
    }
  }

  // Create a copy of the mesh that just contains the data needed for blending.
  const size_t num_vertices = mesh_info->mesh_data.GetNumVertices();
  const size_t mesh_vertex_size = mesh_format.GetVertexSize();
  const size_t blend_vertex_size = blend_format.GetVertexSize();
  mesh_info->base_blend_shape =
      DataContainer::CreateHeapDataContainer(blend_vertex_size * num_vertices);
  for (size_t i = 0; i < num_vertices; ++i) {
    // The mesh and blend shape processing code ensures that their VertexFormats
    // are in the same order, but does not guarantee they contain the same
    // attributes. For example, mesh_format might be "positions-normals-uvs",
    // but blend_format could be just "normals". Because of this, the offsets
    // for both attributes must be used when copying vertices.
    const uint8_t* mesh_vertex =
        mesh_info->mesh_data.GetVertexBytes() + (i * mesh_vertex_size);
    uint8_t* blend_vertex =
        mesh_info->base_blend_shape.GetAppendPtr(blend_vertex_size);
    for (auto& pair : mesh_to_blend_map) {
      const size_t mesh_offset = mesh_format.GetAttributeOffset(pair.first);
      const size_t blend_offset = blend_format.GetAttributeOffset(pair.second);
      const size_t size = blend_format.GetAttributeSize(*pair.second);
      memcpy(blend_vertex + blend_offset, mesh_vertex + mesh_offset, size);
    }
  }

  // Process the individual blend shapes.
  for (const auto& attr_map : gltf_primitive.targets) {
    if (attr_map.empty()) {
      LOG(WARNING) << "Skipping empty blend shape.";
      continue;
    }
    PrepareBlendShape(mesh_info, attr_map, model);
  }
}

void GltfAsset::PrepareBlendShape(MeshInfo* mesh_info,
                                  const std::map<std::string, int>& attr_map,
                                  const tinygltf::Model& model) {
  const size_t num_vertices = mesh_info->mesh_data.GetNumVertices();

  // Fetch pointers to all the supported attributes.
  const uint8_t* positions = nullptr;
  const uint8_t* normals = nullptr;
  const uint8_t* tangents = nullptr;

  // Store strides to navigate each of the attributes.
  size_t positions_stride = 0;
  size_t normals_stride = 0;
  size_t tangents_stride = 0;

  // Store offests for each attribute within a vertex.
  size_t positions_offset = 0;

  // Either normals and tangents are used OR orientations are used.
  size_t normals_offset = 0;
  size_t tangents_offset = 0;
  size_t orientations_offset = 0;

  // Sanity check the vertex format for this blend shape.
  VertexFormat vertex_format;

  // According to the spec, the position attribute must be a Vec3f.
  const auto positions_info = VerifyAndCreateVertexAttribute(
      "POSITION", attr_map, model, &vertex_format,
      VertexAttributeUsage_Position, {TINYGLTF_TYPE_VEC3},
      {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
  if (positions_info) {
    positions = positions_info->data;
    positions_stride = positions_info->byte_stride;
    positions_offset = positions_info->offset;
  }

  const VertexAttributeUsage normals_usage =
      preserve_normal_tangent_ ? VertexAttributeUsage_Normal
                               : VertexAttributeUsage_Orientation;

  // According to the spec, the normal attribute must be a Vec3f.
  const auto normals_info = VerifyAndCreateVertexAttribute(
      "NORMAL", attr_map, model, &vertex_format, normals_usage,
      {TINYGLTF_TYPE_VEC3}, {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
  if (normals_info) {
    normals = normals_info->data;
    normals_stride = normals_info->byte_stride;
    if (preserve_normal_tangent_) {
      normals_offset = normals_info->offset;
    } else {
      orientations_offset = normals_info->offset;
    }
  }

  if (preserve_normal_tangent_) {
    const auto tangents_info = VerifyAndCreateVertexAttribute(
        "TANGENT", attr_map, model, &vertex_format,
        VertexAttributeUsage_Tangent, {TINYGLTF_TYPE_VEC3},
        {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
    if (tangents_info) {
      tangents = normals_info->data;
      tangents_stride = tangents_info->byte_stride;
      tangents_offset = tangents_info->offset;
    }
  } else {
    // Since tangents and normals are both packed into orientations, we only
    // create a vertex attribute if normals were not found. Otherwise, we just
    // fetch the data pointer.
    if (normals == nullptr) {
      const auto tangents_info = VerifyAndCreateVertexAttribute(
          "TANGENT", attr_map, model, &vertex_format,
          VertexAttributeUsage_Orientation, {TINYGLTF_TYPE_VEC3},
          {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
      if (tangents_info) {
        tangents = tangents_info->data;
        tangents_stride = tangents_info->byte_stride;
        orientations_offset = tangents_info->offset;
      }
    } else {
      const auto* tangents_accessor = GetAndVerifyAttributeAccessor(
          attr_map, model, "TANGENT", {TINYGLTF_TYPE_VEC3},
          {TINYGLTF_COMPONENT_TYPE_FLOAT}, num_vertices);
      tangents = DataFromGltfAccessor<uint8_t>(model, *tangents_accessor);
      tangents_stride = ByteStrideFromGltfAccessor(model, *tangents_accessor);
      if (tangents == nullptr) {
        LOG(DFATAL) << "Failed to fetch blend shape TANGENT attribute data.";
      }
    }
  }

  if (mesh_info->blend_shape_format != vertex_format) {
    LOG(DFATAL) << "Mismatched blend shape vertex format: " << vertex_format
                << " does not match " << mesh_info->blend_shape_format;
    return;
  }

  // Allocate heap storage for the entire blend shape.
  const size_t vertex_size = vertex_format.GetVertexSize();
  DataContainer vertices =
      DataContainer::CreateHeapDataContainer(num_vertices * vertex_size);
  uint8_t* vertex = reinterpret_cast<uint8_t*>(vertices.GetData());

  const uint8_t* original_vertex = mesh_info->base_blend_shape.GetReadPtr();
  for (size_t i = 0; i < num_vertices; ++i) {
    // When preserving normals and tangents, blend shapes can operate in
    // displacement mode instead of interpolation mode, so we only need to store
    // the displacements.
    if (preserve_normal_tangent_) {
      if (positions) {
        memcpy(vertex + positions_offset, positions + positions_stride * i,
               sizeof(float) * 3);
      }
      if (normals) {
        memcpy(vertex + normals_offset, normals + normals_stride * i,
               sizeof(float) * 3);
      }
      if (tangents) {
        memcpy(vertex + tangents_offset, tangents + tangents_stride * i,
               sizeof(float) * 3);
      }
    } else {
      // Otherwise, read each displacement, then transform the original vertex
      // data by the displacement to get the weight = 1 vertex data. This is
      // necessary since we cannot convert normal and tangent displacements into
      // orientation displacements.
      if (positions) {
        // The final weight = 1 position is the original position plus the blend
        // shape displacement.
        const mathfu::vec3 base_position(
            reinterpret_cast<const float*>(original_vertex + positions_offset));
        const mathfu::vec3 new_position =
            base_position + mathfu::vec3(reinterpret_cast<const float*>(
                                positions + positions_stride * i));
        memcpy(vertex + positions_offset, &new_position.x, sizeof(float) * 3);
      }
      if (normals || tangents) {
        const uint8_t* orientation = original_vertex + orientations_offset;
        const mathfu::vec4 packed_orientation(
            reinterpret_cast<const float*>(orientation));
        // Orientations are stored XYZW.
        const mathfu::quat base_orientation(
            packed_orientation[3], packed_orientation[0], packed_orientation[1],
            packed_orientation[2]);
        // Since orientations encode a TBN matrix, the X and Z axes can be used
        // to recover the T and N portions.
        mathfu::vec3 new_normal = base_orientation * mathfu::kAxisZ3f;
        mathfu::vec3 new_tangent = base_orientation * mathfu::kAxisX3f;
        if (normals) {
          new_normal += mathfu::vec3(
              reinterpret_cast<const float*>(normals + normals_stride * i));
        }
        if (tangents) {
          new_tangent += mathfu::vec3(
              reinterpret_cast<const float*>(tangents + tangents_stride * i));
        }
        const mathfu::vec4 quat = OrientationForTbn(new_normal, new_tangent);
        memcpy(vertex + orientations_offset, &quat[0], sizeof(float) * 4);
      }
    }
    original_vertex += vertex_size;
    vertex += vertex_size;
    vertices.Advance(vertex_size);
  }
  mesh_info->blend_shapes.emplace_back(std::move(vertices));
}

void GltfAsset::PrepareSkins(const tinygltf::Model& model) {
  skin_infos_.resize(model.skins.size());
  for (size_t i = 0; i < model.skins.size(); ++i) {
    PrepareSkin(&skin_infos_[i], model.skins[i], model);
  }
}

void GltfAsset::PrepareSkin(SkinInfo* skin_info,
                            const tinygltf::Skin& gltf_skin,
                            const tinygltf::Model& model) {
  // GLTF skins don't have to specify inverse bind matrices. If unspecified,
  // use an identity matrix for each. Otherwise, validate the inverse bind
  // matrix accessor and copy them to the skin info.
  const size_t num_bones = gltf_skin.joints.size();
  if (gltf_skin.inverseBindMatrices == kInvalidTinyGltfIndex) {
    skin_info->inverse_bind_matrices.resize(num_bones, mathfu::kAffineIdentity);
  } else {
    const tinygltf::Accessor& ibm_accessor =
        model.accessors[gltf_skin.inverseBindMatrices];
    if (ibm_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
        ibm_accessor.type != TINYGLTF_TYPE_MAT4) {
      LOG(DFATAL) << "Skin inverse bind matrix accessor does not access "
                  << "Mat4fs.";
      return;
    }
    if (ibm_accessor.count != num_bones) {
      LOG(DFATAL) << "Skin does not have one inverse bind matrix per joint.";
      return;
    }

    // Fetch the inverse bind matrices as raw floating point data, then copy it
    // into the SkinInfo.
    const auto* ibms = DataFromGltfAccessor<float>(model, ibm_accessor);
    if (ibms == nullptr) {
      LOG(DFATAL) << "Failed to fetch inverse bind matrix data.";
      return;
    }
    skin_info->inverse_bind_matrices.reserve(num_bones);
    for (size_t i = 0; i < num_bones; ++i) {
      const size_t base = i * 16;
      // GLTF stores matrices in column-major order, but mathfu is row-major.
      skin_info->inverse_bind_matrices.emplace_back(
          ibms[base], ibms[base + 4], ibms[base + 8], ibms[base + 12],
          ibms[base + 1], ibms[base + 5], ibms[base + 9], ibms[base + 13],
          ibms[base + 2], ibms[base + 6], ibms[base + 10], ibms[base + 14]);
    }
  }

  // Configure basic Skin info now that inverse bind matrix parsing is complete.
  skin_info->name = gltf_skin.name;
  skin_info->bones = gltf_skin.joints;
}

void GltfAsset::PrepareAnimations(const tinygltf::Model& model) {
  anim_infos_.resize(model.animations.size());
  for (size_t i = 0; i < model.animations.size(); ++i) {
    PrepareAnimation(&anim_infos_[i], model.animations[i], model);
  }
}

void GltfAsset::PrepareAnimation(AnimationInfo* anim_info,
                                 const tinygltf::Animation& gltf_anim,
                                 const tinygltf::Model& model) {
  // Determine which nodes need to be animated and track the channels that
  // animate their properties.
  std::unordered_map<int, TinyGltfNodeAnimationData> node_to_anims;
  for (const tinygltf::AnimationChannel& channel : gltf_anim.channels) {
    auto res = node_to_anims.emplace(
        channel.target_node,
        TinyGltfNodeAnimationData(model.nodes[channel.target_node], model));
    // TODO: support blend shape weight animations.
    // For now, just ignore these channels instead of ending parsing.
    if (!res.first->second.SetChannel(&gltf_anim, &channel)) {
      LOG(WARNING) << "Unsupported animation channel target path "
                   << channel.target_path;
    }
  }

  // Determine the buffer size and total number of splines to represent the
  // entire animation.
  size_t buffer_size = 0;
  size_t num_splines = 0;
  for (auto iter : node_to_anims) {
    const auto& anim_data = iter.second;
    const Optional<size_t> size = GetRequiredBufferSize(anim_data);
    if (!size) {
      LOG(DFATAL) << "Animation sampler had an invalid type or too many nodes.";
      return;
    }
    buffer_size += *size;
    num_splines += anim_data.GetRequiredSplineCount();
  }

  DataContainer spline_buffer =
      DataContainer::CreateHeapDataContainer(buffer_size);
  uint8_t* buffer = spline_buffer.GetData();
  anim_info->context =
      MakeUnique<SkeletonChannel::AnimationContext>(node_to_anims.size());

  for (auto iter : node_to_anims) {
    const auto& anim_data = iter.second;
    anim_info->context->CreateTarget(
        iter.first, anim_data.HasTranslation(), anim_data.HasRotation(),
        anim_data.HasScale(), anim_data.weights_channel_count);
    Optional<size_t> bytes_used = AddAnimationData(buffer, anim_data);
    if (!bytes_used) {
      LOG(DFATAL) << "Failed to add animation splines.";
      return;
    }
    // This should never fail because we determined the necessary size for the
    // buffer up-front.
    if (!spline_buffer.Advance(*bytes_used)) {
      LOG(DFATAL) << "Failed to advance spline buffer.";
      return;
    }
    buffer += *bytes_used;
  }

  DCHECK_EQ(spline_buffer.GetSize(), spline_buffer.GetCapacity())
      << "Spline buffer capacity should exactly match size.";

  anim_info->splines = std::move(spline_buffer);
  anim_info->num_splines = num_splines;
  anim_info->name = gltf_anim.name;
}

void GltfAsset::PrepareTextures(const tinygltf::Model& model,
                                string_view directory) {
  // TODO textures.
  // TODO Support glTF's sampler attributes.
  for (const tinygltf::Texture& texture : model.textures) {
    TextureInfo texture_info;

    const std::string uri =
        JoinPath(directory, model.images[texture.source].uri);
    texture_info.name = uri;
    texture_info.file = uri;

    texture_infos_.push_back(std::move(texture_info));
  }
}

template <typename Map, typename Key, typename Fn>
void IfInMap(const Map& map, const Key& key, Fn fn) {
  auto itr = map.find(key);
  if (itr != map.end()) {
    fn(itr->second);
  }
}

void GltfAsset::PrepareMaterials(const tinygltf::Model& model) {
  for (const tinygltf::Material& material : model.materials) {
    MaterialInfo material_info("pbr");
    VariantMap properties;

    // Per the glTF spec, baseColorFactor is not required to occur in the glTF
    // but must still be defaulted to one.
    properties[ConstHash("BaseColor")] = mathfu::vec4(1, 1, 1, 1);
    IfInMap(material.values, "baseColorFactor",
            [&](const tinygltf::Parameter& param) {
              properties[ConstHash("BaseColor")] = ColorFactorToVec4(param);
            });

    IfInMap(material.values, "baseColorTexture",
            [&](const tinygltf::Parameter& param) {
              const TextureInfo& texture_info =
                  texture_infos_[param.TextureIndex()];
              material_info.SetTexture(
                  TextureUsageInfo(MaterialTextureUsage_BaseColor),
                  texture_info.name);
            });

    IfInMap(material.additionalValues, "normalTexture",
            [&](const tinygltf::Parameter& param) {
              const TextureInfo& texture_info =
                  texture_infos_[param.TextureIndex()];
              material_info.SetTexture(
                  TextureUsageInfo(MaterialTextureUsage_Normal),
                  texture_info.name);
            });

    IfInMap(material.additionalValues, "emissiveFactor",
            [&](const tinygltf::Parameter& param) {
              properties[ConstHash("Emissive")] = ColorFactorToVec4(param);
            });

    IfInMap(material.additionalValues, "emissiveTexture",
            [&](const tinygltf::Parameter& param) {
              const TextureInfo& texture_info =
                  texture_infos_[param.TextureIndex()];
              material_info.SetTexture(
                  TextureUsageInfo(MaterialTextureUsage_Emissive),
                  texture_info.name);
            });

    // Per the glTF spec, metallicFactor is not required to occur in the glTF
    // but must still be defaulted to one.
    properties[ConstHash("Metallic")] = 1.f;
    IfInMap(material.values, "metallicFactor",
            [&](const tinygltf::Parameter& param) {
              properties[ConstHash("Metallic")] = FactorToFloat(param);
            });

    // Per the glTF spec, roughnessFactor is not required to occur in the glTF
    // but must still be defaulted to one.
    properties[ConstHash("Roughness")] = 1.f;
    IfInMap(material.values, "roughnessFactor",
            [&](const tinygltf::Parameter& param) {
              properties[ConstHash("Roughness")] = FactorToFloat(param);
            });

    // Since occlusion may or may not be folded in with roughness-metallic, we
    // can't use the IfInMap convenience function.
    auto occlusion_texture = material.additionalValues.find("occlusionTexture");
    auto metallic_roughness_texture =
        material.values.find("metallicRoughnessTexture");

    const bool has_occlusion =
        occlusion_texture != material.additionalValues.end();
    const bool has_metallic_roughness =
        metallic_roughness_texture != material.values.end();

    if (has_occlusion && has_metallic_roughness &&
        occlusion_texture->second.TextureIndex() ==
            metallic_roughness_texture->second.TextureIndex()) {
      // If occlusion and roughness/metallic textures are the same, pair them
      // into one usage.
      const TextureInfo& texture_info =
          texture_infos_[metallic_roughness_texture->second.TextureIndex()];

      material_info.SetTexture(
          TextureUsageInfo({MaterialTextureUsage_Occlusion,
                            MaterialTextureUsage_Roughness,
                            MaterialTextureUsage_Metallic}),
          texture_info.name);
    } else {
      if (has_occlusion) {
        const TextureInfo& texture_info =
            texture_infos_[occlusion_texture->second.TextureIndex()];

        material_info.SetTexture(
            TextureUsageInfo(MaterialTextureUsage_Occlusion),
            texture_info.name);
      }

      if (has_metallic_roughness) {
        const TextureInfo& texture_info =
            texture_infos_[metallic_roughness_texture->second.TextureIndex()];

        material_info.SetTexture(
            TextureUsageInfo({MaterialTextureUsage_Unused,
                              MaterialTextureUsage_Roughness,
                              MaterialTextureUsage_Metallic}),
            texture_info.name);
      }
    }

    material_info.SetProperties(properties);
    material_infos_.push_back(std::move(material_info));
  }
}

void GltfAsset::OnFinalize(const std::string& filename, std::string* data) {
  if (finalize_callback_) {
    finalize_callback_();
  }
}

}  // namespace lull
