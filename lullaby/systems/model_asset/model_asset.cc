/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/model_asset/model_asset.h"

#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/make_unique.h"

namespace lull {

ModelAsset::ModelAsset(std::function<void()> finalize_callback)
    : finalize_callback_(std::move(finalize_callback)) {}

void ModelAsset::OnLoad(const std::string& filename, std::string* data) {
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data->data()),
                                 data->size());
  if (!verifier.VerifyBuffer<ModelDef>()) {
    LOG(DFATAL) << filename << " is an invalid model_asset.";
    return;
  }

  model_def_.Set(std::move(*data));
  if (!model_def_) {
    return;
  }
  PrepareMesh();
  PrepareMaterials();
  PrepareTextures();
  PrepareSkeleton();
}

void ModelAsset::OnFinalize(const std::string& filename, std::string* data) {
  if (finalize_callback_) {
    finalize_callback_();
  }
}

void ModelAsset::PrepareMesh() {
  const int lod = 0;
  if (model_def_->lods() == nullptr || model_def_->lods()->size() != 1) {
    LOG(DFATAL) << "Only support single LODs for now.";
    return;
  }
  const ModelInstanceDef* model = model_def_->lods()->Get(lod);

  const uint32_t num_vertices = model->num_vertices();
  if (num_vertices == 0) {
    LOG(DFATAL) << "Model must have vertices.";
    return;
  }

  MeshData::IndexType index_type = MeshData::kIndexU16;
  const uint8_t* index_bytes = nullptr;
  uint32_t num_indices = 0;

  if (model->indices16()) {
    index_type = MeshData::kIndexU16;
    index_bytes = reinterpret_cast<const uint8_t*>(model->indices16()->data());
    num_indices = model->indices16()->size();
  } else if (model->indices32()) {
    index_type = MeshData::kIndexU32;
    index_bytes = reinterpret_cast<const uint8_t*>(model->indices32()->data());
    num_indices = model->indices32()->size();
  } else {
    LOG(DFATAL) << "Model must have indices.";
    return;
  }

  const uint32_t num_materials =
      model->materials() ? model->materials()->size() : 0;
  if (num_materials == 0) {
    LOG(DFATAL) << "Model must have materials.";
    return;
  }

  if (model->vertex_attributes() == nullptr) {
    LOG(DFATAL) << "Model must have attributes.";
    return;
  }

  VertexFormat vertex_format;
  for (auto attrib : *model->vertex_attributes()) {
    vertex_format.AppendAttribute(*attrib);
  }

  const uint8_t* vertex_bytes = model->vertex_data()->data();
  const size_t vertex_num_bytes = model->vertex_data()->size();
  const size_t index_num_bytes =
      num_indices * MeshData::GetIndexSize(index_type);
  const auto* range_bytes =
      reinterpret_cast<const uint8_t*>(model->ranges()->data());
  const size_t range_num_bytes = num_materials * sizeof(MeshData::IndexRange);

  DataContainer vertices =
      DataContainer::WrapDataAsReadOnly(vertex_bytes, vertex_num_bytes);
  DataContainer indices =
      DataContainer::WrapDataAsReadOnly(index_bytes, index_num_bytes);
  DataContainer submeshes =
      DataContainer::WrapDataAsReadOnly(range_bytes, range_num_bytes);
  mesh_data_ =
      MeshData(MeshData::kTriangles, vertex_format, std::move(vertices),
               index_type, std::move(indices), std::move(submeshes));
}

static MaterialInfo BuildMaterialInfo(const MaterialDef* material_def) {
  VariantMap properties;
  VariantMapFromFbVariantMap(material_def->properties(), &properties);

  std::string shading_model;
  constexpr HashValue kShadingModel = ConstHash("ShadingModel");
  auto iter = properties.find(kShadingModel);
  if (iter != properties.end()) {
    shading_model = iter->second.ValueOr(shading_model);
  }

  MaterialInfo material(shading_model);
  material.SetProperties(std::move(properties));

  if (material_def->textures()) {
    for (uint32_t i = 0; i < material_def->textures()->size(); ++i) {
      const MaterialTextureDef* texture_def = material_def->textures()->Get(i);
      if (texture_def->name()) {
        material.SetTexture(texture_def->usage(), texture_def->name()->c_str());
      }
    }
  }
  return material;
}

