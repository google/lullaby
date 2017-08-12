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

#ifndef LULLABY_MODULES_ECS_SYSTEM_H_
#define LULLABY_MODULES_ECS_SYSTEM_H_

#include "flatbuffers/flatbuffers.h"
#include "lullaby/modules/ecs/blueprint.h"
#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/typeid.h"

namespace lull {

// System base-class for Lullaby's Entity-Component-System (ECS) architecture.
//
// Systems are responsible for storing the actual Component data instances
// associated with Entities.  They also perform all the logic for manipulating
// and processing their Components.
//
// This base class simply provides an API for the EntityFactory to associate
// Components with Entities in a data-driven manner.  All other logic and
// functionality is System-specific and is left to the derived classes to
// implement.
class System {
 public:
  explicit System(Registry* registry);
  virtual ~System() {}

  // The ECS uses flatbuffers for serialized data.  All flatbuffer data types
  // derive from flatbuffers::Table.
  using Def = flatbuffers::Table;

  // The Hash of the actual ComponentDef type used for safely casting the
  // Def to a concrete type for extracting data.
  using DefType = HashValue;

  // Initializes inter-system dependencies.  This function is called after all
  // Systems have been created by the EntityFactory.
  virtual void Initialize() {}

  virtual void CreateComponent(Entity e, const Blueprint& blueprint) {
    Create(e, blueprint.GetLegacyDefType(), blueprint.GetLegacyDefData());
  }

  virtual void PostCreateComponent(Entity e, const Blueprint& blueprint) {
    PostCreateInit(e, blueprint.GetLegacyDefType(),
                   blueprint.GetLegacyDefData());
  }

  // Associates Component(s) with the Entity using the serialized |def| data.
  virtual void Create(Entity e, DefType type, const Def* def) {}

  // Performs any post-creation initialization of Component data that may depend
  // on Components from other Systems.
  virtual void PostCreateInit(Entity e, DefType type, const Def* def) {}

  // Disassociates all Component data from the Entity.
  virtual void Destroy(Entity e) {}

 protected:
  // Converts a flatbuffer::Table to a derived type for processing.
  template <typename T>
  const T* ConvertDef(const Def* def) {
    // Unfortunately, flatbuffers uses private inheritance for the defs.  As a
    // result, we need to use a C-Style cast to covert between the base
    // flatbuffers::Table class and the derived Def type.
    return (const T*)def;
  }

  // Helper function to associate the System with DefType in the EntityFactory.
  // Example usage: RegisterDef(this, Hash("MyComponentDef"));
  template <typename S>
  void RegisterDef(S* system, DefType type) {
    RegisterDef(GetTypeId<S>(), type);
  }

  // Associates the System with the DefType in the EntityFactory.
  void RegisterDef(TypeId system_type, DefType type);

  // Register a dependency of this system on another type in the registry.
  // Example usage: RegisterDependency<OtherSystem>(this);
  template <typename T, typename S>
  void RegisterDependency(S* system) {
    registry_->RegisterDependency<T>(system);
  }

  // Generic container that owns all Lullaby objects.
  Registry* registry_;

 private:
  System(const System&);
  System& operator=(const System&);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::System);

#endif  // LULLABY_MODULES_ECS_SYSTEM_H_
