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

#include "lullaby/modules/ecs/system.h"

#include "lullaby/modules/ecs/entity_factory.h"

namespace lull {

System::System(Registry* registry) : registry_(registry) {}

void System::RegisterDef(TypeId system_type, HashValue type) {
  auto* entity_factory = registry_->Get<EntityFactory>();
  if (entity_factory) {
    entity_factory->RegisterDef(system_type, type);
  }
}

}  // namespace lull
