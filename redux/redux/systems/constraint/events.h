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

#ifndef REDUX_SYSTEMS_CONSTRAINT_EVENTS_H_
#define REDUX_SYSTEMS_CONSTRAINT_EVENTS_H_

#include "redux/modules/base/hash.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/ecs/entity.h"

namespace redux {

// Event for when a child consrtaint is added to a parent.
struct ChildConstraintAddedEvent {
  Entity parent;
  Entity child;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(parent, ConstHash("parent"));
    archive(child, ConstHash("child"));
  }
};

// Event for when a child constraint is removed from a parent.
struct ChildConstraintRemovedEvent {
  Entity parent;
  Entity child;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(parent, ConstHash("parent"));
    archive(child, ConstHash("child"));
  }
};

// Event for when a parent constraint changes for a child.
struct ParentConstraintChangedEvent {
  Entity child;
  Entity old_parent;
  Entity new_parent;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(child, ConstHash("child"));
    archive(old_parent, ConstHash("old_parent"));
    archive(new_parent, ConstHash("new_parent"));
  }
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ChildConstraintAddedEvent);
REDUX_SETUP_TYPEID(redux::ChildConstraintRemovedEvent);
REDUX_SETUP_TYPEID(redux::ParentConstraintChangedEvent);

#endif  // REDUX_SYSTEMS_CONSTRAINT_EVENTS_H_
