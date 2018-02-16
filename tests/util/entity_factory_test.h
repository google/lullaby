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

#ifndef LULLABY_TESTS_UTIL_ENTITY_FACTORY_TEST_H_
#define LULLABY_TESTS_UTIL_ENTITY_FACTORY_TEST_H_

#include "lullaby/modules/ecs/entity_factory.h"

#include <string>
#include <unordered_set>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"
#include "tests/portable_test_macros.h"
#include "tests/test_def_generated.h"
#include "tests/test_entity2_generated.h"
#include "tests/test_entity_generated.h"
#include "tests/util/entity_test.h"
#include "tests/util/fake_file_system.h"

namespace lull {
namespace testing {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;

const HashValue kValueDefHash = Hash("ValueDef");
const HashValue kComplexDefHash = Hash("ComplexDef");

// System and Components for test component defs in order to give them testable
// runtime behavior.
class TestSystem : public System {
 public:
  explicit TestSystem(Registry* registry) : System(registry), components_(1) {
    RegisterDef(this, kValueDefHash);
    RegisterDef(this, kComplexDefHash);
  }

  void Create(Entity e, HashValue type, const Def* def) override {
    CHECK(type == kValueDefHash || type == kComplexDefHash);

    // An entity can contain both types of defs, so potentially re-use an
    // existing component.
    auto* test_component = components_.Get(e);
    if (test_component == nullptr) {
      test_component = components_.Emplace(e);
    }
    CHECK(test_component);

    // Track the information for each def separately.
    if (type == kValueDefHash) {
      const auto* value_def = ConvertDef<ValueDef>(def);
      test_component->simple_name = value_def->name()->c_str();
      test_component->simple_value = value_def->value();
    } else if (type == kComplexDefHash) {
      const auto* complex_def = ConvertDef<ComplexDef>(def);
      test_component->complex_name = complex_def->name()->c_str();
      test_component->complex_value = complex_def->data()->value();
    }
  }

  void Destroy(Entity e) override { components_.Destroy(e); }

  std::string GetSimpleName(Entity e) {
    auto* component = components_.Get(e);
    return component ? component->simple_name : "";
  }

  int GetSimpleValue(Entity e) {
    auto* component = components_.Get(e);
    return component ? component->simple_value : 0;
  }

  std::string GetComplexName(Entity e) {
    auto* component = components_.Get(e);
    return component ? component->complex_name : "";
  }

  int GetComplexValue(Entity e) {
    auto* component = components_.Get(e);
    return component ? component->complex_value : 0;
  }

 private:
  struct TestComponent : Component {
    explicit TestComponent(Entity e) : Component(e) {}

    std::string simple_name = "";
    int simple_value = 0;
    std::string complex_name = "";
    int complex_value = 0;
  };

  ComponentPool<TestComponent> components_;
};

// Tiny system for testing the EntityFactory's behavior when a registered
// dependency is missing.
class MissingDependencySystem : public System {
 public:
  explicit MissingDependencySystem(Registry* registry) : System(registry) {
    RegisterDependency<TestSystem>(this);
  }
};

// Type-parameterized test case for testing whether the EntityFactory functions
// with a given EntityDef and ComponentDef implementation.
template <typename TypeParam>
class EntityFactoryTest : public EntityTest<TypeParam> {
 protected:
  using DataBuffer = std::vector<uint8_t>;

  EntityFactoryTest() {
    registry_.Create<AssetLoader>(
        [this](const char* name, std::string* out) -> bool {
          return fake_file_system_.LoadFromDisk(name, out);
        });
    registry_.Create<EntityFactory>(&registry_);
  }

  void InitializeEntityFactory() {
    using EntityDef = typename TypeParam::entity_type;
    using ComponentDef = typename TypeParam::component_type;

    auto entity_factory = registry_.Get<EntityFactory>();
    entity_factory->template Initialize<EntityDef, ComponentDef>(
        TypeParam::get_entity, TypeParam::component_def_type_names());
  }

  Registry registry_;
  FakeFileSystem fake_file_system_;
};
TYPED_TEST_CASE_P(EntityFactoryTest);

TYPED_TEST_P(EntityFactoryTest, NoSystems) {
  PORT_EXPECT_DEBUG_DEATH(this->InitializeEntityFactory(), "");
}

TYPED_TEST_P(EntityFactoryTest, MissingDependency) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<MissingDependencySystem>();
  PORT_EXPECT_DEBUG_DEATH(this->InitializeEntityFactory(), "");
}

TYPED_TEST_P(EntityFactoryTest, CreateFromNullData) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  PORT_EXPECT_DEBUG_DEATH(entity_factory->CreateFromBlueprint(nullptr, "test"),
                          "");
}

TYPED_TEST_P(EntityFactoryTest, CreateFromNullBlueprint) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  Blueprint* blueprint = nullptr;
  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(blueprint), "");
}

