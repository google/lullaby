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

#include "lullaby/systems/datastore/datastore_system.h"

#include "lullaby/generated/datastore_def_generated.h"
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/variant.h"

namespace lull {

static const HashValue kDatastoreDef = Hash("DatastoreDef");

DatastoreSystem::DatastoreSystem(Registry* registry) : System(registry) {
  RegisterDef(this, kDatastoreDef);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Datastore.Set",
                           &lull::DatastoreSystem::Set<Variant>);
    binder->RegisterMethod("lull.Datastore.Remove",
                           &lull::DatastoreSystem::Remove);
    binder->RegisterFunction(
        "lull.Datastore.Get",
        [this](Entity e, HashValue key) { return GetVariant(e, key); });
  }
}

DatastoreSystem::~DatastoreSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Datastore.Set");
    binder->UnregisterFunction("lull.Datastore.Get");
    binder->UnregisterFunction("lull.Datastore.Remove");
  }
}

void DatastoreSystem::Create(Entity entity, HashValue type, const Def* def) {
  CHECK_EQ(type, kDatastoreDef);
  const lull::DatastoreDef* data = ConvertDef<DatastoreDef>(def);
  if (data == nullptr || data->key_value_pairs() == nullptr) {
    LOG(ERROR) << "No data in DatastoreDef.";
    return;
  }

  const auto* pairs = data->key_value_pairs();
  for (auto iter = pairs->begin(); iter != pairs->end(); ++iter) {
    const auto* key = iter->key();
    const void* variant_def = iter->value();
    if (key == nullptr || variant_def == nullptr) {
      LOG(ERROR) << "Invalid (nullptr) key-value data in DatastoreDef.";
      continue;
    }

    Variant var;
    if (VariantFromFbVariant(iter->value_type(), variant_def, &var)) {
      const HashValue key_hash = Hash(key->c_str());
      Set(entity, key_hash, var);
    }
  }
}

void DatastoreSystem::Destroy(Entity entity) { stores_.erase(entity); }

void DatastoreSystem::Set(Entity entity, HashValue key,
                          const Variant& variant) {
  if (entity == kNullEntity) {
    return;
  }

  const auto store_iter = stores_.emplace(entity, Datastore()).first;
  store_iter->second[key] = variant;
}

void DatastoreSystem::Remove(Entity entity, HashValue key) {
  const auto store_iter = stores_.find(entity);
  if (store_iter == stores_.end()) {
    return;
  }
  store_iter->second.erase(key);
  if (store_iter->second.empty()) {
    stores_.erase(entity);
  }
}

const Variant& DatastoreSystem::GetVariant(Entity entity, HashValue key) const {
  const auto store_iter = stores_.find(entity);
  if (store_iter == stores_.end()) {
    return empty_variant_;
  }

  const Datastore& store = store_iter->second;
  auto variant_map_iter = store.find(key);
  if (variant_map_iter == store.end()) {
    return empty_variant_;
  }

  return variant_map_iter->second;
}

}  // namespace lull