void ModelAsset::PrepareMaterials() {
  if (model_def_->lods() == nullptr) {
    LOG(DFATAL) << "No geometry/model data in the lullmodel file.";
    return;
  }
  if (model_def_->lods()->size() != 1) {
    LOG(DFATAL) << "Lullaby currently does not support multiple LODs";
    return;
  }

  const int lod = 0;
  const ModelInstanceDef* model = model_def_->lods()->Get(lod);
  const uint32_t num_materials =
      model->materials() ? model->materials()->size() : 0;

  materials_.reserve(num_materials);
  for (uint32_t i = 0; i < num_materials; ++i) {
    const MaterialDef* material_def = model->materials()->Get(i);
    if (material_def) {
      materials_.emplace_back(BuildMaterialInfo(material_def));
    } else {
      LOG(DFATAL) << "Invalid/missing MaterialDef.";
    }
  }
}

static ModelAsset::TextureInfo BuildTextureInfo(const TextureDef* texture_def) {
  ModelAsset::TextureInfo info;
  if (texture_def->name()) {
    info.name = texture_def->name()->str();
  }
  if (texture_def->file()) {
    info.file = texture_def->file()->str();
  }
  info.params.generate_mipmaps = texture_def->generate_mipmaps();
  info.params.premultiply_alpha = texture_def->premultiply_alpha();
  info.params.min_filter = texture_def->min_filter();
  info.params.mag_filter = texture_def->mag_filter();
  info.params.wrap_s = texture_def->wrap_s();
  info.params.wrap_t = texture_def->wrap_t();
  if (texture_def->data() && texture_def->data()->data()) {
    const uint8_t* bytes = texture_def->data()->data();
    const size_t num_bytes = texture_def->data()->size();
    info.data = DecodeImage(bytes, num_bytes, kDecodeImage_None);
  }
  return info;
}

void ModelAsset::PrepareTextures() {
  const uint32_t num_textures =
      model_def_->textures() ? model_def_->textures()->size() : 0;

  textures_.reserve(num_textures);
  for (uint32_t i = 0; i < num_textures; ++i) {
    const TextureDef* texture_def = model_def_->textures()->Get(i);
    if (texture_def) {
      textures_.emplace_back(BuildTextureInfo(texture_def));
    }
  }
}

bool ModelAsset::HasValidSkeleton() const {
  const auto* skeleton = model_def_->skeleton();
  return skeleton != nullptr && skeleton->bone_names() != nullptr;
}

void ModelAsset::PrepareSkeleton() {
  const auto* skeleton = model_def_->skeleton();
  if (!HasValidSkeleton()) {
    return;
  }

  bone_names_.reserve(skeleton->bone_names()->size());
  for (size_t i = 0; i < bone_names_.capacity(); ++i) {
    bone_names_.push_back(
        skeleton->bone_names()->Get(static_cast<uint32_t>(i))->str());
  }
}

Span<uint8_t> ModelAsset::GetParentBoneIndices() const {
  const auto* skeleton = model_def_->skeleton();
  return {skeleton->bone_parents()->data(), skeleton->bone_parents()->size()};
}

Span<uint8_t> ModelAsset::GetShaderBoneIndices() const {
  const ModelInstanceDef* model = model_def_->lods()->Get(0);
  return {model->shader_to_mesh_bones()->data(),
          model->shader_to_mesh_bones()->size()};
}

Span<mathfu::AffineTransform> ModelAsset::GetInverseBindPose() const {
  const auto* skeleton = model_def_->skeleton();
  return {reinterpret_cast<const mathfu::AffineTransform*>(
              skeleton->bone_transforms()->data()),
          skeleton->bone_transforms()->size()};
}

const ModelDef* ModelAsset::GetModelDef() { return model_def_.get(); }

}  // namespace lull
