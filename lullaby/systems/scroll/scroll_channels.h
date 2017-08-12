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

#ifndef LULLABY_SYSTEMS_SCROLL_SCROLL_CHANNELS_H_
#define LULLABY_SYSTEMS_SCROLL_SCROLL_CHANNELS_H_

#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/util/registry.h"
#include "motive/motivator.h"

namespace lull {

// Channel for animating scroll views.
class ScrollViewOffsetChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

 private:
  ScrollViewOffsetChannel(Registry* registry, size_t pool_size);

  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_SCROLL_SCROLL_CHANNELS_H_
