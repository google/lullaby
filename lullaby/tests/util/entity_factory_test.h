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

#ifndef LULLABY_TESTS_UTIL_ENTITY_FACTORY_TEST_H_
#define LULLABY_TESTS_UTIL_ENTITY_FACTORY_TEST_H_

#include "lullaby/modules/ecs/entity_factory.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/blueprint_builder.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"
#include "lullaby/tests/portable_test_macros.h"
#include "lullaby/tests/test_def_generated.h"
#include "lullaby/tests/test_entity2_generated.h"
#include "lullaby/tests/test_entity_generated.h"
#include "lullaby/tests/util/entity_test.h"
#include "lullaby/tests/util/fake_file_system.h"

namespace lull {
namespace testing {

using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsNull;
using ::testing::Not;
using ::testing::NotNull;

const HashValue kValueDefHash = Hash("ValueDef");
const HashValue kComplexDefHash = Hash("ComplexDef");

// System and Components for test component defs in order to give them testable
// runtime behavior.
class TestSystem : public System {
 public:
  enum RegisterMode {
    DefTypeHash,
    DefTTemplate
  };

  struct TestComponent : Component {
    explicit TestComponent(Entity e) : Component(e) {}

    std::string simple_name = "";
    int simple_value = 0;
    std::string complex_name = "";
    int complex_value = 0;

    Entity parent = kNullEntity;
  };

  explicit TestSystem(Registry* registry, RegisterMode mode = DefTypeHash)
      : System(registry), components_(1) {
    switch (mode) {
     case DefTypeHash:
      RegisterDef(this, kValueDefHash);
      RegisterDef(this, kComplexDefHash);
      break;
     case DefTTemplate:
      RegisterDef<ValueDefT>(this);
      RegisterDef<ComplexDefT>(this);
      break;
     default:
      break;
    }
  }

  void SetCreateChildFn() {
    EntityFactory* entity_factory = registry_->Get<EntityFactory>();
    if (entity_factory) {
    entity_factory->SetCreateChildFn(
        [this, entity_factory](Entity parent, BlueprintTree* bpt) -> Entity {
          const Entity child = entity_factory->Create();
          entity_factory->Create(child, bpt);

          auto* child_component = components_.Get(child);
          // We can't use ASSERT_THAT() in a lambda that returns a non-void.
          if (child_component == nullptr) {
            ADD_FAILURE() << "Did not create component.";
            return child;
          }

          child_component->parent = parent;
          return child;
        });
    }
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

  // TODO test order of create and post create.

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

  Entity GetParent(Entity e)  const {
    auto* component = components_.Get(e);
    return component ? component->parent : kNullEntity;
  }

  const ComponentPool<TestComponent>& GetComponents() { return components_; }

 private:
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
TYPED_TEST_SUITE_P(EntityFactoryTest);

template <typename TypeParam>
using EntityFactoryDeathTest = EntityFactoryTest<TypeParam>;
TYPED_TEST_SUITE_P(EntityFactoryDeathTest);

TYPED_TEST_P(EntityFactoryDeathTest, NoSystems) {
  PORT_EXPECT_DEBUG_DEATH(this->InitializeEntityFactory(), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, MissingDependency) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<MissingDependencySystem>();
  PORT_EXPECT_DEBUG_DEATH(this->InitializeEntityFactory(), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, MissingSystem) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  // TestSystem has not been created and isn't in Registry, so this doesn't add
  // anything.
  entity_factory->template AddSystemFromRegistry<TestSystem>();
  PORT_EXPECT_DEBUG_DEATH(this->InitializeEntityFactory(), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, MissingInitialize) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();

  // Create a blueprint with ValueDef and ComplexDef components, but fails to
  // create without initializing.
  Blueprint blueprint;
  ValueDefT value;
  ComplexDefT complex;
  value.name = "hello world";
  value.value = 42;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint.Write(&value);
  blueprint.Write(&complex);

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(&blueprint), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, CreateFromNullData) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  PORT_EXPECT_DEBUG_DEATH(
      entity_factory->CreateFromBlueprint(nullptr, 0, "test"), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, CreateFromNullBlueprint) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  Blueprint* blueprint = nullptr;
  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(blueprint), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, CreateNullEntity) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  const Entity entity = entity_factory->Create(kNullEntity, "blueprint");
  EXPECT_THAT(entity, Eq(kNullEntity));

