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

#ifndef LULLABY_UTIL_RETICLE_PROVIDER_H_
#define LULLABY_UTIL_RETICLE_PROVIDER_H_

#include "lullaby/base/entity.h"
#include "lullaby/base/input_manager.h"

namespace lull {

// Provides reticle data such as which entity is being hovered or what device is
// currently active. This allows other entities to depend on the reticle without
// forcing apps to use a specific reticle implementation.
class ReticleProvider {
 public:
  virtual ~ReticleProvider() {}

  virtual Entity GetTarget() const = 0;

  virtual Entity GetReticleEntity() const = 0;

  virtual InputManager::DeviceType GetActiveDevice() const = 0;

  virtual float GetReticleErgoAngleOffset() const { return -0.26f; }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReticleProvider);

#endif  // LULLABY_UTIL_RETICLE_PROVIDER_H_
