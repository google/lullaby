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

#ifndef LULLABY_UTIL_ENTITY_H_
#define LULLABY_UTIL_ENTITY_H_

#include <functional>
#include <istream>
#include <ostream>
#include <string>

#include "lullaby/util/typeid.h"

namespace lull {

// Entity definition for Lullaby's Entity-Component-System (ECS) architecture.
//
// An Entity represents each uniquely identifiable object in the Lullaby
// runtime.  An Entity itself does not have any data or functionality - it is
// just a way to uniquely identify objects and is simply a number.
//
// Entity is a separate class so that we can distinguish it from other uint32_t
// such as HashValue, which is useful when converting data to other languages
// such as Java.
class Entity {
 public:
  constexpr Entity() : value(0) {}

  // Allow assignment, static_cast, and implicit cast to Entity.
  constexpr Entity(uint32_t value) : value(value) {}
  constexpr Entity(int32_t value) : value(static_cast<uint32_t>(value)) {}
  Entity(uint64_t value) : value(static_cast<uint32_t>(value)) {}
  Entity(int64_t value) : value(static_cast<uint32_t>(value)) {}

  // Get internal value.
  uint32_t AsUint32() const { return value; }

  // Allow static_cast from Entity.
  // TODO.
  explicit operator uint32_t() const { return value; }
  explicit operator int32_t() const { return static_cast<int32_t>(value); }
  explicit operator uint64_t() const { return static_cast<uint64_t>(value); }
  explicit operator int64_t() const { return static_cast<int64_t>(value); }

  // Allow in if statements.
  explicit operator bool() const { return value != 0; }

 private:
  uint32_t value;
};

// Special Entity value used for invalid Entities.
static constexpr Entity kNullEntity(0);

// Stream and string converters.
// Write Entity to ostream.
inline std::ostream& operator<<(std::ostream& os, Entity entity) {
  return os << entity.AsUint32();
}

// Read Entity from istream.
inline std::istream& operator>>(std::istream& is, Entity& entity) {
  uint32_t value = 0;
  is >> value;
  entity = Entity(value);
  return is;
}

inline std::string to_string(Entity entity) {
  return std::to_string(entity.AsUint32());
}

// Allow equality comparison.
inline bool operator==(Entity a, Entity b) {
  return a.AsUint32() == b.AsUint32();
}
inline bool operator!=(Entity a, Entity b) { return !(a == b); }

// Required for map/set.  Other comparators shouldn't be needed.
inline bool operator<(Entity a, Entity b) {
  return a.AsUint32() < b.AsUint32();
}

// DEPRECATED: Provided for legacy purposes. Please do not introduce more usages
// of these.
inline Entity operator+(Entity a, Entity b) {
  return a.AsUint32() + b.AsUint32();
}
inline Entity operator%(Entity a, Entity b) {
  return a.AsUint32() % b.AsUint32();
}

// Required for unordered_map/set.
struct EntityHash {
  size_t operator()(const Entity& entity) const {
    return static_cast<size_t>(entity.AsUint32());
  }
};

}  // namespace lull

namespace std {
// Required for unordered_map/set.
// TODO Use EntityHash instead and remove this.
template <>
struct hash<lull::Entity> {
  size_t operator()(const lull::Entity& entity) const {
    return hash<uint32_t>{}(entity.AsUint32());
  }
};
}  // namespace std

LULLABY_SETUP_TYPEID(lull::Entity);

#endif  // LULLABY_UTIL_ENTITY_H_
