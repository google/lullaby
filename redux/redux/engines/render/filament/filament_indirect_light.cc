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

#include "redux/engines/render/filament/filament_indirect_light.h"
#include <memory>

#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "filament/IndirectLight.h"
#include "filament/LightManager.h"
#include "filament/Scene.h"
#include "math/vec3.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/texture.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/math/matrix.h"

namespace redux {

inline std::shared_ptr<FilamentTexture> GetFilamentTexture(
    const TexturePtr& texture) {
  return std::static_pointer_cast<FilamentTexture>(texture);
}

FilamentIndirectLight::FilamentIndirectLight(
    Registry* registry, const TexturePtr& reflection,
    const TexturePtr& irradiance, absl::Span<const float> spherical_harmonics)
    : reflection_(reflection), irradiance_(irradiance) {
  CHECK(reflection_);
  fengine_ = GetFilamentEngine(registry);

  if (!spherical_harmonics.empty()) {
    CHECK_EQ(spherical_harmonics.size(), 27);
    const float* sh = spherical_harmonics.data();
    spherical_harmonics_ = {
        filament::math::float3{sh[0], sh[1], sh[2]},
        filament::math::float3{sh[3], sh[4], sh[5]},
        filament::math::float3{sh[6], sh[7], sh[8]},
        filament::math::float3{sh[9], sh[10], sh[11]},
        filament::math::float3{sh[12], sh[13], sh[14]},
        filament::math::float3{sh[15], sh[16], sh[17]},
        filament::math::float3{sh[18], sh[19], sh[20]},
        filament::math::float3{sh[21], sh[22], sh[23]},
        filament::math::float3{sh[24], sh[25], sh[26]},
    };
  }

  auto reflection_ptr = GetFilamentTexture(reflection_);
  if (reflection_ptr) {
    reflection_ptr->OnReady([this]() { OnReady(); });
  }

  auto irradiance_ptr = GetFilamentTexture(irradiance_);
  if (irradiance_ptr) {
    irradiance_ptr->OnReady([this]() { OnReady(); });
  }
}

FilamentIndirectLight::~FilamentIndirectLight() {
  for (auto* scene : scenes_) {
    scene->setIndirectLight(nullptr);
  }
  scenes_.clear();
  fengine_->destroy(fibl_);
}

void FilamentIndirectLight::Enable() {
  if (!visible_) {
    visible_ = true;
    for (filament::Scene* scene : scenes_) {
      scene->setIndirectLight(fibl_);
    }
  }
}

void FilamentIndirectLight::Disable() {
  if (visible_) {
    visible_ = false;
    for (filament::Scene* scene : scenes_) {
      scene->setIndirectLight(nullptr);
    }
  }
}

bool FilamentIndirectLight::IsEnabled() const { return visible_; }

void FilamentIndirectLight::SetTransform(const mat4& transform) {
  transform_ = transform;
  if (fibl_) {
    fibl_->setRotation(ToFilament(transform).upperLeft());
  }
}

void FilamentIndirectLight::SetIntensity(float intensity) {
  intensity_ = intensity;
  if (fibl_) {
    fibl_->setIntensity(intensity);
  }
}

void FilamentIndirectLight::AddToScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  if (!scenes_.contains(fscene)) {
    if (visible_) {
      fscene->setIndirectLight(fibl_);
    }
    scenes_.emplace(fscene);
  }
}

void FilamentIndirectLight::RemoveFromScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  auto iter = scenes_.find(fscene);
  if (iter != scenes_.end()) {
    if (visible_) {
      fscene->setIndirectLight(nullptr);
    }
    scenes_.erase(iter);
  }
}

void FilamentIndirectLight::OnReady() {
  if (fibl_) {
    return;
  }

  auto reflection_ptr = GetFilamentTexture(reflection_);
  if (!reflection_ptr->IsReady()) {
    return;
  }

  auto irradiance_ptr = GetFilamentTexture(irradiance_);
  if (irradiance_ptr && !irradiance_ptr->IsReady()) {
    return;
  }

  filament::IndirectLight::Builder builder;
  builder.reflections(reflection_ptr->GetFilamentTexture());

  if (irradiance_ptr) {
    builder.irradiance(irradiance_ptr->GetFilamentTexture());
  } else if (!spherical_harmonics_.empty()) {
    builder.irradiance(3, spherical_harmonics_.data());
  } else if (!reflection_ptr->GetSphericalHarmonics().empty()) {
    builder.irradiance(3, reflection_ptr->GetSphericalHarmonics().data());
  } else {
    LOG(FATAL) << "No irradiance/spherical harmonics provided";
  }

  builder.intensity(intensity_);
  builder.rotation(ToFilament(transform_).upperLeft());
  fibl_ = builder.build(*fengine_);

  if (visible_) {
    for (filament::Scene* scene : scenes_) {
      scene->setIndirectLight(fibl_);
    }
  }
}

}  // namespace redux
