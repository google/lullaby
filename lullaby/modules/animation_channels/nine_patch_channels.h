/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_ANIMATION_CHANNELS_NINE_PATCH_CHANNELS_H_
#define LULLABY_MODULES_ANIMATION_CHANNELS_NINE_PATCH_CHANNELS_H_

#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/util/registry.h"
#include "motive/motivator.h"

namespace lull {

class NinePatchSystem;

// Channel for animating the size of a nine_patch entity.
class NinePatchSizeChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  NinePatchSizeChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  NinePatchSystem* nine_patch_system_;
};

// Channel for animating the original size (and thus pixel density) of
// nine_patch entities.
class NinePatchOriginalSizeChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  NinePatchOriginalSizeChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  NinePatchSystem* nine_patch_system_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_ANIMATION_CHANNELS_NINE_PATCH_CHANNELS_H_
