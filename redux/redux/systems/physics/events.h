/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_SYSTEMS_PHYSICS_EVENTS_H_
#define REDUX_SYSTEMS_PHYSICS_EVENTS_H_

#include "redux/modules/base/hash.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/ecs/entity.h"

namespace redux {

// Event for when two Entities enter each other's collision volumes.
struct CollisionEnterEvent {
  Entity entity_a;
  Entity entity_b;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(entity_a, ConstHash("entity_a"));
    archive(entity_b, ConstHash("entity_b"));
  }
};

// Event for when two Entities exit each other's collision volumes.
struct CollisionExitEvent {
  Entity entity_a;
  Entity entity_b;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(entity_a, ConstHash("entity_a"));
    archive(entity_b, ConstHash("entity_b"));
  }
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::CollisionEnterEvent);
REDUX_SETUP_TYPEID(redux::CollisionExitEvent);

#endif  // REDUX_SYSTEMS_PHYSICS_EVENTS_H_
