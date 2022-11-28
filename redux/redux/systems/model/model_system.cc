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

#include "redux/engines/render/mesh_factory.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/codecs/decode_image.h"
#include "redux/systems/physics/physics_system.h"
#include "redux/systems/render/render_system.h"
#include "redux/systems/rig/rig_system.h"

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
  if (entity == kNullEntity) {
    return;
  }

  const HashValue key = Hash(def.uri);

  LoadModel(def.uri);
  auto model = models_.Find(key);
  if (model == nullptr) {
    CHECK(false) << "Unable to load model.";
    return;
  }

  EntitySetupInfo setup;
  setup.entity = entity;
  setup.model_id = key;
  setup.render_scene = ConstHash("default");

  if (model->IsReady()) {
    FinalizeEntity(setup);
  } else {
    auto* render_system = registry_->Get<RenderSystem>();
    if (render_system) {
      // We want to create the RenderComponent with an empty mesh early to make
      // sure that IsReadyToRender doesn't return true before we're ready.
      render_system->AddToScene(entity, setup.render_scene);
      render_system->SetMesh(entity, empty_mesh_);
    }
    pending_entities_[key].push_back(setup);
  }
}

void ModelSystem::LoadModel(std::string_view uri) {
  const HashValue key = Hash(uri);
  models_.Create(key, [this, uri, key]() {
    auto model = std::make_shared<ModelAsset>();
    auto on_load = [=](AssetLoader::StatusOrData& asset) {
      auto data = std::make_shared<DataContainer>(std::move(*asset));
      std::function<ImageData(ImageData)> decoder = [](ImageData image) {
        DataContainer data =
            DataContainer::WrapData(image.GetData(), image.GetNumBytes());
        return DecodeImage(data, {});
      };
      model->OnLoad(std::move(data), decoder);
    };
    auto on_finalize = [=](AssetLoader::StatusOrData& asset) {
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

  if (mesh_factory) {
    instance.mesh = mesh_factory->CreateMesh(asset.GetMeshData());
  }

  if (texture_factory) {
    for (const MaterialData& material : asset.GetMaterialData()) {
      for (const auto& texture : material.textures) {
        const std::string_view name = texture.texture;
        const auto* info = asset.GetTextureData(name);
        if (info) {
          TexturePtr texture = nullptr;
          const HashValue key = Hash(name);

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
  if (setup.distinct) {
    instance = GenerateModelInstance(*model);
  } else {
    auto iter = instances_.find(setup.model_id);
    if (iter == instances_.end()) {
      instance = GenerateModelInstance(*model);
      instances_[setup.model_id] = instance;
    } else {
      instance = iter->second;
    }
  }

  auto* rig_system = registry_->Get<RigSystem>();
  auto* render_system = registry_->Get<RenderSystem>();
  auto* physics_system = registry_->Get<PhysicsSystem>();

  if (rig_system) {
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
    for (const MaterialData& material : model->GetMaterialData()) {
      for (const auto& property : material.properties) {
        const HashValue key = property.first;
        const TypeId type = property.second.GetTypeId();
        switch (type) {
          case GetTypeId<int>(): {
            const int value = property.second.ValueOr(0);
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<float>(): {
            const float value = property.second.ValueOr(0.f);
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<vec2i>(): {
            const auto value = property.second.ValueOr(vec2i());
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<vec3i>(): {
            const auto value = property.second.ValueOr(vec3i());
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<vec4i>(): {
            const auto value = property.second.ValueOr(vec4i());
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<vec2>(): {
            const auto value = property.second.ValueOr(vec2());
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<vec3>(): {
            const auto value = property.second.ValueOr(vec3());
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          case GetTypeId<vec4>(): {
            const auto value = property.second.ValueOr(vec4());
            render_system->SetMaterialProperty(setup.entity, key, value);
            break;
          }
          default:
            // LOG(FATAL) << "Unknown type: " << type;
            break;
        }
      }
      for (const auto& sampler : material.textures) {
        auto texture = instance.textures[Hash(sampler.texture)];
        render_system->SetTexture(setup.entity, sampler.usage, texture);
      }
      render_system->SetInverseBindPose(setup.entity,
                                        model->GetInverseBindPose());
      render_system->SetBoneShaderIndices(setup.entity,
                                          model->GetShaderBoneIndices());
      render_system->SetShadingModel(setup.entity, material.shading_model);
    }
  }
}

}  // namespace redux