TYPED_TEST_P(EntityFactoryTest, CreateNullEntity) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  const Entity entity = entity_factory->Create(kNullEntity, "blueprint");
  EXPECT_THAT(entity, Eq(kNullEntity));
}

TYPED_TEST_P(EntityFactoryTest, LoadNonExistantBlueprint) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  const Entity entity1 = entity_factory->Create("blueprint");
  EXPECT_THAT(entity1, Eq(kNullEntity));

  const Entity entity2 = entity_factory->Create();
  const Entity entity3 = entity_factory->Create(entity2, "blueprint");
  EXPECT_THAT(entity3, Eq(kNullEntity));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromFlatbuffer) {
  using EntityDef = typename TypeParam::entity_type;
  using ComponentDef = typename TypeParam::component_type;
  using EntityDefBuilder = typename TypeParam::entity_builder_type;
  using ComponentDefBuilder = typename TypeParam::component_builder_type;

  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Create a flatbuffer EntityDef with ValueDef and ComplexDef components.
  flatbuffers::FlatBufferBuilder fbb;
  {
    std::vector<flatbuffers::Offset<ComponentDef>> components;
    {
      auto value_def_offset = CreateValueDefDirect(fbb, "hello world", 42);
      ComponentDefBuilder value_component_builder(fbb);
      value_component_builder.add_def_type(
          TypeParam::template component_def_type_value<ValueDef>());
      value_component_builder.add_def(value_def_offset.Union());
      components.push_back(value_component_builder.Finish());
    }
    {
      auto complex_def_offset =
          CreateComplexDefDirect(fbb, "foo bar baz", CreateIntData(fbb, 256));
      ComponentDefBuilder complex_component_builder(fbb);
      complex_component_builder.add_def_type(
          TypeParam::template component_def_type_value<ComplexDef>());
      complex_component_builder.add_def(complex_def_offset.Union());
      components.push_back(complex_component_builder.Finish());
    }
    auto components_offset = fbb.CreateVector(components);
    EntityDefBuilder entity_builder(fbb);
    entity_builder.add_components(components_offset);
    fbb.Finish(entity_builder.Finish());
  }
  this->fake_file_system_.SaveToDisk("test_entity.bin", fbb.GetBufferPointer(),
                                     fbb.GetSize());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromBlueprint) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Create a blueprint with ValueDef and ComplexDef components.
  Blueprint blueprint;
  ValueDefT value;
  ComplexDefT complex;
  value.name = "hello world";
  value.value = 42;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint.Write(&value);
  blueprint.Write(&complex);

  const Entity entity = entity_factory->Create(&blueprint);
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromFinalizedBlueprint) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Create a blueprint with ValueDef and ComplexDef components, finalize it,
  // then save the finalized blueprint to disk.
  Blueprint blueprint;
  ValueDefT value;
  ComplexDefT complex;
  value.name = "hello world";
  value.value = 42;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint.Write(&value);
  blueprint.Write(&complex);
  auto data = entity_factory->Finalize(&blueprint);
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));
}

TYPED_TEST_P(EntityFactoryTest, Destroy) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  ValueDefT value_def;

  Blueprint blueprint1;
  value_def.name = "hello";
  blueprint1.Write(&value_def);
  const Entity entity1 = entity_factory->Create(&blueprint1);

  Blueprint blueprint2;
  value_def.name = "world";
  blueprint2.Write(&value_def);
  const Entity entity2 = entity_factory->Create(&blueprint2);

  EXPECT_THAT(system->GetSimpleName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->Destroy(kNullEntity);
  EXPECT_THAT(system->GetSimpleName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->Destroy(entity1);
  EXPECT_THAT(system->GetSimpleName(entity1), Eq(""));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->Destroy(entity2);
  EXPECT_THAT(system->GetSimpleName(entity1), Eq(""));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq(""));
}

