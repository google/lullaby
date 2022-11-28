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

#ifndef REDUX_MODULES_ECS_ENTITY_H_
#define REDUX_MODULES_ECS_ENTITY_H_

#include <cstdint>

#include "redux/modules/base/typeid.h"

namespace redux {

// A handle for each uniquely identifiable object in the redux runtime.
//
// An Entity itself does not have any data or functionality; it is just a way to
// uniquely identify objects and is simply a number.
class Entity {
 public:
  // Underlying representation for Entity.
  using Rep = std::uint32_t;

  constexpr Entity() : value_(0) {}
  constexpr explicit Entity(Rep value) : value_(value) {}

  constexpr Rep get() const { return value_; }

  explicit operator bool() const { return value_ != 0; }

 private:
  template <typename H>
  friend H AbslHashValue(H h, const Entity& entity) {
    return H::combine(std::move(h), entity.value_);
  }

  friend bool operator==(Entity lhs, Entity rhs) {
    return lhs.get() == rhs.get();
  }
  friend bool operator!=(Entity lhs, Entity rhs) {
    return lhs.get() != rhs.get();
  }
  friend bool operator<(Entity lhs, Entity rhs) {
    return lhs.get() < rhs.get();
  }
  friend bool operator<=(Entity lhs, Entity rhs) {
    return lhs.get() <= rhs.get();
  }
  friend bool operator>(Entity lhs, Entity rhs) {
    return lhs.get() > rhs.get();
  }
  friend bool operator>=(Entity lhs, Entity rhs) {
    return lhs.get() >= rhs.get();
  }

  Rep value_;
};

// Special Entity value used for invalid Entities.
inline constexpr Entity kNullEntity;

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Entity);

#endif  // REDUX_MODULES_ECS_ENTITY_H_
