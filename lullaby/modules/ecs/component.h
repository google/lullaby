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

#ifndef LULLABY_MODULES_ECS_COMPONENT_H_
#define LULLABY_MODULES_ECS_COMPONENT_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/unordered_vector_map.h"

namespace lull {

// Base class for Components in Lullaby's Entity-Component-System (ECS)
// architecture.
//
// While Components are not required to have a specific structure, this struct
// can be useful as a base-class for most common situations.  The purpose of
// this struct is to provide a consistent way to get the Entity to which a
// Component belongs.  It is not intended to be base-class for an object
// oriented hierarchy and, as such, the lack of a virtual destructor is a
// deliberate choice to keep the overhead minimal.
struct Component {
  explicit Component(Entity e) : entity(e) {}

  Entity GetEntity() const { return entity; }

 private:
  Entity entity;
};

// Hash functor for Components so they can be used in unordered containers.
// Specifically, it uses the Component's Entity as the key value.
struct ComponentHash {
  Entity operator()(const Component& c) const {
    return c.GetEntity();
  }
};

// Type alias for using UnorderedVectorMap to objects deriving from Component.
template <typename T>
using ComponentPool = UnorderedVectorMap<Entity, T, ComponentHash>;

}  // namespace lull

#endif  // LULLABY_MODULES_ECS_COMPONENT_H_
