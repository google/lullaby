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

#ifndef REDUX_SYSTEMS_DATASTORE_DATASTORE_SYSTEM_H_
#define REDUX_SYSTEMS_DATASTORE_DATASTORE_SYSTEM_H_

#include <functional>
#include <optional>

#include "absl/container/flat_hash_map.h"
#include "redux/engines/script/function_binder.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/var/var_table.h"
#include "redux/systems/datastore/datastore_def_generated.h"

namespace redux {

// Associates arbitrary data as key/value pairs (in a VarTable) with Entities.
class DatastoreSystem : public System {
 public:
  explicit DatastoreSystem(Registry* registry);

  void OnRegistryInitialize();

  // Adds a value to the Entity's datastore with the given key.
  void Add(Entity entity, HashValue key, Var value);

  // Returns the value associated with the key on the Entity. Returns an
  // uninitialized Var if no such value exists.
  const Var& GetValue(Entity entity, HashValue key) const;

  // Removes the value associated with the given key from the Entity.
  void Remove(Entity entity, HashValue key);

  // Removes all the values associated with the Entity.
  void RemoveAll(Entity entity);

  // Adds all the values from the DatastoreDef to the Entity.
  void SetFromDatastoreDef(Entity entity, const DatastoreDef& def);

 private:
  void OnDestroy(Entity entity) override;

  FunctionBinder fns_;
  absl::flat_hash_map<Entity, VarTable> tables_;
  Var empty_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::DatastoreSystem);

#endif  // REDUX_SYSTEMS_DATASTORE_DATASTORE_SYSTEM_H_
