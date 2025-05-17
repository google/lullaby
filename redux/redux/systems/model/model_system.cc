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

#include "redux/systems/model/model_system.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "redux/engines/physics/physics_engine.h"
#include "redux/engines/render/render_engine.h"
#include "redux/engines/render/mesh_factory.h"
#include "redux/engines/render/texture.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"
#include "redux/systems/model/model_asset.h"
#include "redux/systems/model/model_asset_factory.h"
#include "redux/systems/model/model_def_generated.h"
#include "redux/systems/physics/physics_system.h"
#include "redux/systems/render/render_system.h"
#include "redux/systems/rig/rig_system.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

ModelSystem::ModelSystem(Registry* registry)
    : System(registry), models_(ResourceCacheMode::kCacheFullyOnCreate) {
  RegisterDef(&ModelSystem::AddFromDef);
  registry->RegisterDependency<RenderSystem>(this, true);
}

void ModelSystem::OnRegistryInitialize() {
  auto* mesh_factory = registry_->Get<MeshFactory>();
  if (mesh_factory) {
    empty_mesh_ = mesh_factory->EmptyMesh();
  }
}

void ModelSystem::AddFromDef(Entity entity, const ModelDef& def) {
  SetModel(entity, def.uri);
}

void ModelSystem::SetModel(Entity entity, std::string_view uri) {
  if (entity == kNullEntity) {
    return;
  }

  const HashValue key = Hash(uri);

  LoadModel(uri);
  auto model = models_.Find(key);
  if (model == nullptr) {
    CHECK(false) << "Unable to load model.";
    return;
  }

  EntitySetupInfo setup;
  setup.entity = entity;
  setup.model_id = key;

  if (model->IsReady()) {
    FinalizeEntity(setup);
  } else {
    auto* render_system = registry_->Get<RenderSystem>();
    if (render_system) {
      // We want to create the RenderComponent with an empty mesh early to make
      // sure that IsReadyToRender doesn't return true before we're ready.
      render_system->AddToScene(entity, ConstHash("default"));
      render_system->SetMesh(entity, empty_mesh_);
    }
    pending_entities_[key].push_back(setup);
  }
}

void ModelSystem::LoadModel(std::string_view uri) {
  const HashValue key = Hash(uri);
  models_.Create(key, [this, uri, key]() {
    ModelAssetPtr model = ModelAssetFactory::CreateModelAsset(registry_, uri);
    CHECK(model) << "Unable to create model, unknown type: " << uri;

    auto on_load = [=](AssetLoader::StatusOrData& asset) {
      auto data = std::make_shared<DataContainer>(std::move(*asset));
      model->OnLoad(std::move(data));
    };

    auto on_finalize = [=, this](AssetLoader::StatusOrData& asset) {
      model->OnFinalize();
      FinalizeModel(key);
    };

    AssetLoader* asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync(uri, on_load, on_finalize);
    return model;
  });
}

void ModelSystem::ReleaseModel(HashValue key) {
  models_.Release(key);

  auto iter = instances_.find(key);
  if (iter != instances_.end()) {
    auto* texture_factory = registry_->Get<TextureFactory>();
    if (texture_factory) {
      for (auto& texture_iter : iter->second.textures) {
        texture_factory->ReleaseTexture(texture_iter.first);
      }
    }
    instances_.erase(iter);
  }
}

void ModelSystem::FinalizeModel(HashValue key) {
  auto iter = pending_entities_.find(key);
  if (iter != pending_entities_.end()) {
    for (const EntitySetupInfo& setup : iter->second) {
      FinalizeEntity(setup);
    }
    pending_entities_.erase(iter);
  }
}

