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

#include "redux/systems/light/light_system.h"

#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/choreographer.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

LightSystem::LightSystem(Registry* registry) : System(registry) {
  RegisterDef(&LightSystem::SetFromDirectionalLightDef);
  RegisterDef(&LightSystem::SetFromSpotLightDef);
  RegisterDef(&LightSystem::SetFromPointLightDef);
  RegisterDef(&LightSystem::SetFromEnvLightDef);
  RegisterDependency<TransformSystem>(this);
}

void LightSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<RenderEngine>();
  CHECK(engine_);

  auto choreo = registry_->Get<Choreographer>();
  choreo->Add<&LightSystem::PrepareToRender>(Choreographer::Stage::kRender)
      .Before<&RenderEngine::Render>();
}

void LightSystem::PrepareToRender() {
  auto transform_system = registry_->Get<TransformSystem>();
  for (const auto& iter : lights_) {
    const Transform transform = transform_system->GetTransform(iter.first);
    const mat4 matrix = TransformMatrix(transform);
    if (iter.second.light) {
      iter.second.light->SetTransform(matrix);
    } else if (iter.second.indirect_light) {
      iter.second.indirect_light->SetTransform(matrix);
    }
  }
}

void LightSystem::SetFromDirectionalLightDef(Entity entity,
                                             const DirectionalLightDef& def) {
  AddDirectionalLight(entity);
  SetColor(entity, Color4f(def.color.x, def.color.y, def.color.z, 1.f));
  SetIntensity(entity, def.intensity);
}

void LightSystem::SetFromSpotLightDef(Entity entity, const SpotLightDef& def) {
  AddSpotLight(entity);
  SetColor(entity, Color4f(def.color.x, def.color.y, def.color.z, 1.f));
  SetIntensity(entity, def.intensity);
  SetFalloffDistance(entity, def.falloff);
  SetSpotLightCone(entity, def.inner_cone_angle, def.outer_cone_angle);
}

void LightSystem::SetFromPointLightDef(Entity entity,
                                       const PointLightDef& def) {
  AddPointLight(entity);
  SetColor(entity, Color4f(def.color.x, def.color.y, def.color.z, 1.f));
  SetIntensity(entity, def.intensity);
  SetFalloffDistance(entity, def.falloff);
}

void LightSystem::SetFromEnvLightDef(Entity entity, const EnvLightDef& def) {
  auto texture_factory = registry_->Get<TextureFactory>();
  CHECK(texture_factory);

  TexturePtr reflections;
  if (!def.reflections.empty()) {
    reflections = texture_factory->LoadTexture(def.reflections, {});
  } else {
    reflections = texture_factory->DefaultEnvReflectionTexture();
  }
  CHECK(reflections);

  TexturePtr irradiance;
  if (!def.irradiance.empty()) {
    reflections = texture_factory->LoadTexture(def.reflections, {});
  }

  AddEnvironmentalLight(entity, reflections, irradiance);
}

void LightSystem::AddEnvironmentalLight(Entity entity,
                                        const TexturePtr& reflections,
                                        std::optional<HashValue> scene) {
  AddEnvironmentalLight(entity, reflections, nullptr, scene);
}

void LightSystem::AddEnvironmentalLight(Entity entity,
                                        const TexturePtr& reflections,
                                        const TexturePtr& irradiance,
                                        std::optional<HashValue> scene) {
  LightComponent& c = lights_[entity];
  CHECK(c.light == nullptr);
  CHECK(c.indirect_light == nullptr);

  c.indirect_light = engine_->CreateIndirectLight(reflections, irradiance);
  CHECK(c.indirect_light);

  RenderScenePtr scene_ptr;
  if (scene) {
    if (scene.has_value()) {
      scene_ptr = engine_->GetRenderScene(scene.value());
      CHECK(scene_ptr);
    }
  } else {
    scene_ptr = engine_->GetDefaultRenderScene();
    CHECK(scene_ptr);
  }
  if (scene_ptr) {
    scene_ptr->Add(*c.indirect_light);
  }
}

void LightSystem::AddDirectionalLight(Entity entity,
                                      std::optional<HashValue> scene) {
  CreateLight(entity, Light::Directional, scene);
}

void LightSystem::AddSpotLight(Entity entity, std::optional<HashValue> scene) {
  CreateLight(entity, Light::Spot, scene);
}

void LightSystem::AddPointLight(Entity entity, std::optional<HashValue> scene) {
  CreateLight(entity, Light::Point, scene);
}

void LightSystem::OnDestroy(Entity entity) { lights_.erase(entity); }

void LightSystem::AddToScene(Entity entity, HashValue scene) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }

  RenderScenePtr scene_ptr = engine_->GetRenderScene(scene);
  if (scene_ptr) {
    if (iter->second.light) {
      scene_ptr->Add(*iter->second.light);
    } else if (iter->second.indirect_light) {
      scene_ptr->Add(*iter->second.indirect_light);
    }
  }
}

void LightSystem::RemoveFromScene(Entity entity, HashValue scene) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }

  RenderScenePtr scene_ptr = engine_->GetRenderScene(scene);
  if (scene_ptr) {
    if (iter->second.light) {
      scene_ptr->Add(*iter->second.light);
    } else if (iter->second.indirect_light) {
      scene_ptr->Add(*iter->second.indirect_light);
    }
  }
}

void LightSystem::Hide(Entity entity) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }

  if (iter->second.light) {
    iter->second.light->Disable();
  } else if (iter->second.indirect_light) {
    iter->second.indirect_light->Disable();
  }
}

void LightSystem::Show(Entity entity) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }

  if (iter->second.light) {
    iter->second.light->Enable();
  } else if (iter->second.indirect_light) {
    iter->second.indirect_light->Enable();
  }
}

bool LightSystem::IsHidden(Entity entity) const {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return true;
  }

  if (iter->second.light) {
    return iter->second.light->IsEnabled() ? false : true;
  } else if (iter->second.indirect_light) {
    return iter->second.indirect_light->IsEnabled() ? false : true;
  } else {
    return true;
  }
}

void LightSystem::SetColor(Entity entity, const Color4f& color) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }
  if (iter->second.light) {
    iter->second.light->SetColor(color);
  }
}

void LightSystem::SetIntensity(Entity entity, float intensity) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }
  if (iter->second.light) {
    iter->second.light->SetIntensity(intensity);
  }
}

void LightSystem::SetFalloffDistance(Entity entity, float distance) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }
  if (iter->second.light) {
    iter->second.light->SetFalloffDistance(distance);
  }
}

void LightSystem::SetSpotLightCone(Entity entity, float inner_angle,
                                   float outer_angle) {
  auto iter = lights_.find(entity);
  if (iter == lights_.end()) {
    return;
  }
  if (iter->second.light) {
    iter->second.light->SetSpotLightConeAngles(inner_angle, outer_angle);
  }
}

void LightSystem::CreateLight(Entity entity, Light::Type type,
                              std::optional<HashValue> scene) {
  LightComponent& c = lights_[entity];
  CHECK(c.light == nullptr);
  CHECK(c.indirect_light == nullptr);

  c.light = engine_->CreateLight(type);
  CHECK(c.light);

  RenderScenePtr scene_ptr;
  if (scene) {
    if (scene.has_value()) {
      scene_ptr = engine_->GetRenderScene(scene.value());
      CHECK(scene_ptr);
    }
  } else {
    scene_ptr = engine_->GetDefaultRenderScene();
    CHECK(scene_ptr);
  }
  if (scene_ptr) {
    scene_ptr->Add(*c.light);
  }
}

}  // namespace redux
