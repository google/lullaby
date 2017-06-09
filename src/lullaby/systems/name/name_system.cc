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

#include "lullaby/systems/name/name_system.h"

#include "lullaby/generated/name_def_generated.h"
#include "lullaby/util/logging.h"

namespace lull {

namespace {

const char* kEmptyString = "";
const HashValue kNameDefHash = Hash("NameDef");
}  // namespace

NameSystem::NameSystem(Registry* registry, bool allow_duplicate_names)
    : System(registry), allow_duplicate_names_(allow_duplicate_names) {
  RegisterDef(this, kNameDefHash);
}

void NameSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kNameDefHash) {
    LOG(DFATAL) << "Invalid def type, expecting NameDef.";
    return;
  } else if (def == nullptr) {
    LOG(DFATAL) << "Invalid def, nullptr.";
    return;
  }

  const auto* data = ConvertDef<NameDef>(def);
  if (data->name() != nullptr) {
    SetName(entity, data->name()->str());
  }
}

void NameSystem::Destroy(Entity entity) {
  auto iter = entity_to_hash_.find(entity);
  if (iter != entity_to_hash_.end()) {
    hash_to_entity_.erase(iter->second);
    entity_to_hash_.erase(iter);
  }
  entity_to_name_.erase(entity);
}

void NameSystem::SetName(Entity entity, const std::string& name) {
  if (entity == kNullEntity) {
    LOG(DFATAL) << "Invalid entity, kNullEntity.";
    return;
  }

  const auto existing_name = GetName(entity);
  // No need to proceed if current name and target name are the same.
  if (existing_name == name) {
    return;
  }

  const auto hash = Hash(name.c_str());
  if (!allow_duplicate_names_) {
    // Ensure a different entity with the same name does not already exist. This
    // may happen if an entity with the name had not been properly deleted or
    // the same entity had been created multiple times.
    if (FindEntity(name) != kNullEntity) {
      LOG(DFATAL) << "Entity " << name << " already exists!";
      return;
    }
    hash_to_entity_.erase(Hash(existing_name.c_str()));
    hash_to_entity_[hash] = entity;
  }
  entity_to_name_[entity] = name;
  entity_to_hash_[entity] = hash;
}

std::string NameSystem::GetName(Entity entity) const {
  const auto iter = entity_to_name_.find(entity);
  return iter != entity_to_name_.end() ? iter->second
                                       : std::string(kEmptyString);
}

Entity NameSystem::FindEntity(const std::string& name) const {
  const auto hash = Hash(name.c_str());
  if (allow_duplicate_names_) {
    for (const auto& entry : entity_to_hash_) {
      if (entry.second == hash) {
        return entry.first;
      }
    }
    return kNullEntity;
  } else {
    const auto iter = hash_to_entity_.find(hash);
    return iter != hash_to_entity_.end() ? iter->second : kNullEntity;
  }
}

Entity NameSystem::FindDescendant(Entity root, const std::string& name) const {
  if (root == kNullEntity) {
    LOG(DFATAL) << "root cannot be kNullEntity in FindDescendant()";
    return kNullEntity;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  if (!transform_system) {
    LOG(DFATAL) << "TransformSystem is required.";
    return kNullEntity;
  }

  const HashValue hash = Hash(name.c_str());
  if (allow_duplicate_names_) {
    return FindDescendantWithDuplicateNames(transform_system, root, hash);
  } else {
    const auto iter = hash_to_entity_.find(hash);
    if (iter != hash_to_entity_.end()) {
      const Entity entity = iter->second;
      if (root == entity || transform_system->IsAncestorOf(root, entity)) {
        return entity;
      }
    }
    return kNullEntity;
  }
}

Entity NameSystem::FindDescendantWithDuplicateNames(
    TransformSystem* transform_system, Entity root, HashValue hash) const {
  const auto iter = entity_to_hash_.find(root);
  if (iter != entity_to_hash_.end() && iter->second == hash) {
    return root;
  }

  const auto* children = transform_system->GetChildren(root);
  if (children) {
    for (Entity child : *children) {
      const Entity result =
          FindDescendantWithDuplicateNames(transform_system, child, hash);
      if (result != kNullEntity) {
        return result;
      }
    }
  }

  return kNullEntity;
}

}  // namespace lull
