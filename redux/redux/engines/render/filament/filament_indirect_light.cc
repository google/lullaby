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

#include "filament/Color.h"
#include "filament/IndirectLight.h"
#include "filament/LightManager.h"
#include "filament/TransformManager.h"
#include "utils/EntityManager.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/engines/render/filament/filament_utils.h"

namespace redux {

FilamentIndirectLight::FilamentIndirectLight(Registry* registry,
                                             const TexturePtr& reflection,
                                             const TexturePtr& irradiance)
    : reflection_(reflection), irradiance_(irradiance) {
  CHECK(reflection_);

  fengine_ = GetFilamentEngine(registry);
  filament::IndirectLight::Builder builder;
  FilamentTexture* reflections =
      static_cast<FilamentTexture*>(reflection_.get());
  builder.reflections(reflections->GetFilamentTexture());
  if (irradiance_) {
    FilamentTexture* irradiance =
        static_cast<FilamentTexture*>(irradiance_.get());
    builder.irradiance(irradiance->GetFilamentTexture());
  } else {
    // clang-format off
    std::array<filament::math::float3, 9> sh_coefficients = {
        filament::math::float3{ 0.592915142902302,  0.580783147865357,  0.564906236122309},  // L00, irradiance, pre-scaled base
        filament::math::float3{ 0.038230073440953,  0.040661612793765,  0.045912497583365},  // L1-1, irradiance, pre-scaled base
        filament::math::float3{-0.306182569332798, -0.298728189882871, -0.292527808646246},  // L10, irradiance, pre-scaled base
        filament::math::float3{-0.268674829827722, -0.258309969107310, -0.244936138194592},  // L11, irradiance, pre-scaled base
        filament::math::float3{ 0.055981897791156,  0.053190319920282,  0.047808414744011},  // L2-2, irradiance, pre-scaled base
        filament::math::float3{ 0.009835221123367,  0.006544190646597,  0.000350193519574},  // L2-1, irradiance, pre-scaled base
        filament::math::float3{ 0.017525154215762,  0.017508716588022,  0.018218263542429},  // L20, irradiance, pre-scaled base
        filament::math::float3{ 0.306912095635860,  0.292384283162994,  0.274657325943371},  // L21, irradiance, pre-scaled base
        filament::math::float3{ 0.055928224084081,  0.051564836176893,  0.044938623517990},  // L22, irradiance, pre-scaled base
    };
    // clang-format on
    builder.irradiance(3, sh_coefficients.data());
  }
  builder.intensity(30000);
  fibl_ = builder.build(*fengine_);
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
  if (fibl_) {
    fibl_->setRotation(ToFilament(transform).upperLeft());
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

}  // namespace redux
