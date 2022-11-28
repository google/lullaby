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

#include "redux/systems/model/model_asset.h"

#include "redux/data/asset_defs/model_asset_def_generated.h"
#include "redux/modules/flatbuffers/math.h"
#include "redux/modules/flatbuffers/var.h"
#include "redux/modules/graphics/image_utils.h"

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
    texture.texture = texture_def->name()->name()->str();
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

static MeshData::PartData ReadMeshPart(
    const ModelInstancePartAssetDef* part_def) {
  MeshData::PartData part_data;
  if (part_def) {
    if (part_def->name()) {
      part_data.name = ReadFbs(*part_def->name());
    }
    if (part_def->bounding_box()) {
      part_data.box = ReadFbs(*part_def->bounding_box());
    }
    part_data.primitive_type = MeshPrimitiveType::Triangles;
    CHECK(part_def->range());
    part_data.start = part_def->range()->start();
    part_data.end = part_def->range()->end();
  }
  return part_data;
}

std::shared_ptr<ImageData> ModelAsset::ReadImage(ImageData image,
                                                 const ImageDecoder& decoder) {
  if (IsCompressedFormat(image.GetFormat()) && decoder) {
    decoded_images_.push_back(
        std::make_shared<ImageData>(decoder(std::move(image))));
    return decoded_images_.back();
  } else {
    return std::make_shared<ImageData>(std::move(image));
  }
}

static ModelAsset::TextureData ReadTextureData(
    const ModelTextureAssetDef* model_texture_def,
    const ModelAsset::ImageDecoder& decoder) {
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

void ModelAsset::OnLoad(std::shared_ptr<DataContainer> data,
                        ImageDecoder decoder) {
  data_ = std::move(data);

  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data_->GetBytes());
  flatbuffers::Verifier verifier(ptr, data_->GetNumBytes(), 64 /* max_depth */,
                                 1000000 /* max_tables */,
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

  const ModelVertexBufferAssetDef* vertex_buffer = instance->vertices();
  const ModelIndexBufferAssetDef* index_buffer = instance->indices();

  ReadVector(&parts_, instance->parts(), ReadMeshPart);
  ReadVector(&materials_, instance->parts(), ReadMaterial);

  vertex_format_ = ReadVertexFormat(vertex_buffer);
  vertex_data_ = ReadVertexData(vertex_buffer);
  index_format_ = ReadIndexType(index_buffer);
  index_data_ = ReadIndexData(index_buffer);

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
      textures_[name] = ReadTextureData(model_texture_def, decoder);
      if (model_texture_def->texture() &&
          model_texture_def->texture()->image()) {
        const auto& image_def = model_texture_def->texture()->image();
        auto bytes = AsByteSpan(image_def->data());
        const ImageFormat format = image_def->format();
        vec2i size = ReadFbs(*image_def->size());
        ImageData image(format, size, DataContainer::WrapData(bytes));
        textures_[name].image = ReadImage(std::move(image), decoder);
      }
    }
  }
}

const ModelAsset::TextureData* ModelAsset::GetTextureData(
    std::string_view name) const {
  auto iter = textures_.find(Hash(name));
  if (iter != textures_.end()) {
    return &iter->second;
  } else {
    return nullptr;
  }
}

MeshData ModelAsset::GetMeshData() const {
  DataContainer vertices =
      DataContainer::WrapDataInSharedPtr(vertex_data_, data_);
  DataContainer indices =
      DataContainer::WrapDataInSharedPtr(index_data_, data_);
  DataContainer parts = DataContainer::WrapData(parts_.data(), parts_.size());

  MeshData mesh_data;
  mesh_data.SetVertexData(vertex_format_, std::move(vertices), bounds_);
  if (!index_data_.empty()) {
    mesh_data.SetIndexData(index_format_, std::move(indices));
  }
  mesh_data.SetParts(parts.Clone());
  return mesh_data;
}
}  // namespace redux
