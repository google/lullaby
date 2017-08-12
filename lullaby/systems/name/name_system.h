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

#ifndef LULLABY_SYSTEMS_NAME_NAME_SYSTEM_H_
#define LULLABY_SYSTEMS_NAME_NAME_SYSTEM_H_

#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/hash.h"

namespace lull {

// Associates a name with an entity.
class NameSystem : public System {
 public:
  // If |allow_duplicate_names| is true, multiple entities are allowed to be
  // associated with the same name.
  NameSystem(Registry* registry, bool allow_duplicate_names);
  explicit NameSystem(Registry* registry) : NameSystem(registry, false) {}

  // Associates |entity| with a name. Removes any existing name associated
  // with this entity.
  void Create(Entity entity, HashValue type, const Def* def) override;

  // Disassociates any name from |entity|.
  void Destroy(Entity entity) override;

  // Associates |entity| with |name|. Removes any existing name associated
  // with this entity.
  void SetName(Entity entity, const std::string& name);

  // Finds the name associated with |entity|. Returns empty string if no name
  // is found.
  std::string GetName(Entity entity) const;

  // Finds the entity associated with |name|. Returns kNullEntity if no entity
  // is found.
  // If |allow_duplicate_names| is true and more than one entity with the name
  // is present, which of those entities will be returned is not well defined.
  // The use of this method is discouraged when |allow_duplicate_names| is true
  // since it involves a linear search across all entities. Use |FindDescendant|
  // instead.
  Entity FindEntity(const std::string& name) const;

  // Finds the entity associated with |name| within the descendants of |root|,
  // including |root|. Returns kNullEntity if no entity is found.
  // If |allow_duplicate_names| is true and more than one entity with the name
  // is present, which of those entities will be returned is not well defined.
  Entity FindDescendant(Entity root, const std::string& name) const;

 private:
  std::unordered_map<Entity, std::string> entity_to_name_;
  std::unordered_map<Entity, HashValue> entity_to_hash_;
  // Only used when |allow_duplicate_names| is false.
  std::unordered_map<HashValue, Entity> hash_to_entity_;
  bool allow_duplicate_names_;

  Entity FindDescendantWithDuplicateNames(
      TransformSystem* transform_system, Entity root, HashValue hash) const;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::NameSystem);

#endif  // LULLABY_SYSTEMS_NAME_NAME_SYSTEM_H_
