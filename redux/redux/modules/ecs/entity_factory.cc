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

#include "redux/modules/ecs/entity_factory.h"

#include "redux/modules/ecs/entity.h"
#include "redux/modules/ecs/system.h"

namespace redux {

static constexpr int32_t kDisabledExplicitly = 0x1 << 0;
static constexpr int32_t kDisabledIndirectly = 0x1 << 1;
static constexpr int32_t kDisabled = kDisabledExplicitly | kDisabledIndirectly;

EntityFactory::EntityFactory(Registry* registry)
    : registry_(registry), blueprint_factory_(registry) {}

EntityFactory::~EntityFactory() {}

Entity EntityFactory::Create() {
  const Entity entity(++entity_generator_);
  CHECK_NE(entity_generator_, 0);
  metadata_[entity] = Bits32::None();
  return entity;
}

Entity EntityFactory::Create(const BlueprintPtr& blueprint) {
  CHECK(blueprint);
  if (blueprint == nullptr) {
    return kNullEntity;
  }

  Entity entity = Create();
  for (size_t i = 0; i < blueprint->GetNumComponents(); ++i) {
    const TypeId type = blueprint->GetComponentType(i);
    auto iter = add_fns_.find(type);
    CHECK(iter != add_fns_.end());
    absl::Status status = iter->second(entity, blueprint->GetComponent(i));
    CHECK(status.ok()) << "Unable to read component " << i << " of blueprint "
                       << blueprint->GetName() << ": " << status;
  }

  return entity;
}

Entity EntityFactory::Load(std::string_view uri) {
  const BlueprintPtr blueprint = blueprint_factory_.LoadBlueprint(uri);
  return Create(blueprint);
}

void EntityFactory::DestroyNow(Entity entity) {
  auto iter = metadata_.find(entity);
  if (iter != metadata_.end()) {
    for (auto& system_entry : systems_) {
      system_entry.second->OnDestroy(entity);
    }
    metadata_.erase(iter);
  }
}

void EntityFactory::QueueForDestruction(Entity entity) {
  if (entity != kNullEntity) {
    pending_destruction_.push(entity);
  }
}

void EntityFactory::DestroyQueuedEntities() {
  std::queue<Entity> pending;

  // Swap out the pending queue in case someone calls QueueForDestruction for
  // another Entity during destruction.
  using std::swap;
  std::swap(pending_destruction_, pending);

  while (!pending.empty()) {
    DestroyNow(pending.front());
    pending.pop();
  }
}

void EntityFactory::Enable(Entity entity) {
  UpdateEnableBits(entity, 0, kDisabledExplicitly);
}

void EntityFactory::Disable(Entity entity) {
  UpdateEnableBits(entity, kDisabledExplicitly, 0);
}

void EntityFactory::DisableIndirectly(Entity entity) {
  UpdateEnableBits(entity, kDisabledIndirectly, 0);
}

void EntityFactory::ClearIndirectDisable(Entity entity) {
  UpdateEnableBits(entity, 0, kDisabledIndirectly);
}

void EntityFactory::UpdateEnableBits(Entity entity, int32_t set_bits,
                                     int32_t clear_bits) {
  auto iter = metadata_.find(entity);
  if (iter == metadata_.end()) {
    return;
  }

  const Bits32 before = iter->second;
  iter->second.Set(set_bits);
  iter->second.Clear(clear_bits);
  const Bits32 after = iter->second;

  const bool enable = before.Any(kDisabled) && after.None(kDisabled);
  const bool disable = before.None(kDisabled) && after.Any(kDisabled);

  if (enable) {
    for (auto& system_entry : systems_) {
      system_entry.second->OnEnable(entity);
    }
  } else if (disable) {
    for (auto& system_entry : systems_) {
      system_entry.second->OnDisable(entity);
    }
  }
}

bool EntityFactory::IsAlive(Entity entity) const {
  return metadata_.contains(entity);
}

bool EntityFactory::IsEnabled(Entity entity) const {
  auto iter = metadata_.find(entity);
  return iter != metadata_.end() ? iter->second.None(kDisabled) : false;
}

}  // namespace redux