TYPED_TEST_P(EntityFactoryTest, QueuedDestroy) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  ValueDefT value_def;

  Blueprint blueprint1;
  value_def.name = "hello";
  blueprint1.Write(&value_def);
  const Entity entity1 = entity_factory->Create(&blueprint1);

  Blueprint blueprint2;
  value_def.name = "world";
  blueprint2.Write(&value_def);
  const Entity entity2 = entity_factory->Create(&blueprint2);

  EXPECT_THAT(system->GetSimpleName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->QueueForDestruction(kNullEntity);
  EXPECT_THAT(system->GetSimpleName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->DestroyQueuedEntities();
  EXPECT_THAT(system->GetSimpleName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->QueueForDestruction(entity1);
  EXPECT_THAT(system->GetSimpleName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->DestroyQueuedEntities();
  EXPECT_THAT(system->GetSimpleName(entity1), Eq(""));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->QueueForDestruction(entity2);
  EXPECT_THAT(system->GetSimpleName(entity1), Eq(""));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("world"));

  entity_factory->DestroyQueuedEntities();
  EXPECT_THAT(system->GetSimpleName(entity1), Eq(""));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq(""));
}

TYPED_TEST_P(EntityFactoryTest, GetEntityToBlueprintMap) {
  using EntityDef = typename TypeParam::entity_type;
  using ComponentDef = typename TypeParam::component_type;

  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();
  auto& map = entity_factory->GetEntityToBlueprintMap();

  ValueDefT value_def;

  // Create a flatbuffer EntityDef with a ValueDef component.  Save the
  // flatbuffer to disk and attempt to create an Entity from that saved
  // blueprint.
  Blueprint blueprint1;
  value_def.name = "hello";
  blueprint1.Write(&value_def);
  auto data1 = entity_factory->Finalize(&blueprint1);
  this->fake_file_system_.SaveToDisk("one.bin", data1.data(), data1.size());
  const Entity entity1 = entity_factory->Create("one");

  Blueprint blueprint2;
  value_def.name = "world";
  blueprint2.Write(&value_def);
  auto data2 = entity_factory->Finalize(&blueprint2);
  this->fake_file_system_.SaveToDisk("two.bin", data2.data(), data2.size());
  const Entity entity2 = entity_factory->Create("two");

  EXPECT_THAT(map.size(), Eq(static_cast<size_t>(2)));
  EXPECT_THAT(map.count(entity1), Eq(static_cast<size_t>(1)));
  EXPECT_THAT(map.count(entity2), Eq(static_cast<size_t>(1)));
  EXPECT_THAT(map.find(entity1)->second, Eq("one"));
  EXPECT_THAT(map.find(entity2)->second, Eq("two"));

  entity_factory->Destroy(entity1);
  EXPECT_THAT(map.size(), Eq(static_cast<size_t>(1)));
  EXPECT_THAT(map.count(entity1), Eq(static_cast<size_t>(0)));
  EXPECT_THAT(map.count(entity2), Eq(static_cast<size_t>(1)));

  entity_factory->Destroy(entity2);
  EXPECT_THAT(map.size(), Eq(static_cast<size_t>(0)));
  EXPECT_THAT(map.count(entity1), Eq(static_cast<size_t>(0)));
  EXPECT_THAT(map.count(entity2), Eq(static_cast<size_t>(0)));
}

TYPED_TEST_P(EntityFactoryTest, MultipleSchemas) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();
  entity_factory->template RegisterFlatbufferConverter<
      lull::testing2::EntityDef, lull::testing2::ComponentDef>(
      lull::testing2::GetEntityDef, lull::testing2::EnumNamesComponentDefType(),
      "TEST");

  // Create a flatbuffer EntityDef with a ValueDef component.  Save the
  // flatbuffer to disk and attempt to create an Entity from that saved
  // blueprint.
  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<lull::testing::ComponentDef>> components;
  auto name_def = CreateValueDefDirect(fbb, "hello", 1);
  components.push_back(
      CreateComponentDef(fbb, ComponentDefType_ValueDef, name_def.Union()));
  FinishEntityDefBuffer(fbb, CreateEntityDefDirect(fbb, &components));
  this->fake_file_system_.SaveToDisk("test_entity.bin", fbb.GetBufferPointer(),
                                     fbb.GetSize());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(1));

  // Do the same as above, but with the 2nd entity schema
  flatbuffers::FlatBufferBuilder fbb2;
  std::vector<flatbuffers::Offset<lull::testing2::ComponentDef>> components2;
  auto name_def2 = CreateValueDefDirect(fbb2, "hello2", 2);
  components2.push_back(lull::testing2::CreateComponentDef(
      fbb2, lull::testing2::ComponentDefType_ValueDef, name_def2.Union()));
  lull::testing2::FinishEntityDefBuffer(
      fbb2, lull::testing2::CreateEntityDefDirect(fbb2, &components2));
  this->fake_file_system_.SaveToDisk("test_entity2.bin",
                                     fbb2.GetBufferPointer(), fbb2.GetSize());

  const Entity entity2 = entity_factory->Create("test_entity2");
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("hello2"));
  EXPECT_THAT(system->GetSimpleValue(entity2), Eq(2));
}

REGISTER_TYPED_TEST_CASE_P(EntityFactoryTest, NoSystems, MissingDependency,
                           CreateFromNullData, CreateFromNullBlueprint,
                           CreateNullEntity, LoadNonExistantBlueprint,
                           CreateFromFlatbuffer, CreateFromBlueprint,
                           CreateFromFinalizedBlueprint, Destroy, QueuedDestroy,
                           GetEntityToBlueprintMap, MultipleSchemas);

}  // namespace testing
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::testing::TestSystem);
LULLABY_SETUP_TYPEID(lull::testing::MissingDependencySystem);

#endif  // LULLABY_TESTS_UTIL_ENTITY_FACTORY_TEST_H_
