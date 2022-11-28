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

#include "redux/systems/name/name_system.h"

#include <functional>
#include <string>
#include <string_view>

#include "absl/strings/str_cat.h"

namespace redux {

NameSystem::NameSystem(Registry* registry) : System(registry), fns_(registry) {
  RegisterDef(&NameSystem::SetFromNameDef);
}

void NameSystem::OnRegistryInitialize() {
  fns_.RegisterMemFn("rx.Name.GetName", this, &NameSystem::GetName);
  fns_.RegisterMemFn("rx.Name.SetName", this, &NameSystem::SetName);
  fns_.RegisterMemFn("rx.Name.FindEntities", this,
                     &NameSystem::FindEntitiesInternal);
}

void NameSystem::SetName(Entity entity, std::string_view name) {
  if (entity == kNullEntity) {
    return;
  }

  // No need to proceed if current name and target name are the same.
  if (GetName(entity) == name) {
    return;
  }

  // Clear out the component entirely.
  if (name.empty()) {
    OnDestroy(entity);
    return;
  }

  // Create and store an std::string in hash_to_name_.
  const HashValue hash = Hash(name);
  std::string& cached_name = hash_to_name_[hash];
  if (cached_name.empty()) {
    cached_name = name;
  } else {
    CHECK(cached_name == name)
        << "Hash collision: `" << cached_name << "` : `" << name << "`";
  }

  entity_to_name_[entity] = cached_name;
  hash_to_entities_[hash].emplace_back(entity);
}

void NameSystem::OnDestroy(Entity entity) {
  auto iter = entity_to_name_.find(entity);
  if (iter == entity_to_name_.end()) {
    return;
  }

  const HashValue hash = Hash(iter->second);
  std::vector<Entity>& entities = hash_to_entities_[hash];

  // Remove `entity` from `entities` using swap-and-pop idiom.
  for (Entity& e : entities) {
    if (e == entity) {
      e = entities.back();
      entities.pop_back();
      break;
    }
  }

  // Remove the name associated with the entity.
  entity_to_name_.erase(iter);

  // Cleanup hash-based tables if no more entities with the name.
  if (entities.empty()) {
    hash_to_entities_.erase(hash);
    hash_to_name_.erase(hash);
  }
}

void NameSystem::SetFromNameDef(Entity entity, const NameDef& def) {
  SetName(entity, def.name);
}

std::string_view NameSystem::GetName(Entity entity) const {
  const auto iter = entity_to_name_.find(entity);
  return iter != entity_to_name_.end() ? iter->second : std::string_view();
}

absl::StatusOr<Entity> NameSystem::FindEntity(std::string_view name) const {
  const auto& entities = FindEntitiesInternal(name);
  if (entities.empty()) {
    return absl::NotFoundError(
        absl::StrCat("No Entity with the name: `", name, "`."));
  } else if (entities.size() == 1) {
    return entities.front();
  } else {
    return absl::OutOfRangeError(
        absl::StrCat("Multiple Entities with the name `", name, "`. ",
                     "Try FindEntities instead?"));
  }
}

absl::flat_hash_set<Entity> NameSystem::FindEntities(
    std::string_view name) const {
  const auto entities = FindEntitiesInternal(name);
  absl::flat_hash_set<Entity> results(entities.begin(), entities.end());
  return results;
}

void NameSystem::ForEachEntity(std::string_view name,
                               absl::FunctionRef<void(Entity)> fn) const {
  auto iter = hash_to_entities_.find(Hash(name));
  if (iter != hash_to_entities_.end()) {
    for (Entity entity : iter->second) {
      fn(entity);
    }
  }
}

absl::Span<const Entity> NameSystem::FindEntitiesInternal(
    std::string_view name) const {
  auto iter = hash_to_entities_.find(Hash(name));
  if (iter != hash_to_entities_.end()) {
    return iter->second;
  } else {
    return {};
  }
}

}  // namespace redux
