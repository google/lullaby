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

#ifndef LULLABY_CONTRIB_PLANAR_GRAB_PLANAR_GRAB_EVENTS_H_
#define LULLABY_CONTRIB_PLANAR_GRAB_PLANAR_GRAB_EVENTS_H_

#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"
#include "mathfu/constants.h"

namespace lull {

struct PlanarGrabEvent {
  PlanarGrabEvent() {}
  PlanarGrabEvent(Entity e, const mathfu::vec3& location)
      : entity(e), location(location) {}
  // The entity being grabbed.
  Entity entity = kNullEntity;
  // Location of the grab in local coordinates of the entity.
  mathfu::vec3 location = mathfu::kZeros3f;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&location, ConstHash("location"));
  }
};

struct PlanarGrabReleasedEvent {
  PlanarGrabReleasedEvent() {}
  explicit PlanarGrabReleasedEvent(Entity e) : entity(e) {}
  // The entity being released.
  Entity entity = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PlanarGrabEvent);
LULLABY_SETUP_TYPEID(lull::PlanarGrabReleasedEvent);

#endif  // LULLABY_CONTRIB_PLANAR_GRAB_PLANAR_GRAB_EVENTS_H_
