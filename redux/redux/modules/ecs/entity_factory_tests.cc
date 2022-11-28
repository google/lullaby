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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/ecs/blueprint_factory.h"
#include "redux/modules/ecs/entity_factory.h"
#include "redux/modules/ecs/system.h"

namespace redux {
namespace {

using ::testing::Eq;

struct TestDef {
  int value = 0;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(value, ConstHash("value"));
  }
};

class TestSystem : public System {
 public:
  explicit TestSystem(Registry* registry) : System(registry) {
    RegisterDef(&TestSystem::Add);
  }

  void Add(Entity entity, const TestDef& component) {
    data_[entity] = component.value;
  }

  absl::flat_hash_map<Entity, int> data_;
};

}  // namespace
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TestDef);
REDUX_SETUP_TYPEID(redux::TestSystem);

namespace redux {
namespace {

TEST(EntityFactoryTest, Basic) {
  Registry registry;
  auto blueprint_factory = registry.Create<BlueprintFactory>(&registry);
  auto entity_factory = registry.Create<EntityFactory>(&registry);
  auto test_system = entity_factory->CreateSystem<TestSystem>();

  const char* txt =
      "{"
      "  'redux::TestDef': {"
      "    'value': (+ 12 34),"
      "  },"
      "}";

  BlueprintPtr blueprint = blueprint_factory->ReadBlueprint(txt);
  Entity entity = entity_factory->Create(blueprint);

  EXPECT_TRUE(test_system->data_.contains(entity));
  EXPECT_THAT(test_system->data_[entity], Eq(46));
}

}  // namespace
}  // namespace redux
