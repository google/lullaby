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

#ifndef LULLABY_EVENTS_ANIMATION_EVENTS_H_
#define LULLABY_EVENTS_ANIMATION_EVENTS_H_

#include "lullaby/base/entity.h"
#include "lullaby/base/typeid.h"

namespace lull {

using AnimationId = unsigned int;
static const AnimationId kNullAnimation = 0;

// The reason a specific animation ended.
enum class AnimationCompletionReason { kCompleted, kInterrupted, kCancelled };

struct AnimationCompleteEvent {
  AnimationCompleteEvent() {}
  AnimationCompleteEvent(
      Entity e, AnimationId id,
      AnimationCompletionReason r = AnimationCompletionReason::kCompleted)
      : target(e), id(id), reason(r) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&id, Hash("id"));
    int r = static_cast<int>(reason);
    archive(&r, Hash("reason"));
  }

  Entity target = kNullEntity;
  AnimationId id = kNullAnimation;
  AnimationCompletionReason reason = AnimationCompletionReason::kCompleted;
};

// Invokes AnimationSystem::CancelAllAnimations(entity)
struct CancelAllAnimationsEvent {
  CancelAllAnimationsEvent() {}
  explicit CancelAllAnimationsEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AnimationCompleteEvent);
LULLABY_SETUP_TYPEID(lull::CancelAllAnimationsEvent);

#endif  // LULLABY_EVENTS_ANIMATION_EVENTS_H_
