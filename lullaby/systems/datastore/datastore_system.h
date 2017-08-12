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

#ifndef LULLABY_SYSTEMS_DATASTORE_DATASTORE_SYSTEM_H_
#define LULLABY_SYSTEMS_DATASTORE_DATASTORE_SYSTEM_H_

#include <unordered_map>
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/variant.h"

namespace lull {

// Manages a Datastore per Entity.
//
// A Datastore is just a dictionary of a HashValue to a Variant.  Adding a
// datastore to an Entity allows arbitrary key-value pairs to be associated with
// the Entity.
class DatastoreSystem : public System {
 public:
  explicit DatastoreSystem(Registry* registry);

  ~DatastoreSystem() override;

  // Adds values to the datastore for Entity |entity| using data from the |def|.
  void Create(Entity entity, HashValue type, const Def* def) override;

  // Removes datastore from the Entity.
  void Destroy(Entity entity) override;

  // Associates the |value| with the |key| on the |entity|.
  template <typename T>
  void Set(Entity entity, HashValue key, const T& value);

  // Associates the value stored in |variant| with the |key| on the |entity|.
  void Set(Entity entity, HashValue key, const Variant& variant);

  // Removes the value associated with the |key| on the |entity|.
  void Remove(Entity entity, HashValue key);

  // Returns a pointer to the value associated with the |key| on the |entity|,
  // or returns nullptr if not set or the type is incorrect.
  template <typename T>
  const T* Get(Entity entity, HashValue key) const;

  // Returns the Variant value associated with the |key| on the |entity|, or an
  // empty Variant if not set.
  const Variant& GetVariant(Entity entity, HashValue key) const;

 private:
  using Datastore = std::unordered_map<HashValue, Variant>;
  using EntityMap = std::unordered_map<Entity, Datastore>;

  EntityMap stores_;
  Variant empty_variant_;

  DatastoreSystem(const DatastoreSystem&);
  DatastoreSystem& operator=(const DatastoreSystem&);
};

template <typename T>
void DatastoreSystem::Set(Entity entity, HashValue key, const T& value) {
  if (entity == kNullEntity) {
    return;
  }

  const auto store_iter = stores_.emplace(entity, Datastore()).first;
  store_iter->second[key] = value;
}

template <typename T>
const T* DatastoreSystem::Get(Entity entity, HashValue key) const {
  return GetVariant(entity, key).Get<T>();
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DatastoreSystem);

#endif  // LULLABY_SYSTEMS_DATASTORE_DATASTORE_SYSTEM_H_
