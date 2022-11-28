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

#ifndef REDUX_MODULES_ECS_ENTITY_FACTORY_H_
#define REDUX_MODULES_ECS_ENTITY_FACTORY_H_

#include <cstddef>
#include <functional>
#include <queue>

#include "redux/engines/script/redux/script_env.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/ecs/blueprint.h"
#include "redux/modules/ecs/blueprint_factory.h"
#include "redux/modules/ecs/component_serializer.h"
#include "redux/modules/ecs/entity.h"

namespace redux {

class System;

// Responsible for the creation and lifecycle management of Entities.
//
// The EntityFactory knows about all the Systems in a runtime and is able to
// co-ordinate the creation and destruction of Components by way of the Systems.
//
// The EntityFactory also provides Enable and Disable functions for controlling
// the active state of an Entity. In this way, the EntityFactory is, itself,
// like a System (albeit one that doesn't inherit from the System base class).
class EntityFactory {
 public:
  explicit EntityFactory(Registry* registry);
  ~EntityFactory();

  EntityFactory(const EntityFactory& rhs) = delete;
  EntityFactory& operator=(const EntityFactory& rhs) = delete;

  // Creates a System and registers it with the EntityFactory. Also registers
  // the System with the Registry. Systems _MUST_ be created this way and not
  // directly with the Registry.
  template <typename T, typename... Args>
  T* CreateSystem(Args&&... args);

  // Creates an "empty" Entity; one that has no Components.
  Entity Create();

  // Creates an Entity with attached Components as defined by the Blueprint.
  Entity Create(const BlueprintPtr& blueprint);

  // Convenience function that uses the BlueprintFactory to load a Blueprint
  // from the given 'uri' and create an Entity from it all in one go.
  Entity Load(std::string_view uri);

  // Destroys an Entity by asking all the Systems to remove all Components from
  // the Entity.
  void DestroyNow(Entity entity);

  // Queues an Entity to be destroyed later, when DestroyQueuedEntities is
  // called.
  void QueueForDestruction(Entity entity);

  // Destroys all Entities that have been previously marked for destruction.
  void DestroyQueuedEntities();

  // Enables an Entity, also calling OnEnable() on all Systems for the Entity.
  void Enable(Entity entity);

  // Disables an Entity, also calling OnDisable() on all Systems for the Entity.
  void Disable(Entity entity);

  // Disables an Entity because it's "owning" Entity has been disabled.
  void DisableIndirectly(Entity entity);

  // Removes the "inherited" disabled state of an Entity.
  void ClearIndirectDisable(Entity entity);

  // Returns true if an Entity has been created, but not yet destroyed.
  bool IsAlive(Entity entity) const;

  // Returns true if an Entity is enabled.
  bool IsEnabled(Entity entity) const;

  // Associates a ComponentDefT with a SystemT. When a Blueprint contains data
  // for a ComponentDefT, the EntityFactory will pass that data to the given
  // member function of the System, allowing the System to create Components
  // and attaching them to an Entity.
  template <typename DefT, typename SystemT>
  void RegisterDef(SystemT* system, void (SystemT::*fn)(Entity, const DefT&));

 private:
  void UpdateEnableBits(Entity entity, int32_t set_bits, int32_t clear_bits);

  using AddFn = std::function<absl::Status(Entity, const Var&)>;
  using Metadata = absl::flat_hash_map<Entity, Bits32>;

  Registry* registry_ = nullptr;
  Entity::Rep entity_generator_ = 0;
  BlueprintFactory blueprint_factory_;
  ScriptEnv env_;
  std::queue<Entity> pending_destruction_;
  absl::flat_hash_map<Entity, Bits32> metadata_;
  absl::flat_hash_map<TypeId, System*> systems_;
  absl::flat_hash_map<TypeId, AddFn> add_fns_;
};

template <typename T, typename... Args>
T* EntityFactory::CreateSystem(Args&&... args) {
  T* system = registry_->Create<T>(registry_, std::forward<Args>(args)...);
  const TypeId type = GetTypeId<T>();
  systems_[type] = system;
  return system;
}

template <typename DefT, typename SystemT>
void EntityFactory::RegisterDef(SystemT* system,
                                void (SystemT::*fn)(Entity, const DefT&)) {
  const TypeId type = GetTypeId<DefT>();
  add_fns_[type] = [this, system, fn](Entity entity, const Var& component) {
    ComponentSerializer loader(component, &env_);
    DefT def;
    Serialize(loader, def);
    if (loader.Status().ok()) {
      (system->*fn)(entity, def);
    }
    return loader.Status();
  };
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::EntityFactory);

#endif  // REDUX_MODULES_ECS_ENTITY_FACTORY_H_