  PORT_EXPECT_DEBUG_DEATH(
      entity_factory->CreateFromBlueprint(kNullEntity, nullptr, 0, "test"), "");

  BlueprintTree blueprint_tree;
  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(kNullEntity, &blueprint_tree),
                          "");
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
    fbb.Finish(entity_builder.Finish(), EntityFactory::kLegacyFileIdentifier);
  }
  this->fake_file_system_.SaveToDisk("test_entity.bin", fbb.GetBufferPointer(),
                                     fbb.GetSize());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
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
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromBlueprintRegisterDefTTemplate) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>(
      TestSystem::DefTTemplate);
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
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromBlueprintTree) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Create a blueprint tree with ValueDef, and a child with ComplexDef.
  BlueprintTree blueprint;
  BlueprintTree* blueprint_child = blueprint.NewChild();
  ValueDefT value;
  value.name = "hello world";
  value.value = 42;
  blueprint.Write(&value);
  ComplexDefT complex;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint_child->Write(&complex);

  const Entity entity = entity_factory->Create(&blueprint);
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));

  bool found = false;
  system->GetComponents().ForEach(
      [&](const TestSystem::TestComponent& component) {
        if (component.complex_name.compare("foo bar baz") == 0 &&
            component.complex_value == 256) {
          found = true;
        }
      });
  EXPECT_THAT(found, Eq(true));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromBlueprintTreeWithEntity) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Create a blueprint tree with ValueDef, and a child with ComplexDef.
  BlueprintTree blueprint;
  BlueprintTree* blueprint_child = blueprint.NewChild();
  ValueDefT value;
  value.name = "hello world";
  value.value = 42;
  blueprint.Write(&value);
  ComplexDefT complex;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint_child->Write(&complex);

  const Entity entity = entity_factory->Create();
  bool found = false;
  auto find_child = [&](const TestSystem::TestComponent& component) {
    if (component.complex_name.compare("foo bar baz") == 0 &&
        component.complex_value == 256) {
      found = true;
    }
  };
  EXPECT_THAT(system->GetSimpleName(entity), Eq(""));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(0));
  system->GetComponents().ForEach(find_child);
  EXPECT_THAT(found, Eq(false));

  const Entity entity_ = entity_factory->Create(entity, &blueprint);
  EXPECT_THAT(entity_, Eq(entity));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  system->GetComponents().ForEach(find_child);
  EXPECT_THAT(found, Eq(true));
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
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));

  // Also create with existing entity.
  const Entity entity2 = entity_factory->Create();
  EXPECT_THAT(entity2, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq(""));
  EXPECT_THAT(system->GetSimpleValue(entity2), Eq(0));
  EXPECT_THAT(system->GetComplexName(entity2), Eq(""));
  EXPECT_THAT(system->GetComplexValue(entity2), Eq(0));
  const Entity entity2_ = entity_factory->Create(entity2, "test_entity");
  EXPECT_THAT(entity2_, Eq(entity2));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity2), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity2), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity2), Eq(256));

  // Also create with data directly.
  const Entity entity3 =
      entity_factory->CreateFromBlueprint(data.data(), data.size(), "test");
  EXPECT_THAT(entity3, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity3), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity3), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity3), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity3), Eq(256));

  // Also create with existing entity and data directly.
  const Entity entity4 = entity_factory->Create();
  EXPECT_THAT(entity4, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity4), Eq(""));
  EXPECT_THAT(system->GetSimpleValue(entity4), Eq(0));
  EXPECT_THAT(system->GetComplexName(entity4), Eq(""));
  EXPECT_THAT(system->GetComplexValue(entity4), Eq(0));
  const bool result = entity_factory->CreateFromBlueprint(entity4, data.data(),
                                                          data.size(), "test2");
  EXPECT_TRUE(result);
  EXPECT_THAT(system->GetSimpleName(entity4), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity4), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity4), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity4), Eq(256));
}

