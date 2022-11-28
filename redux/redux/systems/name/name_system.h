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

#ifndef REDUX_SYSTEMS_NAME_NAME_SYSTEM_H_
#define REDUX_SYSTEMS_NAME_NAME_SYSTEM_H_

#include <functional>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/functional/function_ref.h"
#include "absl/status/statusor.h"
#include "redux/engines/script/function_binder.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/name/name_def_generated.h"

namespace redux {

// Associates a string name with an Entity.
class NameSystem : public System {
 public:
  explicit NameSystem(Registry* registry);

  void OnRegistryInitialize();

  // Sets the name of the Entity.
  void SetName(Entity entity, std::string_view name);

  // Returns the name of the Entity.
  std::string_view GetName(Entity entity) const;

  // Returns an Entity with the given name. If multiple such Entities exist
  // (i.e. multiple Entities share the same name), returns absl::OutOfRange.
  // In this case, consider using FindEntities instead. Returns absl::NotFound
  // if no such Entity exists.
  absl::StatusOr<Entity> FindEntity(std::string_view name) const;

  // Returns the set of Entities with the given name.
  absl::flat_hash_set<Entity> FindEntities(std::string_view name) const;

  // Invokes the provided function for each Entity with the given name.
  void ForEachEntity(std::string_view name,
                     absl::FunctionRef<void(Entity)> fn) const;

  // Adds a name to the Entity from a NameDef.
  void SetFromNameDef(Entity entity, const NameDef& def);

 private:
  void OnDestroy(Entity entity) override;

  absl::Span<const Entity> FindEntitiesInternal(std::string_view name) const;

  FunctionBinder fns_;
  absl::flat_hash_map<HashValue, std::string> hash_to_name_;
  absl::flat_hash_map<Entity, std::string_view> entity_to_name_;
  absl::flat_hash_map<HashValue, std::vector<Entity>> hash_to_entities_;
  static std::vector<Entity> empty_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::NameSystem);

#endif  // REDUX_SYSTEMS_NAME_NAME_SYSTEM_H_
