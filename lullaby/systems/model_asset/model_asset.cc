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
#include "lullaby/systems/render/mesh_factory.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/util/make_unique.h"

namespace lull {

ModelAsset::ModelAsset(Registry* registry, const ModelAssetDef* asset_def)
    : registry_(registry), asset_def_(asset_def) {}

ModelAsset::~ModelAsset() {
  auto* texture_factory = registry_->Get<TextureFactory>();
  for (const auto& texture : textures_) {
    texture_factory->ReleaseTexture(texture);
  }
}

void ModelAsset::OnLoad(const std::string& filename, std::string* data) {
  raw_data_ = std::move(*data);
  model_def_ = GetModelDef(raw_data_.c_str());
  if (model_def_ == nullptr) {
    return;
  }
  if (model_def_->lods() == nullptr || model_def_->lods()->size() == 0) {
    model_def_ = nullptr;
    return;
  }

  BuildMesh();
  BuildMaterials();
}

void ModelAsset::OnFinalize(const std::string& filename, std::string* data) {
  if (model_def_ == nullptr) {
    return;
  }

  PrepareTextures();

  ready_ = true;
  for (const Entity entity : pending_entities_) {
    AddEntity(entity);
  }
  pending_entities_.clear();
}

void ModelAsset::AddEntity(Entity entity) {
  if (entity == kNullEntity) {
    return;
  } else if (!ready_) {
    pending_entities_.push_back(entity);
    return;
  }
  if (model_def_) {
    SetMesh(entity);
    SetMaterials(entity);
    SetRig(entity);
  }
}

void ModelAsset::SetMesh(Entity entity) {
  if (mesh_ == nullptr && mesh_data_ != nullptr) {
    auto* mesh_factory = registry_->Get<MeshFactory>();
    if (mesh_factory) {
      mesh_ = mesh_factory->CreateMesh(std::move(*mesh_data_));
      mesh_data_.reset();
    }
  }

  auto* render_system = registry_->Get<RenderSystem>();
  if (render_system && mesh_) {
    HashValue pass = RenderSystem::kDefaultPass;
    if (asset_def_ && asset_def_->pass() != 0) {
      pass = asset_def_->pass();
    }
    render_system->Create(entity, pass);
    render_system->SetMesh(entity, pass, mesh_);
  }
}

void ModelAsset::SetMaterials(Entity entity) {
  auto* render_system = registry_->Get<RenderSystem>();
  if (render_system == nullptr) {
    return;
  }

  for (size_t i = 0; i < materials_.size(); ++i) {
    render_system->SetMaterial(entity, static_cast<int>(i), materials_[i]);
    if (asset_def_ && asset_def_->materials()) {
      for (const auto mat : *asset_def_->materials()) {
        // We only support LOD 0 for now.  An LOD of -1 applies to all LODs.
        if (mat->lod() != -1 && mat->lod() != 0) {
          continue;
        } else if (mat->submesh() != -1 && mat->submesh() != i) {
          continue;
        } else {
          ApplyUniforms(entity, materials_[i], mat);
          break;
        }
      }
    }
  }
}

void ModelAsset::ApplyUniforms(Entity entity, const MaterialInfo& material,
                               const ModelAssetMaterialDef* def) {
  if (!def->shading_uniforms()) {
    return;
  }

  auto* render_system = registry_->Get<RenderSystem>();
  for (const ShaderUniformDef* uniform : *def->shading_uniforms()) {
    const int count = uniform->array_size();
    CHECK(count == 0);

    const char* name = uniform->name()->c_str();
    if (name == nullptr || *name == 0) {
      continue;
    }
    const HashValue key = Hash(name);

    int dimension = 0;
    const float* data = nullptr;
    switch (uniform->type()) {
      case ShaderUniformType_Float1: {
        const auto* ptr = material.GetProperty<float>(key);
        data = ptr;
        dimension = 1;
        break;
      }
      case ShaderUniformType_Float3: {
        const auto* ptr = material.GetProperty<mathfu::vec3>(key);
        data = ptr ? &ptr->x : nullptr;
        dimension = 3;
        break;
      }
      case ShaderUniformType_Float4: {
        const auto* ptr = material.GetProperty<mathfu::vec4>(key);
        data = ptr ? &ptr->x : nullptr;
        dimension = 4;
        break;
      }
      default:
        LOG(ERROR) << "Only support 1d, 3d, and 4d float types.";
        break;
    }
    if (data) {
      HashValue pass = RenderSystem::kDefaultPass;
      if (asset_def_ && asset_def_->pass() != 0) {
        pass = asset_def_->pass();
      }
      render_system->SetUniform(entity, pass, name, data, dimension, 1);
    }
  }
}

void ModelAsset::SetRig(Entity entity) {
  auto* rig_system = registry_->Get<RigSystem>();
  const auto* skeleton = model_def_->skeleton();
  if (rig_system && skeleton) {
    const ModelInstanceDef* model = model_def_->lods()->Get(0);

    std::vector<std::string> bone_names;
    bone_names.reserve(skeleton->bone_names()->size());
    for (size_t i = 0; i < bone_names.capacity(); ++i) {
      bone_names.push_back(
          skeleton->bone_names()->Get(static_cast<uint32_t>(i))->str());
    }
    RigSystem::BoneIndices parent_indices(skeleton->bone_parents()->data(),
                                          skeleton->bone_parents()->size());
    RigSystem::BoneIndices shader_indices(
        model->shader_to_mesh_bones()->data(),
        model->shader_to_mesh_bones()->size());
    RigSystem::Pose inverse_bind_pose(
        reinterpret_cast<const mathfu::AffineTransform*>(
            skeleton->bone_transforms()->data()),
        skeleton->bone_transforms()->size());
    rig_system->SetRig(entity, parent_indices, inverse_bind_pose,
                       shader_indices, std::move(bone_names));
  }
}

void ModelAsset::BuildMesh() {
  const int lod = 0;
  DCHECK(model_def_->lods()->size() == 1)
      << "Only support single LODs for now.";
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
    index_bytes =
        reinterpret_cast<const uint8_t*>(model->indices16()->data());
    num_indices = model->indices16()->size();
  } else if (model->indices32()) {
    index_type = MeshData::kIndexU32;
    index_bytes =
        reinterpret_cast<const uint8_t*>(model->indices32()->data());
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
  mesh_data_ = MakeUnique<MeshData>(MeshData::kTriangles, vertex_format,
                                    std::move(vertices), index_type,
                                    std::move(indices), std::move(submeshes));
}

void ModelAsset::BuildMaterials() {
  const int lod = 0;
  DCHECK(model_def_->lods()->size() == 1)
      << "Only support single LODs for now.";
  const ModelInstanceDef* model = model_def_->lods()->Get(lod);
  const uint32_t num_materials =
      model->materials() ? model->materials()->size() : 0;

  materials_.reserve(num_materials);
  for (uint32_t i = 0; i < num_materials; ++i) {
    const MaterialDef* material_def = model->materials()->Get(i);
    materials_.emplace_back(BuildMaterialInfo(material_def, lod, i));
  }
}

MaterialInfo ModelAsset::BuildMaterialInfo(
    const MaterialDef* material_def, int lod, int submesh_index) {
  const ModelAssetMaterialDef* extra_material_def = nullptr;
  if (asset_def_ && asset_def_->materials()) {
    for (const auto mat : *asset_def_->materials()) {
      if (mat->lod() != -1 && mat->lod() != lod) {
        continue;
      } else if (mat->submesh() != -1 && mat->submesh() != submesh_index) {
        continue;
      } else {
        extra_material_def = mat;
        break;
      }
    }
  }

  VariantMap properties;
  if (material_def) {
    VariantMapFromFbVariantMap(material_def->properties(), &properties);
  }
  if (extra_material_def && extra_material_def->properties()) {
    VariantMapFromFbVariantMap(extra_material_def->properties(), &properties);
  }

  std::string shading_model;
  constexpr HashValue kShadingModel = ConstHash("ShadingModel");
  auto iter = properties.find(kShadingModel);
  if (iter != properties.end()) {
    shading_model = iter->second.ValueOr(std::string());
  }
  if (extra_material_def && extra_material_def->shading_model()) {
    shading_model = extra_material_def->shading_model()->str();
  }

  MaterialInfo material(shading_model);
  material.SetProperties(std::move(properties));

  if (material_def && material_def->textures()) {
    for (uint32_t i = 0; i < material_def->textures()->size(); ++i) {
      const MaterialTextureDef* texture_def =
          material_def->textures()->Get(i);
      if (texture_def->name()) {
        material.SetTexture(texture_def->usage(), texture_def->name()->c_str());
      }
    }
  }
  return material;
}

void ModelAsset::PrepareTextures() {
  if (model_def_->textures()) {
    for (auto texture : *model_def_->textures()) {
      CreateTexture(texture);
    }
  }
  if (asset_def_ && asset_def_->materials()) {
    for (auto material : *asset_def_->materials()) {
      const int lod = material->lod();
      if (material->textures() == nullptr) {
        continue;
      }

      for (auto texture_def : *material->textures()) {
        if (texture_def->texture() == nullptr) {
          continue;
        }

        const std::string name = CreateTexture(texture_def->texture());
        const MaterialTextureUsage usage = texture_def->usage();
        if (lod != -1 && lod < materials_.size()) {
          materials_[lod].SetTexture(usage, name);
        } else {
          for (auto& m : materials_) {
            m.SetTexture(usage, name);
          }
        }
      }
    }
  }
}

std::string ModelAsset::CreateTexture(const TextureDef* texture_def) {
  auto* texture_factory = registry_->Get<TextureFactory>();
  if (texture_factory == nullptr) {
    LOG(DFATAL) << "Missing texture factory.";
    return "";
  }
  if (!texture_def->name()) {
    LOG(DFATAL) << "Texture must have a name.";
    return "";
  }

  TextureFactory::CreateParams params;
  params.generate_mipmaps = texture_def->generate_mipmaps();
  params.premultiply_alpha = texture_def->premultiply_alpha();
  params.min_filter = texture_def->min_filter();
  params.mag_filter = texture_def->mag_filter();
  params.wrap_s = texture_def->wrap_s();
  params.wrap_t = texture_def->wrap_t();

  TexturePtr texture;
  if (texture_def->data() && texture_def->data()->data()) {
    const uint8_t* bytes = texture_def->data()->data();
    const size_t num_bytes = texture_def->data()->size();
    ImageData image = DecodeImage(bytes, num_bytes, kDecodeImage_None);
    texture = texture_factory->CreateTexture(std::move(image), params);
  } else if (texture_def->file()) {
    const char* filename = texture_def->file()->c_str();
    texture = texture_factory->LoadTexture(filename, params);
  }

  std::string texture_name;
  if (texture) {
    texture_name = texture_def->name()->str();
    const HashValue key = Hash(texture_name);
    texture_factory->CacheTexture(key, texture);
    textures_.emplace_back(key);
  }
  return texture_name;
}

}  // namespace lull
