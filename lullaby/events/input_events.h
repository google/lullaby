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

#ifndef LULLABY_EVENTS_INPUT_EVENTS_H_
#define LULLABY_EVENTS_INPUT_EVENTS_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"
#include "mathfu/constants.h"

namespace lull {

struct StartHoverEvent {
  StartHoverEvent() {}
  explicit StartHoverEvent(Entity e) : target(e) {}
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }
};

struct StopHoverEvent {
  StopHoverEvent() {}
  explicit StopHoverEvent(Entity e) : target(e) {}
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }
};

struct ClickEvent {
  ClickEvent() {}
  ClickEvent(Entity e, const mathfu::vec3& location)
      : target(e), location(location) {}
  Entity target = kNullEntity;
  // Location of the click in local coordinates of the entity.
  mathfu::vec3 location = mathfu::kZeros3f;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }
};

struct ClickReleasedEvent {
  ClickReleasedEvent() {}
  explicit ClickReleasedEvent(Entity pressed_entity, Entity released_entity)
      : pressed_entity(pressed_entity), released_entity(released_entity) {}

  // The original entity targeted by the input controller, as the user
  // initiates the button press.
  Entity pressed_entity = kNullEntity;

  // The current entity targeted by the input controller, as the user
  // releases the input button press.
  Entity released_entity = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&pressed_entity, Hash("pressed_entity"));
    archive(&released_entity, Hash("released_entity"));
  }
};

struct ClickPressedAndReleasedEvent {
  ClickPressedAndReleasedEvent() {}
  explicit ClickPressedAndReleasedEvent(Entity e) : target(e) {}
  ClickPressedAndReleasedEvent(Entity e, int64_t ms)
      : target(e), duration(ms) {}
  Entity target = kNullEntity;
  int64_t duration = -1;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&duration, Hash("duration"));
  }
};

struct CollisionExitEvent {
  CollisionExitEvent() {}
  explicit CollisionExitEvent(Entity e) : target(e) {}
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }
};

// Sent when Primary button is pressed down.
struct PrimaryButtonPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is released in less than 500 ms.
struct PrimaryButtonClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is held for more than 500 ms.
struct PrimaryButtonLongPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is released after being held for more than 500 ms.
struct PrimaryButtonLongClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is released.
struct PrimaryButtonRelease {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is pressed down.
struct SecondaryButtonPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is released in less than 500 ms.
struct SecondaryButtonClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is held for more than 500 ms.
struct SecondaryButtonLongPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is released after being held for more than 500 ms.
struct SecondaryButtonLongClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};


// Sent when Secondary button is released.
struct SecondaryButtonRelease {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is pressed down.
struct SystemButtonPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is released in less than 500 ms.
struct SystemButtonClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is held for more than 500 ms.
struct SystemButtonLongPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is released after being held for more than 500 ms.
struct SystemButtonLongClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};


// Sent when System button is released.
struct SystemButtonRelease {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

struct GlobalRecenteredEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StartHoverEvent);
LULLABY_SETUP_TYPEID(lull::StopHoverEvent);
LULLABY_SETUP_TYPEID(lull::ClickEvent);
LULLABY_SETUP_TYPEID(lull::ClickPressedAndReleasedEvent);
LULLABY_SETUP_TYPEID(lull::ClickReleasedEvent);
LULLABY_SETUP_TYPEID(lull::CollisionExitEvent);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonPress);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonClick);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonLongPress);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonLongClick);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonRelease);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonPress);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonClick);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonLongPress);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonLongClick);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonRelease);
LULLABY_SETUP_TYPEID(lull::SystemButtonPress);
LULLABY_SETUP_TYPEID(lull::SystemButtonClick);
LULLABY_SETUP_TYPEID(lull::SystemButtonLongPress);
LULLABY_SETUP_TYPEID(lull::SystemButtonLongClick);
LULLABY_SETUP_TYPEID(lull::SystemButtonRelease);
LULLABY_SETUP_TYPEID(lull::GlobalRecenteredEvent);

#endif  // LULLABY_EVENTS_INPUT_EVENTS_H_
