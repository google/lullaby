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

#include "redux/systems/datastore/datastore_system.h"

#include <functional>
#include <optional>
#include <utility>

namespace redux {

DatastoreSystem::DatastoreSystem(Registry* registry)
    : System(registry), fns_(registry) {
  RegisterDef(&DatastoreSystem::SetFromDatastoreDef);
}

void DatastoreSystem::OnRegistryInitialize() {
  fns_.RegisterMemFn("rx.Datastore.Add", this, &DatastoreSystem::Add);
  fns_.RegisterMemFn("rx.Datastore.Remove", this, &DatastoreSystem::Remove);
  fns_.RegisterMemFn("rx.Datastore.Get", this, &DatastoreSystem::GetValue);
}

void DatastoreSystem::Add(Entity entity, HashValue key, Var value) {
  tables_[entity][key] = std::move(value);
}

void DatastoreSystem::SetFromDatastoreDef(Entity entity,
                                          const DatastoreDef& def) {
  if (entity != kNullEntity) {
    VarTable& table = tables_[entity];
    for (const auto& kv : def.data) {
      table[kv.first] = kv.second;
    }
  }
}

const Var& DatastoreSystem::GetValue(Entity entity, HashValue key) const {
  auto iter = tables_.find(entity);
  if (iter != tables_.end()) {
    return iter->second[key];
  }
  return empty_;
}

void DatastoreSystem::Remove(Entity entity, HashValue key) {
  auto iter = tables_.find(entity);
  if (iter != tables_.end()) {
    iter->second.Erase(key);
  }
}

void DatastoreSystem::RemoveAll(Entity entity) { tables_.erase(entity); }

void DatastoreSystem::OnDestroy(Entity entity) { RemoveAll(entity); }

}  // namespace redux
