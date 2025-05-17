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

#include "redux/systems/model/redux_model_asset.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "redux/data/asset_defs/model_asset_def_generated.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/codecs/decode_image.h"
#include "redux/modules/flatbuffers/math.h"
#include "redux/modules/flatbuffers/var.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/graphics/image_utils.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"
#include "redux/systems/model/model_asset.h"
#include "redux/systems/model/model_asset_factory.h"

namespace redux {

template <typename T>
static absl::Span<const std::byte> AsByteSpan(
    const flatbuffers::Vector<T>* vec) {
  if (vec) {
    const auto bytes = reinterpret_cast<const std::byte*>(vec->data());
    const size_t num_bytes = vec->size() * sizeof(T);
    return {bytes, num_bytes};
  } else {
    return {};
  }
}

static mat4 ReadMat4x3IntoMat4(const fbs::Mat3x4f* mat) {
  if (mat) {
    const float m00 = mat->col0().x();
    const float m10 = mat->col0().y();
    const float m20 = mat->col0().z();
    const float m01 = mat->col1().x();
    const float m11 = mat->col1().y();
    const float m21 = mat->col1().z();
    const float m02 = mat->col2().x();
    const float m12 = mat->col2().y();
    const float m22 = mat->col2().z();
    const float m03 = mat->col3().x();
    const float m13 = mat->col3().y();
    const float m23 = mat->col3().z();
    return mat4(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, 0.f,
                0.f, 0.f, 1.f);
  } else {
    return mat4::Identity();
  }
}

template <typename Fn, typename T, typename U>
void ReadVector(std::vector<U>* out, const flatbuffers::Vector<T>* vec, Fn fn) {
  if (vec && out) {
    out->reserve(vec->size());
    for (const auto* def : *vec) {
      if (def) {
        out->push_back(fn(def));
      }
    }
  }
}

static MeshIndexType ReadIndexType(const ModelIndexBufferAssetDef* buffer) {
  if (buffer == nullptr) {
    return MeshIndexType::U16;
  } else if (buffer->data16()) {
    return MeshIndexType::U16;
  } else if (buffer->data32()) {
    return MeshIndexType::U32;
  } else {
    return MeshIndexType::U16;
  }
}

static VertexFormat ReadVertexFormat(const ModelVertexBufferAssetDef* buffer) {
  VertexFormat vertex_format;
  for (const ModelVertexAttributeAssetDef* attrib : *buffer->vertex_format()) {
    vertex_format.AppendAttribute({attrib->usage(), attrib->type()});
  }
  return vertex_format;
}

static absl::Span<const std::byte> ReadVertexData(
    const ModelVertexBufferAssetDef* buffer) {
  if (buffer == nullptr) {
    return {};
  } else {
    return AsByteSpan(buffer->data());
  }
}

static absl::Span<const std::byte> ReadIndexData(
    const ModelIndexBufferAssetDef* buffer) {
  if (buffer == nullptr) {
    return {};
  } else if (buffer->data16()) {
    return AsByteSpan(buffer->data16());
  } else if (buffer->data32()) {
    return AsByteSpan(buffer->data32());
  } else {
    return {};
  }
}

static MaterialData::TextureData ReadMaterialTextureData(
    const MaterialTextureAssetDef* texture_def) {
  MaterialData::TextureData texture;
  if (texture_def == nullptr) {
    return texture;
  }

  if (texture_def->name() && texture_def->name()->name()) {
    texture.name = texture_def->name()->name()->str();
  }

  if (texture_def->usage()) {
    CHECK_LE(texture_def->usage()->size(), TextureUsage::kNumChannels);
    for (size_t i = 0; i < texture_def->usage()->size(); ++i) {
      texture.usage.channel[i] =
          static_cast<MaterialTextureType>(texture_def->usage()->Get(i));
    }
  }
  return texture;
}

static MaterialData ReadMaterial(const ModelInstancePartAssetDef* part_def) {
  MaterialData material;
  if (part_def == nullptr) {
    return material;
  }
  const MaterialAssetDef* material_def = part_def->material();
  if (material_def == nullptr) {
    return material;
  }

  TryReadFbs(material_def->properties(), &material.properties);

  // Read the shading model from the properties.
  static constexpr HashValue kShadingModel = ConstHash("ShadingModel");
  material.shading_model =
      material.properties[kShadingModel].ValueOr(std::string());

  ReadVector(&material.textures, material_def->textures(),
             ReadMaterialTextureData);

  return material;
}

static ModelAsset::TextureData ReadTextureData(
    const ModelTextureAssetDef* model_texture_def) {
  ModelAsset::TextureData info;
  if (model_texture_def->uri()) {
    info.uri = model_texture_def->uri()->c_str();
  }
  const TextureAssetDef* texture_def = model_texture_def->texture();
  if (texture_def) {
    info.params.generate_mipmaps = texture_def->generate_mipmaps();
    info.params.premultiply_alpha = texture_def->premultiply_alpha();
    info.params.min_filter = texture_def->min_filter();
    info.params.mag_filter = texture_def->mag_filter();
    info.params.wrap_s = texture_def->wrap_s();
    info.params.wrap_t = texture_def->wrap_t();
  }
  return info;
}

void ReduxModelAsset::ProcessData() {
  const uint8_t* ptr =
      reinterpret_cast<const uint8_t*>(asset_data_->GetBytes());
  flatbuffers::Verifier verifier(ptr, asset_data_->GetNumBytes(),
                                 64 /* max_depth */, 1000000 /* max_tables */,
                                 false /* check_alignment */);
  if (!verifier.VerifyBuffer<ModelAssetDef>()) {
    LOG(FATAL) << "Invalid model asset.";
    return;
  }

  const ModelAssetDef* model_def = flatbuffers::GetRoot<ModelAssetDef>(ptr);

  const int lod = 0;
  if (model_def->lods() == nullptr || model_def->lods()->size() != 1) {
    LOG(FATAL) << "Only support single LODs for now.";
    return;
  }

  const ModelInstanceAssetDef* instance = model_def->lods()->Get(lod);
  CHECK(instance);

  // All meshes share the same vertex buffer (since an rxmodel only contains
  // a single vertex buffer).
  const ModelVertexBufferAssetDef* vertex_buffer = instance->vertices();
  const VertexFormat vertex_format = ReadVertexFormat(vertex_buffer);
  const ByteSpan vertex_data = ReadVertexData(vertex_buffer);

  const ModelIndexBufferAssetDef* index_buffer = instance->indices();
  const MeshIndexType index_format = ReadIndexType(index_buffer);
  const size_t index_size = GetMeshIndexTypeSize(index_format);
  const ByteSpan index_data = ReadIndexData(index_buffer);

  if (instance->parts() && instance->parts()->size() > 0) {
    meshes_.reserve(instance->parts()->size());
    for (const ModelInstancePartAssetDef* part_def : *instance->parts()) {
      CHECK(part_def);

      auto mesh = std::make_shared<MeshData>();
      if (part_def->name()) {
        mesh->SetName(ReadFbs(*part_def->name()));
      }

      Box bounding_box;
      if (part_def->bounding_box()) {
        bounding_box = ReadFbs(*part_def->bounding_box());
      }

      auto vertices =
          DataContainer::WrapDataInSharedPtr(vertex_data, asset_data_);
      mesh->SetVertexData(vertex_format, std::move(vertices), bounding_box);

      if (!index_data.empty()) {
        // Find the subset of the index buffer that defines each mesh.
        CHECK(part_def->range());
        const std::uint32_t start = part_def->range()->start();
        const std::uint32_t end = part_def->range()->end();
        const size_t count = end - start;
        const size_t offset = start * index_size;
        const size_t length = count * index_size;
        const auto subset = absl::MakeSpan(index_data.begin() + offset, length);
        auto indices = DataContainer::WrapDataInSharedPtr(subset, asset_data_);
        mesh->SetIndexData(index_format, MeshPrimitiveType::Triangles,
                           std::move(indices));
      }
      meshes_.push_back(std::move(mesh));
    }
  }

  ReadVector(&materials_, instance->parts(), ReadMaterial);

  if (instance->blend_shapes() && instance->blend_shapes()->size() > 0) {
    // We assume the first blend shape has the format.
    blend_format_ =
        ReadVertexFormat(instance->blend_shapes()->Get(0)->vertices());

    for (const ModelBlendShapeAssetDef* shape : *instance->blend_shapes()) {
      CHECK(shape->name());
      const HashValue name(shape->name()->hash());
      blend_shapes_[name] = ReadVertexData(shape->vertices());
    }
  }

  const ModelSkeletonAssetDef* skeleton = model_def->skeleton();
  if (skeleton && skeleton->bone_names() &&
      skeleton->bone_names()->size() > 0) {
    bone_names_.reserve(skeleton->bone_names()->size());
    for (const auto* name : *skeleton->bone_names()) {
      bone_names_.push_back(name->c_str());
    }

    const auto* parents = skeleton->bone_parents();
    parent_bones_ = {parents->data(), parents->size()};

    const auto* bone_mapping = instance->shader_to_mesh_bones();
    shader_bones_ = {bone_mapping->data(), bone_mapping->size()};

    ReadVector(&inverse_bind_pose_, skeleton->bone_transforms(),
               ReadMat4x3IntoMat4);
  }

  if (model_def->textures()) {
    for (const auto* model_texture_def : *model_def->textures()) {
      CHECK(model_texture_def->name());
      const HashValue name = ReadFbs(*model_texture_def->name());
      textures_[name] = ReadTextureData(model_texture_def);
      if (model_texture_def->texture() &&
          model_texture_def->texture()->image()) {
        const auto& image_def = model_texture_def->texture()->image();
        auto bytes = AsByteSpan(image_def->data());
        const ImageFormat format = image_def->format();
        vec2i size = ReadFbs(*image_def->size());
        ImageData image(format, size, DataContainer::WrapData(bytes));
        textures_[name].image = ReadImage(std::move(image));
      }
    }
  }
}

std::shared_ptr<ImageData> ReduxModelAsset::ReadImage(ImageData image) {
  if (IsCompressedFormat(image.GetFormat())) {
    ImageData uncompressed_image = DecodeImage(
        DataContainer::WrapData(image.GetData(), image.GetNumBytes()), {});
    decoded_images_.push_back(
        std::make_shared<ImageData>(std::move(uncompressed_image)));
    return decoded_images_.back();
  } else {
    return std::make_shared<ImageData>(std::move(image));
  }
}
}  // namespace redux

REDUX_REGISTER_MODEL_ASSET(rxmodel, redux::ReduxModelAsset);