ModelSystem::ModelInstance ModelSystem::GenerateModelInstance(
    const ModelAsset& asset) {
  ModelInstance instance;

  auto* mesh_factory = registry_->Get<RenderEngine>()->GetMeshFactory();
  auto* physics_engine = registry_->Get<PhysicsEngine>();
  auto* texture_factory = registry_->Get<RenderEngine>()->GetTextureFactory();

  const auto meshes = asset.GetMeshData();

  // Compute the bounding box.
  for (const std::shared_ptr<MeshData>& mesh : meshes) {
    const auto& box = mesh->GetBoundingBox();
    instance.bounding_box = instance.bounding_box.Included(box.min);
    instance.bounding_box = instance.bounding_box.Included(box.max);
  }

  if (mesh_factory) {
    // Note: MeshFactory::CreateMesh will call std::move on all meshes, so we
    // need to wrap the shared_ptrs into a movable MeshData object.
    std::vector<MeshData> tmp_meshes;
    tmp_meshes.reserve(meshes.size());
    for (const std::shared_ptr<MeshData>& mesh : meshes) {
      tmp_meshes.push_back(MeshData::WrapDataInSharedPtr(mesh));
    }
    instance.mesh = mesh_factory->CreateMesh(
        absl::MakeSpan(tmp_meshes.data(), tmp_meshes.size()));
  }

  if (texture_factory) {
    for (const MaterialData& material : asset.GetMaterialData()) {
      for (const auto& texture_data : material.textures) {
        const std::string_view name = texture_data.name;
        const auto* info = asset.GetTextureData(name);
        if (info) {
          const HashValue key = Hash(name);
          if (instance.textures.contains(key)) {
            continue;
          }

          TexturePtr texture = nullptr;
          if (info->image) {
            texture = texture_factory->CreateTexture(
                key, ImageData::Rebind(info->image), info->params);
          } else if (!info->uri.empty()) {
            texture = texture_factory->LoadTexture(info->uri, info->params);
          } else {
            LOG(FATAL) << "Texture must have either a filename or image data.";
          }
          instance.textures[key] = texture;
        }
      }
    }
  }

  if (physics_engine && asset.GetCollisionData()) {
    instance.collision_shape =
        physics_engine->CreateShape(asset.GetCollisionData());
  }

  return instance;
}

void ModelSystem::FinalizeEntity(const EntitySetupInfo& setup) {
  auto model = models_.Find(setup.model_id);
  CHECK(model && model->IsReady()) << "Model is not ready.";

  ModelInstance instance;
  auto iter = instances_.find(setup.model_id);
  if (iter == instances_.end()) {
    instance = GenerateModelInstance(*model);
    instances_[setup.model_id] = instance;
  } else {
    instance = iter->second;
  }

  auto* rig_system = registry_->Get<RigSystem>();
  auto* render_system = registry_->Get<RenderSystem>();
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  if (transform_system) {
    transform_system->SetBox(setup.entity, instance.bounding_box);
  }

  if (rig_system && !model->GetBoneNames().empty()) {
    rig_system->SetSkeleton(setup.entity, model->GetBoneNames(),
                            model->GetParentBoneIndices());
  }

  render_system->SetMesh(setup.entity, instance.mesh);

  if (physics_system) {
    physics_system->SetShape(setup.entity, instance.collision_shape);
  }

  // Set the material at the very end after all the other properties are done.
  // This way shader autogeneration can detect what features are necessary.
  if (render_system) {
    const auto& materials = model->GetMaterialData();

    // Ensure that each material corresponds to a mesh part.
    CHECK_EQ(materials.size(), instance.mesh->GetNumParts());

    for (size_t i = 0; i < materials.size(); ++i) {
      const MaterialData& material = materials[i];
      const HashValue part = instance.mesh->GetPartName(i);

      for (const auto& property : material.properties) {
        const HashValue key = property.first;
        const TypeId type = property.second.GetTypeId();
        switch (type) {
          case GetTypeId<bool>(): {
            const bool value = property.second.ValueOr(false);
            if (value) {
              render_system->EnableShadingFeature(setup.entity, part, key);
            } else {
              render_system->DisableShadingFeature(setup.entity, part, key);
            }
            break;
          }
          case GetTypeId<int>(): {
            const int value = property.second.ValueOr(0);
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<float>(): {
            const float value = property.second.ValueOr(0.f);
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<vec2i>(): {
            const auto value = property.second.ValueOr(vec2i());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<vec3i>(): {
            const auto value = property.second.ValueOr(vec3i());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<vec4i>(): {
            const auto value = property.second.ValueOr(vec4i());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<vec2>(): {
            const auto value = property.second.ValueOr(vec2());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<vec3>(): {
            const auto value = property.second.ValueOr(vec3());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<vec4>(): {
            const auto value = property.second.ValueOr(vec4());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<mat3>(): {
            const auto value = property.second.ValueOr(mat3());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<mat4>(): {
            const auto value = property.second.ValueOr(mat4());
            render_system->SetMaterialProperty(setup.entity, part, key, value);
            break;
          }
          case GetTypeId<std::string>(): {
            const auto value = property.second.ValueOr<std::string>("");
            LOG(ERROR) << "Ignoring string property: " << value;
            break;
          }
          default:
            LOG(FATAL) << "Unknown type: " << type;
            break;
        }
      }
      for (const auto& texture_data : material.textures) {
        auto texture = instance.textures[Hash(texture_data.name)];
        render_system->SetTexture(setup.entity, part, texture_data.usage,
                                  texture);
      }
      render_system->SetInverseBindPose(setup.entity,
                                        model->GetInverseBindPose());
      render_system->SetBoneShaderIndices(setup.entity,
                                          model->GetShaderBoneIndices());
      render_system->SetShadingModel(setup.entity, part,
                                     material.shading_model);
    }
  }
}

}  // namespace redux
