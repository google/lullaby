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

#ifndef LULLABY_EVENTS_GRAB_EVENTS_H_
#define LULLABY_EVENTS_GRAB_EVENTS_H_

#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"

namespace lull {

/// Sent when a grab has been intentionally let go.
struct GrabReleasedEvent {
  GrabReleasedEvent() {}
  GrabReleasedEvent(Entity entity, const Sqt& final_sqt)
      : entity(entity), final_sqt(final_sqt) {}
  /// The entity being grabbed.
  Entity entity = kNullEntity;

  /// A valid position the entity should end at.  If GrabDef's snap_to_final was
  /// true for this entity, this will already be set as the entity's current
  /// sqt.  Otherwise, the entity will still be at the position the grab was
  /// released at and the app is responsible for animating it to a desired
  /// position.
  Sqt final_sqt;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&final_sqt, ConstHash("final_sqt"));
  }
};

/// Sent when a grab is forced to cancel (i.e. due to dragging something too far
/// outside of a valid position.
struct GrabCanceledEvent {
  GrabCanceledEvent() {}
  GrabCanceledEvent(Entity entity, const Sqt& starting_sqt)
      : entity(entity), starting_sqt(starting_sqt) {}
  Entity entity = kNullEntity;

  /// The sqt of the entity when the drag was started.  If GrabDef's
  /// snap_to_final was true for this entity, this will already be set as the
  /// entity's current sqt.  Otherwise, the entity will still be at the position
  /// the grab was released at and the app is responsible for animating it to a
  /// desired position.
  Sqt starting_sqt;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&starting_sqt, ConstHash("starting_sqt"));
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::GrabCanceledEvent);
LULLABY_SETUP_TYPEID(lull::GrabReleasedEvent);

#endif  // LULLABY_EVENTS_GRAB_EVENTS_H_