TYPED_TEST_P(EntityFactoryTest, CreateFromFinalizedBlueprintTree) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>(
      TestSystem::DefTTemplate);
  this->InitializeEntityFactory();

  // Create a blueprint tree with ValueDef, and a child with ComplexDef.
  // Finalize it, then save the finalized blueprint to disk.
  BlueprintTree blueprint_tree;
  BlueprintTree* blueprint_child = blueprint_tree.NewChild();
  ValueDefT value;
  value.name = "hello world";
  value.value = 42;
  blueprint_tree.Write(&value);
  ComplexDefT complex;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint_child->Write(&complex);
  auto data = entity_factory->Finalize(&blueprint_tree);
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());


  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));

  bool found = false;
  system->GetComponents().ForEach(
      [&](const TestSystem::TestComponent& component) {
        if (component.complex_name.compare("foo bar baz") == 0 &&
            component.complex_value == 256) {
          found = true;
        }
      });
  EXPECT_THAT(found, Eq(true));
}

TYPED_TEST_P(EntityFactoryTest, CreateBlueprintFromBuilder) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>(
      TestSystem::DefTTemplate);
  this->InitializeEntityFactory();

  // Legacy entities are still creatable while having BlueprintDef entities, but
  // they need the correct identifier "ENTS", which is default for Finalize().
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
  const string_view identifier(
      flatbuffers::GetBufferIdentifier(data.data()),
      flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  EXPECT_THAT(identifier, Eq(EntityFactory::kLegacyFileIdentifier));
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));

  // BlueprintDef entities are created with the raw binary components, and the
  // Builder will add the correct identifier "BLPT".
  detail::BlueprintBuilder builder;
  flatbuffers::FlatBufferBuilder fbb;
  {
    auto value_def_offset = CreateValueDefDirect(fbb, "cat dog", 64);
    fbb.Finish(value_def_offset);
    builder.AddComponent("ValueDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  {
    auto complex_def_offset =
        CreateComplexDefDirect(fbb, "meow bark", CreateIntData(fbb, 123));
    fbb.Finish(complex_def_offset);
    builder.AddComponent("ComplexDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  flatbuffers::DetachedBuffer data2 = builder.Finish();
  const string_view identifier2(
      flatbuffers::GetBufferIdentifier(data2.data()),
      flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  EXPECT_THAT(data2.size(), Gt(0));
  EXPECT_THAT(identifier2,
              Eq(detail::BlueprintBuilder::kBlueprintFileIdentifier));
  this->fake_file_system_.SaveToDisk("test_entity2.bin", data2.data(),
                                     data2.size());

  const Entity entity2 = entity_factory->Create("test_entity2");
  EXPECT_THAT(entity2, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("cat dog"));
  EXPECT_THAT(system->GetSimpleValue(entity2), Eq(64));
  EXPECT_THAT(system->GetComplexName(entity2), Eq("meow bark"));
  EXPECT_THAT(system->GetComplexValue(entity2), Eq(123));

  // The builder is reusable to create multiple entities.
  {
    auto value_def_offset = CreateValueDefDirect(fbb, "cow sheep", 32);
    fbb.Finish(value_def_offset);
    builder.AddComponent("ValueDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  {
    auto complex_def_offset =
        CreateComplexDefDirect(fbb, "moo baa", CreateIntData(fbb, 111));
    fbb.Finish(complex_def_offset);
    builder.AddComponent("ComplexDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  flatbuffers::DetachedBuffer data3 = builder.Finish();
  EXPECT_THAT(data3.size(), Gt(0));
  this->fake_file_system_.SaveToDisk("test_entity3.bin", data3.data(),
                                     data3.size());

  const Entity entity3 = entity_factory->Create("test_entity3");
  EXPECT_THAT(entity3, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity3), Eq("cow sheep"));
  EXPECT_THAT(system->GetSimpleValue(entity3), Eq(32));
  EXPECT_THAT(system->GetComplexName(entity3), Eq("moo baa"));
  EXPECT_THAT(system->GetComplexValue(entity3), Eq(111));
}

TYPED_TEST_P(EntityFactoryTest, CreateNestedBlueprintFromBuilder) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>(
      TestSystem::DefTTemplate);
  this->InitializeEntityFactory();
  system->SetCreateChildFn();

  detail::BlueprintBuilder builder;
  flatbuffers::FlatBufferBuilder fbb;
  auto create_component = [&fbb, &builder](char* simple_name,
                                           int simple_value) {
    auto value_def_offset =
        CreateValueDefDirect(fbb, simple_name, simple_value);
    fbb.Finish(value_def_offset);
    builder.AddComponent("ValueDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  };

  // BlueprintDef entities can be nested, and parent-child relationships can be
  // created with SetCreateChildFn(). This creates a hierarchy:
  //   A -> D
  //     -> B -> C
  {
    builder.StartChildren();
    create_component("D", 4);
    EXPECT_TRUE(builder.FinishChild());
    {
      builder.StartChildren();
      create_component("C", 3);
      EXPECT_TRUE(builder.FinishChild());
      EXPECT_TRUE(builder.FinishChildren());
    }
    create_component("B", 2);
    EXPECT_TRUE(builder.FinishChild());
    EXPECT_TRUE(builder.FinishChildren());
  }
  create_component("A", 1);
  flatbuffers::DetachedBuffer data = builder.Finish();
  EXPECT_THAT(data.size(), Gt(0));
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("A"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(1));

  std::unordered_map<int, int> child_to_parent;
  system->GetComponents().ForEach(
      [&](const TestSystem::TestComponent& component) {
        child_to_parent.emplace(
            component.simple_value,
            system->GetSimpleValue(system->GetParent(component.GetEntity())));
      });
  EXPECT_THAT(child_to_parent,
              Eq(std::unordered_map<int, int>{{1, 0}, {2, 1}, {3, 2}, {4, 1}}));
}

TYPED_TEST_P(EntityFactoryDeathTest,
             CreateBlueprintFromBuilderRegisterDefTypeHash) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  detail::BlueprintBuilder builder;
  flatbuffers::FlatBufferBuilder fbb;
  {
    auto value_def_offset = CreateValueDefDirect(fbb, "cat dog", 64);
    fbb.Finish(value_def_offset);
    builder.AddComponent("ValueDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  {
    auto complex_def_offset =
        CreateComplexDefDirect(fbb, "meow bark", CreateIntData(fbb, 123));
    fbb.Finish(complex_def_offset);
    builder.AddComponent("ComplexDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  flatbuffers::DetachedBuffer data = builder.Finish();
  EXPECT_THAT(data.size(), Gt(0));
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create("test_entity"), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, CreateBlueprintFromBuilderUnknown) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>(
      TestSystem::DefTTemplate);
  this->InitializeEntityFactory();

  detail::BlueprintBuilder builder;
  flatbuffers::FlatBufferBuilder fbb;
  {
    auto value_def_offset = CreateValueDefDirect(fbb, "cat dog", 64);
    fbb.Finish(value_def_offset);
    builder.AddComponent("ValueDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  {
    auto complex_def_offset =
        CreateComplexDefDirect(fbb, "meow bark", CreateIntData(fbb, 123));
    fbb.Finish(complex_def_offset);
    builder.AddComponent("ComplexDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  {
    auto unknown_def_offset =
        CreateUnknownDefDirect(fbb, "missingno", -1);
    fbb.Finish(unknown_def_offset);
    builder.AddComponent("UnknownDef", {fbb.GetBufferPointer(), fbb.GetSize()});
    fbb.Clear();
  }
  flatbuffers::DetachedBuffer data = builder.Finish();
  EXPECT_THAT(data.size(), Gt(0));
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create("test_entity"), "");
}

TYPED_TEST_P(EntityFactoryTest, BlueprintBuilderErrors) {
  {
    // FinishChild() must be between StartChildren() and FinishChildren().
    detail::BlueprintBuilder builder;
    builder.StartChildren();
    EXPECT_TRUE(builder.FinishChild());
    EXPECT_TRUE(builder.FinishChildren());
    EXPECT_FALSE(builder.FinishChild());
  }
  {
    // FinishChildren() must be balanced with StartChildren().
    detail::BlueprintBuilder builder;
    builder.StartChildren();
    EXPECT_TRUE(builder.FinishChildren());
    EXPECT_FALSE(builder.FinishChildren());
  }
  {
    // StartChildren() must be balanced with FinishChildren().
    detail::BlueprintBuilder builder;
    builder.StartChildren();
    auto data = builder.Finish();
    EXPECT_THAT(data.size(), Eq(0));
  }
}

TYPED_TEST_P(EntityFactoryDeathTest, UnknownComponentDef) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Try to create a blueprint with ValueDef, ComplexDef and UnknownDef
  // components, fails.
  Blueprint blueprint;
  ValueDefT value;
  ComplexDefT complex;
  UnknownDefT unknown;
  value.name = "hello world";
  value.value = 42;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  unknown.name = "missingno";
  unknown.value = -1;
  blueprint.Write(&value);
  blueprint.Write(&complex);
  blueprint.Write(&unknown);
  auto data = entity_factory->Finalize(&blueprint);
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create("test_entity"), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, UnknownSystem) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  // Overwrite the system for ValueDef to a null value.
  entity_factory->RegisterDef(0, kValueDefHash);
  this->InitializeEntityFactory();

  // Try to create a blueprint with ValueDef, which will fail.
  Blueprint blueprint;
  ValueDefT value;
  value.name = "hello world";
  value.value = 42;
  blueprint.Write(&value);

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(&blueprint), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, UnknownSystemRegisterDefTTemplate) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>(
      TestSystem::DefTTemplate);
  // Overwrite the system for ValueDef to a null value.
  entity_factory->template RegisterDef<ValueDefT>(0);
  this->InitializeEntityFactory();

  // Try to create a blueprint with ValueDef, which will fail.
  Blueprint blueprint;
  ValueDefT value;
  value.name = "hello world";
  value.value = 42;
  blueprint.Write(&value);

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(&blueprint), "");
}

TYPED_TEST_P(EntityFactoryDeathTest, CreateFromBadBlueprint) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Save some bad data to disk, which will fail to create.
  uint8_t bad[16];
  memset(bad, 0, sizeof(bad));
  this->fake_file_system_.SaveToDisk("test_entity.bin", bad, sizeof(bad));

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create("test_entity"), "");
}

TYPED_TEST_P(EntityFactoryTest, CreateFromBadBlueprintCorrectIdentifier) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  // Same as before, but also save a valid identifier. This will pass the
  // identifier check, but should fail to verify.
  uint8_t bad[16];
  memset(bad, 0, sizeof(bad));
  char* identifier = const_cast<char*>(flatbuffers::GetBufferIdentifier(bad));
  memcpy(identifier, EntityFactory::kLegacyFileIdentifier,
         flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  this->fake_file_system_.SaveToDisk("test_entity.bin", bad, sizeof(bad));

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(entity, Eq(kNullEntity));

  const Entity entity2 = entity_factory->Create();
  const Entity entity3 = entity_factory->Create(entity2, "test_entity");
  EXPECT_THAT(entity3, Eq(kNullEntity));
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
  EXPECT_THAT(entity_factory->GetFlatbufferConverterCount(),
              Eq(static_cast<size_t>(0)));

  this->InitializeEntityFactory();
  EXPECT_THAT(entity_factory->GetFlatbufferConverterCount(),
              Eq(static_cast<size_t>(2)));

  entity_factory->template RegisterFlatbufferConverter<
      lull::testing2::EntityDef, lull::testing2::ComponentDef>(
      lull::testing2::GetEntityDef, lull::testing2::EnumNamesComponentDefType(),
      "TEST");
  EXPECT_THAT(entity_factory->GetFlatbufferConverterCount(),
              Eq(static_cast<size_t>(3)));

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

TYPED_TEST_P(EntityFactoryDeathTest, FinalizeWrongIdentifier) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();

  Blueprint blueprint;
  ValueDefT value;
  ComplexDefT complex;
  value.name = "hello world";
  value.value = 42;
  complex.name = "foo bar baz";
  complex.data.value = 256;
  blueprint.Write(&value);
  blueprint.Write(&complex);
  PORT_EXPECT_DEBUG_DEATH(entity_factory->Finalize(&blueprint, "UNKN"), "");
}

TYPED_TEST_P(EntityFactoryTest, FinalizeMultipleSchemas) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();
  entity_factory->template RegisterFlatbufferConverter<
      lull::testing2::EntityDef, lull::testing2::ComponentDef>(
      lull::testing2::GetEntityDef, lull::testing2::EnumNamesComponentDefType(),
      "TEST");

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
  const string_view identifier(
      flatbuffers::GetBufferIdentifier(data.data()),
      flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  EXPECT_THAT(identifier, Eq(EntityFactory::kLegacyFileIdentifier));
  this->fake_file_system_.SaveToDisk("test_entity.bin", data.data(),
                                     data.size());

  const Entity entity = entity_factory->Create("test_entity");
  EXPECT_THAT(entity, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity), Eq(256));

  auto data2 = entity_factory->Finalize(&blueprint, "TEST");
  const string_view identifier2(
      flatbuffers::GetBufferIdentifier(data2.data()),
      flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  EXPECT_THAT(identifier2, Eq("TEST"));
  this->fake_file_system_.SaveToDisk("test_entity2.bin", data2.data(),
                                     data2.size());

  const Entity entity2 = entity_factory->Create("test_entity2");
  EXPECT_THAT(entity2, Not(Eq(kNullEntity)));
  EXPECT_THAT(system->GetSimpleName(entity2), Eq("hello world"));
  EXPECT_THAT(system->GetSimpleValue(entity2), Eq(42));
  EXPECT_THAT(system->GetComplexName(entity2), Eq("foo bar baz"));
  EXPECT_THAT(system->GetComplexValue(entity2), Eq(256));
}

TYPED_TEST_P(EntityFactoryDeathTest, UnknownSchema) {
  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();
  entity_factory->template RegisterFlatbufferConverter<
      lull::testing2::EntityDef, lull::testing2::ComponentDef>(
      lull::testing2::GetEntityDef, lull::testing2::EnumNamesComponentDefType(),
      "UNKN");  // This is purposely wrong, it is supposed to be "TEST"

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

  // Do the same as above, but with the 2nd entity schema and it will have a
  // wrong identifier.
  flatbuffers::FlatBufferBuilder fbb2;
  std::vector<flatbuffers::Offset<lull::testing2::ComponentDef>> components2;
  auto name_def2 = CreateValueDefDirect(fbb2, "hello2", 2);
  components2.push_back(lull::testing2::CreateComponentDef(
      fbb2, lull::testing2::ComponentDefType_ValueDef, name_def2.Union()));
  lull::testing2::FinishEntityDefBuffer(
      fbb2, lull::testing2::CreateEntityDefDirect(fbb2, &components2));
  this->fake_file_system_.SaveToDisk("test_entity2.bin",
                                     fbb2.GetBufferPointer(), fbb2.GetSize());

  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create("test_entity2"), "");
}

TYPED_TEST_P(EntityFactoryTest, CreateBlueprint) {
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

  Optional<BlueprintTree> wrong_name =
      entity_factory->CreateBlueprint("wrong_name");
  EXPECT_FALSE(wrong_name);

  Optional<BlueprintTree> result =
      entity_factory->CreateBlueprint("test_entity");
  EXPECT_TRUE(result);

  int count = 0;
  result->ForEachComponent([&](const Blueprint& blueprint) {
    if (count == 0) {
      EXPECT_TRUE(blueprint.Is<ValueDefT>());
      ValueDefT value;
      blueprint.Read(&value);
      EXPECT_THAT(value.name, Eq("hello world"));
      EXPECT_THAT(value.value, Eq(42));
    } else if (count == 1) {
      EXPECT_TRUE(blueprint.Is<ComplexDefT>());
      ComplexDefT complex;
      blueprint.Read(&complex);
      EXPECT_THAT(complex.name, Eq("foo bar baz"));
      EXPECT_THAT(complex.data.value, Eq(256));
    }
    ++count;
  });
  EXPECT_THAT(count, Eq(2));
  EXPECT_TRUE(result->Children()->empty());
}

TYPED_TEST_P(EntityFactoryDeathTest, CreateBlueprintWithoutInitialize) {
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

  // Now, create a new Registry and EntityFactory without Initialize and it
  // should fail to read from disk (same fake file system). The original
  // EntityFactory needed Initialize to use Finalize.
  Registry second_registry;
  second_registry.Create<AssetLoader>(
      [this](const char* name, std::string* out) -> bool {
        return this->fake_file_system_.LoadFromDisk(name, out);
      });
  second_registry.Create<EntityFactory>(&second_registry);

  PORT_EXPECT_DEBUG_DEATH(
      second_registry.Get<EntityFactory>()->CreateBlueprint("test_entity"), "");
}

TYPED_TEST_P(EntityFactoryTest, CreateBlueprintTree) {
  using EntityDef = typename TypeParam::entity_type;
  using ComponentDef = typename TypeParam::component_type;
  using EntityDefBuilder = typename TypeParam::entity_builder_type;
  using ComponentDefBuilder = typename TypeParam::component_builder_type;

  auto entity_factory = this->registry_.template Get<EntityFactory>();
  auto* system = entity_factory->template CreateSystem<TestSystem>();
  this->InitializeEntityFactory();


  // Create a child EntityDef with ValueDef and parent EntityDef with ComplexDef
  // and the child.
  flatbuffers::FlatBufferBuilder fbb;
  {
    std::vector<flatbuffers::Offset<EntityDef>> children;
    {
      std::vector<flatbuffers::Offset<ComponentDef>> components;
      {
        auto value_def_offset = CreateValueDefDirect(fbb, "child", 42);
        ComponentDefBuilder value_component_builder(fbb);
        value_component_builder.add_def_type(
            TypeParam::template component_def_type_value<ValueDef>());
        value_component_builder.add_def(value_def_offset.Union());
        components.push_back(value_component_builder.Finish());
      }
      auto components_offset = fbb.CreateVector(components);
      EntityDefBuilder entity_builder(fbb);
      entity_builder.add_components(components_offset);
      children.push_back(entity_builder.Finish());
    }

    std::vector<flatbuffers::Offset<ComponentDef>> components;
    {
      auto complex_def_offset =
          CreateComplexDefDirect(fbb, "parent", CreateIntData(fbb, 256));
      ComponentDefBuilder complex_component_builder(fbb);
      complex_component_builder.add_def_type(
          TypeParam::template component_def_type_value<ComplexDef>());
      complex_component_builder.add_def(complex_def_offset.Union());
      components.push_back(complex_component_builder.Finish());
    }
    auto components_offset = fbb.CreateVector(components);
    auto children_offset = fbb.CreateVector(children);
    EntityDefBuilder entity_builder(fbb);
    entity_builder.add_components(components_offset);
    entity_builder.add_children(children_offset);
    fbb.Finish(entity_builder.Finish(), EntityFactory::kLegacyFileIdentifier);
  }
  this->fake_file_system_.SaveToDisk("test_entity.bin", fbb.GetBufferPointer(),
                                     fbb.GetSize());

  Optional<BlueprintTree> result =
      entity_factory->CreateBlueprint("test_entity");
  EXPECT_TRUE(result);

  int count = 0;
  result->ForEachComponent([&](const Blueprint& blueprint) {
    EXPECT_TRUE(blueprint.Is<ComplexDefT>());
    ComplexDefT parent;
    blueprint.Read(&parent);
    EXPECT_THAT(parent.name, Eq("parent"));
    EXPECT_THAT(parent.data.value, Eq(256));
    ++count;
  });
  EXPECT_THAT(count, Eq(1));
  const auto children = result->Children();
  EXPECT_THAT(children->size(), Eq(static_cast<size_t>(1)));
  children->front().ForEachComponent([&](const Blueprint& blueprint) {
    EXPECT_TRUE(blueprint.Is<ValueDefT>());
    ValueDefT child;
    blueprint.Read(&child);
    EXPECT_THAT(child.name, Eq("child"));
    EXPECT_THAT(child.value, Eq(42));
    ++count;
  });
  EXPECT_THAT(count, Eq(2));
  EXPECT_TRUE(children->front().Children()->empty());
}

REGISTER_TYPED_TEST_SUITE_P(
    EntityFactoryTest, LoadNonExistantBlueprint, CreateFromFlatbuffer,
    CreateFromBlueprint, CreateFromBlueprintRegisterDefTTemplate,
    CreateFromBlueprintTree, CreateFromBlueprintTreeWithEntity,
    CreateFromFinalizedBlueprint, CreateFromFinalizedBlueprintTree,
    CreateBlueprintFromBuilder, CreateNestedBlueprintFromBuilder,
    BlueprintBuilderErrors, CreateFromBadBlueprintCorrectIdentifier, Destroy,
    QueuedDestroy, GetEntityToBlueprintMap, MultipleSchemas,
    FinalizeMultipleSchemas, CreateBlueprint, CreateBlueprintTree);

REGISTER_TYPED_TEST_SUITE_P(
    EntityFactoryDeathTest, NoSystems, MissingDependency, MissingSystem,
    MissingInitialize, CreateFromNullData, CreateFromNullBlueprint,
    CreateNullEntity, CreateBlueprintFromBuilderRegisterDefTypeHash,
    CreateBlueprintFromBuilderUnknown, UnknownComponentDef, UnknownSystem,
    UnknownSystemRegisterDefTTemplate, CreateFromBadBlueprint,
    FinalizeWrongIdentifier, UnknownSchema, CreateBlueprintWithoutInitialize);

}  // namespace testing
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::testing::TestSystem);
LULLABY_SETUP_TYPEID(lull::testing::MissingDependencySystem);

#endif  // LULLABY_TESTS_UTIL_ENTITY_FACTORY_TEST_H_
