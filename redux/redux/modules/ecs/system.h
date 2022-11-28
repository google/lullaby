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

#ifndef REDUX_MODULES_ECS_SYSTEM_H_
#define REDUX_MODULES_ECS_SYSTEM_H_

#include "redux/modules/base/function_traits.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/static_registry.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/ecs/entity_factory.h"

namespace redux {

// System base-class for Entity-Component-System (ECS) architecture.
//
// Systems are responsible for storing the actual Component data instances
// associated with Entities. They also perform all the logic for manipulating
// and processing their Components.
//
// This base class provides an API for the EntityFactory to coordinate the
// management of Components within the Systems. Specifically, the EntityFactory
// can notify Systems when Entities are enabled, disabled, and destroyed.
//
// For Entity creation, Systems can register with the EntityFactory such that
// specific ComponentDefs in a Blueprint will invoke a function of the System
// that can create Components and associate them with the newly created Entity.
//
// To simplify the creation of Systems with the EntityFactory, libraries can use
// the REDUX_STATIC_REGISTER_SYSTEM macro to statically register the System.
class System {
 public:
  explicit System(Registry* registry) : registry_(registry) {}
  virtual ~System() = default;

  System(const System&) = delete;
  System& operator=(const System&) = delete;

 protected:
  friend class EntityFactory;

  // Removes all Components from the `entity`.
  virtual void OnDestroy(Entity entity) {}

  // Enables all Components associated with the `entity`.
  virtual void OnEnable(Entity entity) {}

  // Disables all Components associated with the `entity`.
  virtual void OnDisable(Entity entity) {}

  // Wrapper around EntityFactory::IsEnabled.
  bool IsEntityEnabled(Entity entity) const {
    const EntityFactory* entity_factory = GetEntityFactory();
    return entity_factory ? entity_factory->IsEnabled(entity) : true;
  }

  // Helper function to associate the System with DefT in the EntityFactory.
  // Example usage: RegisterDef(&MySystem::AddFromDefT);
  template <typename Fn>
  void RegisterDef(Fn&& fn) {
    EntityFactory* entity_factory = GetEntityFactory();
    if (entity_factory) {
      using Traits = FunctionTraits<Fn>;
      using System = typename Traits::ClassType;
      using DefT = std::decay_t<typename Traits::template Arg<1>::Type>;

      auto system = static_cast<System*>(this);
      entity_factory->RegisterDef<DefT>(system, std::forward<Fn>(fn));
    }
  }

  // Register a dependency of this system on another type in the registry.
  // Example usage: RegisterDependency<OtherSystem>(this);
  template <typename T, typename S>
  void RegisterDependency(S* system) {
    CHECK(static_cast<System*>(system) == this);
    registry_->RegisterDependency<T>(system);
  }

  EntityFactory* GetEntityFactory() {
    if (entity_factory_ == nullptr) {
      entity_factory_ = registry_->Get<EntityFactory>();
    }
    return entity_factory_;
  }

  const EntityFactory* GetEntityFactory() const {
    if (entity_factory_ == nullptr) {
      entity_factory_ = registry_->Get<EntityFactory>();
    }
    return entity_factory_;
  }

  Registry* registry_ = nullptr;
  mutable EntityFactory* entity_factory_ = nullptr;
};

template <typename T>
static void CreateSystem(Registry* registry) {
  EntityFactory* entity_factory = registry->Get<EntityFactory>();
  if (entity_factory == nullptr) {
    entity_factory = registry->Create<EntityFactory>(registry);
  }
  entity_factory->CreateSystem<T>();
}

// By simply adding this macro to a .cc file and linking it into a binary, the
// System T will automatically be created and registered with the EntityFactory
// when calling StaticRegistry::Create.
#define REDUX_STATIC_REGISTER_SYSTEM(T) \
  static ::redux::StaticRegistry Static_Register(::redux::CreateSystem<T>);

}  // namespace redux

#endif  // REDUX_MODULES_ECS_SYSTEM_H_
