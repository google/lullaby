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

#ifndef LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_RETICLE_PROVIDER_H_
#define LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_RETICLE_PROVIDER_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/reticle/reticle_provider.h"
#include "lullaby/systems/reticle/reticle_system.h"
#include "lullaby/util/math.h"

namespace lull {

// A simple implementation of ReticleProvider that relies on the ReticleSystem.
class ReticleSystemReticleProvider : public ReticleProvider {
 public:
  explicit ReticleSystemReticleProvider(ReticleSystem* reticle_system)
      : reticle_system_(reticle_system) {}

  Entity GetTarget() const override {
    if (reticle_system_) {
      return reticle_system_->GetTarget();
    }
    return kNullEntity;
  }

  Entity GetReticleEntity() const override {
    if (reticle_system_) {
      return reticle_system_->GetReticle();
    }
    return kNullEntity;
  }

  Ray GetCollisionRay() const override {
    if (reticle_system_) {
      return reticle_system_->GetCollisionRay();
    }
    return Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f);
  }

  InputManager::DeviceType GetActiveDevice() const override {
    if (reticle_system_) {
      return reticle_system_->GetActiveDevice();
    }
    return InputManager::kHmd;
  }

 private:
  ReticleSystem* reticle_system_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_RETICLE_PROVIDER_H_
