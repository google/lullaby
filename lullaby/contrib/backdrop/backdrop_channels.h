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

#ifndef LULLABY_CONTRIB_BACKDROP_BACKDROP_CHANNELS_H_
#define LULLABY_CONTRIB_BACKDROP_BACKDROP_CHANNELS_H_

#include "lullaby/util/registry.h"
#include "lullaby/systems/animation/animation_channel.h"

namespace lull {

/// Channel for animating backdrop aabbs.
class BackdropAabbChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

 private:
  BackdropAabbChannel(Registry* registry, size_t pool_size);

  bool Get(Entity entity, float* values, size_t len) const override;
  void Set(Entity entity, const float* values, size_t len) override;
};

}  // namespace lull

#endif  // LULLABY_CONTRIB_BACKDROP_BACKDROP_CHANNELS_H_
