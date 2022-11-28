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

#include "redux/engines/render/filament/filament_light.h"

#include "filament/Color.h"
#include "filament/LightManager.h"
#include "filament/TransformManager.h"
#include "utils/EntityManager.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/engines/render/filament/filament_utils.h"

namespace redux {

FilamentLight::FilamentLight(Registry* registry, Type type) : type_(type) {
  fengine_ = GetFilamentEngine(registry);
  switch (type_) {
    case Light::Directional: {
      CreateLightEntity(filament::LightManager::Type::DIRECTIONAL);
      break;
    }
    case Light::Spot: {
      CreateLightEntity(filament::LightManager::Type::SPOT);
      break;
    }
    case Light::Point: {
      CreateLightEntity(filament::LightManager::Type::POINT);
      break;
    }
  }
}

FilamentLight::~FilamentLight() {
  for (auto* scene : scenes_) {
    scene->remove(fentity_);
  }
  scenes_.clear();

  fengine_->getLightManager().destroy(fentity_);
  fengine_->getTransformManager().destroy(fentity_);
  utils::EntityManager::get().destroy(fentity_);
}

void FilamentLight::CreateLightEntity(filament::LightManager::Type type) {
  fentity_ = utils::EntityManager::get().create();
  filament::LightManager::Builder builder(type);
  builder.direction({0, 0, -1});  // point lights in same direction as camera
  builder.build(*fengine_, fentity_);
}

void FilamentLight::Enable() {
  if (!visible_) {
    visible_ = true;
    for (filament::Scene* scene : scenes_) {
      scene->addEntity(fentity_);
    }
  }
}

void FilamentLight::Disable() {
  if (visible_) {
    visible_ = false;
    for (filament::Scene* scene : scenes_) {
      scene->remove(fentity_);
    }
  }
}

bool FilamentLight::IsEnabled() const { return visible_; }

void FilamentLight::SetTransform(const mat4& transform) {
  if (fentity_) {
    auto ti = fengine_->getTransformManager().getInstance(fentity_);
    fengine_->getTransformManager().setTransform(ti, ToFilament(transform));
  }
}

void FilamentLight::SetColor(const Color4f& color) {
  auto& lm = fengine_->getLightManager();
  auto li = lm.getInstance(fentity_);
  filament::LinearColor fcolor(color.r, color.g, color.b);
  lm.setColor(li, fcolor);
}

void FilamentLight::SetIntensity(float intensity) {
  auto& lm = fengine_->getLightManager();
  auto li = lm.getInstance(fentity_);
  lm.setIntensity(li, intensity);
}

void FilamentLight::SetFalloffDistance(float distance) {
  auto& lm = fengine_->getLightManager();
  auto li = lm.getInstance(fentity_);
  lm.setFalloff(li, distance);
}

void FilamentLight::SetSpotLightConeAngles(float inner, float outer) {
  auto& lm = fengine_->getLightManager();
  auto li = lm.getInstance(fentity_);
  lm.setSpotLightCone(li, inner, outer);
}

void FilamentLight::AddToScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  if (!scenes_.contains(fscene)) {
    if (visible_) {
      fscene->addEntity(fentity_);
    }
    scenes_.emplace(fscene);
  }
}

void FilamentLight::RemoveFromScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  auto iter = scenes_.find(fscene);
  if (iter != scenes_.end()) {
    if (visible_) {
      fscene->remove(fentity_);
    }
    scenes_.erase(iter);
  }
}

}  // namespace redux
