/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "base/stringprintf.h"
#include "ion/base/logging.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"
#include "lullaby/tests/test_entity_generated.h"

class FuzzSystem : public lull::System {
  const lull::HashValue kValueDefHash = lull::ConstHash("ValueDef");
  const lull::HashValue kComplexDefHash = lull::ConstHash("ComplexDef");

 public:
  explicit FuzzSystem(lull::Registry* registry) : lull::System(registry) {
    RegisterDef(this, kValueDefHash);
    RegisterDef(this, kComplexDefHash);
  }

  void Create(lull::Entity e, lull::HashValue type, const Def* def) override {
    CHECK(type == kValueDefHash || type == kComplexDefHash);
  }

  void PostCreateComponent(lull::Entity e,
                           const lull::Blueprint& blueprint) override {
    CHECK(blueprint.Is<lull::testing::ValueDefT>() ||
          blueprint.Is<lull::testing::ComplexDefT>());
  }
};

LULLABY_SETUP_TYPEID(FuzzSystem);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string test_data(reinterpret_cast<const char*>(data), size);
  lull::Registry registry;
  registry.Create<lull::AssetLoader>(
      [=](const char* name, std::string* out) -> bool {
        *out = test_data;
        return true;
      });
  auto* entity_factory = registry.Create<lull::EntityFactory>(&registry);

  entity_factory->CreateSystem<FuzzSystem>();

  entity_factory
      ->Initialize<lull::testing::EntityDef, lull::testing::ComponentDef>(
          lull::testing::GetEntityDef,
          lull::testing::EnumNamesComponentDefType());

  ion::base::ScopedDisableExitOnDFatal disable_exit_on_dfatal;
  entity_factory->Create("test-entity");

  return 0;
}
