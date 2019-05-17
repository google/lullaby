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

#ifndef LULLABY_EVENTS_FADE_EVENTS_H_
#define LULLABY_EVENTS_FADE_EVENTS_H_

#include "lullaby/util/entity.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Invokes FadeSystem::FadeIn(entity, time_ms)
struct FadeInEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&time_ms, ConstHash("time_ms"));
  }

  Entity entity = kNullEntity;
  float time_ms = 0.f;
};

// Invokes FadeSystem::FadeOut(entity, time_ms)
struct FadeOutEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&time_ms, ConstHash("time_ms"));
  }

  Entity entity = kNullEntity;
  float time_ms = 0.f;
};

struct FadeInCompleteEvent {
  FadeInCompleteEvent() {}
  FadeInCompleteEvent(Entity e, bool interrupted)
      : target(e), interrupted(interrupted) {}
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, ConstHash("target"));
    archive(&interrupted, ConstHash("interrupted"));
  }

  Entity target = kNullEntity;

  // Set to true only if the FadeIn was interrupted by a FadeOut.
  bool interrupted = false;
};

struct FadeOutCompleteEvent {
  FadeOutCompleteEvent() {}
  FadeOutCompleteEvent(Entity e, bool interrupted)
      : target(e), interrupted(interrupted) {}
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, ConstHash("target"));
    archive(&interrupted, ConstHash("interrupted"));
  }

  Entity target = kNullEntity;

  // Set to true only if the FadeOut was interrupted by a FadeIn.
  bool interrupted = false;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FadeInCompleteEvent);
LULLABY_SETUP_TYPEID(lull::FadeInEvent);
LULLABY_SETUP_TYPEID(lull::FadeOutCompleteEvent);
LULLABY_SETUP_TYPEID(lull::FadeOutEvent);

#endif  // LULLABY_EVENTS_FADE_EVENTS_H_
