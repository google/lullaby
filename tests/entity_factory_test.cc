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

#include "lullaby/base/entity_factory.h"

#include <unordered_set>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/base/asset_loader.h"
#include "lullaby/systems/datastore/datastore_system.h"
#include "lullaby/systems/name/name_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/tests/portable_test_macros.h"
#include "lullaby/generated/tests/test_entity_generated.h"

namespace lull {

class MissingDependencySystem : public System {
 public:
  explicit MissingDependencySystem(Registry* registry) : System(registry) {
    RegisterDependency<NameSystem>(this);
  }
};

namespace {

using ::testing::Eq;
using ::testing::Ne;

class EntityFactoryTest : public ::testing::Test {
 protected:
  using DataBuffer = std::vector<uint8_t>;

  EntityFactoryTest() {
    auto loader = [this](const char* filename, std::string* out) -> bool {
      return LoadFromDisk(filename, out);
    };
    registry_.Create<AssetLoader>(loader);
    registry_.Create<EntityFactory>(&registry_);
  }

  void SaveToDisk(const std::string& name, const uint8_t* data, size_t len) {
    filesystem_[name] = DataBuffer(data, data + len);
  }

  bool LoadFromDisk(const std::string& name, std::string* out) {
    auto iter = filesystem_.find(name);
    if (iter == filesystem_.end()) {
      return false;
    }

    const DataBuffer& data = iter->second;
    out->resize(data.size());
    memcpy(const_cast<char*>(out->data()), data.data(), data.size());
    return true;
  }

  void InitializeEntityFactory() {
    auto entity_factory = registry_.Get<EntityFactory>();
    entity_factory->Initialize<EntityDef, ComponentDef>(
        GetEntityDef, EnumNamesComponentDefType());
  }

  Registry registry_;
  std::unordered_map<std::string, DataBuffer> filesystem_;
};

using EntityFactoryDeathTest = EntityFactoryTest;

TEST_F(EntityFactoryDeathTest, NoSystems) {
  PORT_EXPECT_DEBUG_DEATH(InitializeEntityFactory(), "");
}

TEST_F(EntityFactoryDeathTest, MissingDependency) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<MissingDependencySystem>();
  PORT_EXPECT_DEBUG_DEATH(InitializeEntityFactory(), "");
}

TEST_F(EntityFactoryDeathTest, CreateFromNullData) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  PORT_EXPECT_DEBUG_DEATH(entity_factory->CreateFromBlueprint(nullptr, "test"),
                          "");
}

TEST_F(EntityFactoryDeathTest, CreateFromNullBlueprint) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  Blueprint* blueprint = nullptr;
  PORT_EXPECT_DEBUG_DEATH(entity_factory->Create(blueprint), "");
}

TEST_F(EntityFactoryTest, CreateNullEntity) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  const Entity entity = entity_factory->Create(kNullEntity, "blueprint");
  EXPECT_THAT(entity, Eq(kNullEntity));
}

TEST_F(EntityFactoryTest, LoadNonExistantBlueprint) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  const Entity entity1 = entity_factory->Create("blueprint");
  EXPECT_THAT(entity1, Eq(kNullEntity));

  const Entity entity2 = entity_factory->Create();
  const Entity entity3 = entity_factory->Create(entity2, "blueprint");
  EXPECT_THAT(entity3, Eq(kNullEntity));
}

TEST_F(EntityFactoryTest, CreateFromFlatbuffer) {
  auto entity_factory = registry_.Get<EntityFactory>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  // Create a flatbuffer EntityDef with a NameDef component.  Save the
  // flatbuffer to disk and attempt to create an Entity from that saved
  // blueprint.
  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<ComponentDef>> components;
  auto name_def = CreateNameDefDirect(fbb, "hello");
  components.push_back(
      CreateComponentDef(fbb, ComponentDefType_NameDef, name_def.Union()));
  FinishEntityDefBuffer(fbb, CreateEntityDefDirect(fbb, &components));
  SaveToDisk("test_entity.bin", fbb.GetBufferPointer(), fbb.GetSize());

  const Entity entity = entity_factory->Create("test_entity");
  const std::string entity_name = system->GetName(entity);
  EXPECT_THAT(entity_name, Eq("hello"));
}

TEST_F(EntityFactoryTest, CreateFromBlueprint) {
  auto entity_factory = registry_.Get<EntityFactory>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  // Create a blueprint with a NameDefT component.
  Blueprint blueprint;
  NameDefT name;
  name.name = "hello";
  blueprint.Write(&name);

  const Entity entity = entity_factory->Create(&blueprint);
  const std::string entity_name = system->GetName(entity);
  EXPECT_THAT(entity_name, Eq("hello"));
}

TEST_F(EntityFactoryTest, CreateFromFinalizedBlueprint) {
  auto entity_factory = registry_.Get<EntityFactory>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  // Create a blueprint with a NameDefT component.  Finalize and save that
  // blueprint to disk, and then attempt to load the Entity from that saved
  // blueprint.
  Blueprint blueprint;
  NameDefT name;
  name.name = "hello";
  blueprint.Write(&name);
  auto data = entity_factory->Finalize(&blueprint);
  SaveToDisk("test_entity.bin", data.data(), data.size());

  const Entity entity = entity_factory->Create("test_entity");
  const std::string entity_name = system->GetName(entity);
  EXPECT_THAT(entity_name, Eq("hello"));
}

TEST_F(EntityFactoryTest, CreateFromFlatbufferWithChildren) {
  // Create a flatbuffer EntityDef with a NameDef component.  Save the
  // flatbuffer to disk and attempt to create an Entity from that saved
  // blueprint.
  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<ComponentDef>> components;
  auto name_def = CreateNameDefDirect(fbb, "world");
  auto xform_def = CreateTransformDefDirect(fbb);
  components.push_back(CreateComponentDef(fbb, ComponentDefType_TransformDef,
                                          xform_def.Union()));
  components.push_back(
      CreateComponentDef(fbb, ComponentDefType_NameDef, name_def.Union()));
  auto child = CreateEntityDefDirect(fbb, &components);
  components.clear();
  name_def = CreateNameDefDirect(fbb, "hello");
  xform_def = CreateTransformDefDirect(fbb);
  components.push_back(CreateComponentDef(fbb, ComponentDefType_TransformDef,
                                          xform_def.Union()));
  components.push_back(
      CreateComponentDef(fbb, ComponentDefType_NameDef, name_def.Union()));
  std::vector<flatbuffers::Offset<EntityDef>> children;
  children.push_back(child);
  FinishEntityDefBuffer(fbb,
                        CreateEntityDefDirect(fbb, &components, &children));
  SaveToDisk("test_entity_with_children.bin", fbb.GetBufferPointer(),
             fbb.GetSize());

  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<TransformSystem>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  const Entity entity = entity_factory->Create("test_entity_with_children");
  const std::string entity_name = system->GetName(entity);
  EXPECT_THAT(entity_name, Eq("hello"));

  const Entity world = system->FindDescendant(entity, "world");
  EXPECT_THAT(world, Ne(kNullEntity));
  EXPECT_THAT(world, Ne(entity));
}

TEST_F(EntityFactoryTest, CreateFromBlueprintWithChildren) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<TransformSystem>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  // Create a blueprint with a NameDefT component.
  BlueprintTree blueprint;
  TransformDefT xform;
  NameDefT name;
  name.name = "hello";
  blueprint.Write(&xform);
  blueprint.Write(&name);
  auto* child = blueprint.NewChild();
  name.name = "world";
  child->Write(&xform);
  child->Write(&name);

  const Entity entity = entity_factory->Create(&blueprint);
  const std::string entity_name = system->GetName(entity);
  EXPECT_THAT(entity_name, Eq("hello"));

  const Entity world = system->FindDescendant(entity, "world");
  EXPECT_THAT(world, Ne(kNullEntity));
  EXPECT_THAT(world, Ne(entity));
}

TEST_F(EntityFactoryTest, Destroy) {
  auto entity_factory = registry_.Get<EntityFactory>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  NameDefT name_def;

  Blueprint blueprint1;
  name_def.name = "hello";
  blueprint1.Write(&name_def);
  const Entity entity1 = entity_factory->Create(&blueprint1);

  Blueprint blueprint2;
  name_def.name = "world";
  blueprint2.Write(&name_def);
  const Entity entity2 = entity_factory->Create(&blueprint2);

  EXPECT_THAT(system->GetName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->Destroy(kNullEntity);
  EXPECT_THAT(system->GetName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->Destroy(entity1);
  EXPECT_THAT(system->GetName(entity1), Eq(""));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->Destroy(entity2);
  EXPECT_THAT(system->GetName(entity1), Eq(""));
  EXPECT_THAT(system->GetName(entity2), Eq(""));
}

TEST_F(EntityFactoryTest, QueuedDestroy) {
  auto entity_factory = registry_.Get<EntityFactory>();
  auto system = entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();

  NameDefT name_def;

  Blueprint blueprint1;
  name_def.name = "hello";
  blueprint1.Write(&name_def);
  const Entity entity1 = entity_factory->Create(&blueprint1);

  Blueprint blueprint2;
  name_def.name = "world";
  blueprint2.Write(&name_def);
  const Entity entity2 = entity_factory->Create(&blueprint2);

  EXPECT_THAT(system->GetName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->QueueForDestruction(kNullEntity);
  EXPECT_THAT(system->GetName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->DestroyQueuedEntities();
  EXPECT_THAT(system->GetName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->QueueForDestruction(entity1);
  EXPECT_THAT(system->GetName(entity1), Eq("hello"));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->DestroyQueuedEntities();
  EXPECT_THAT(system->GetName(entity1), Eq(""));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->QueueForDestruction(entity2);
  EXPECT_THAT(system->GetName(entity1), Eq(""));
  EXPECT_THAT(system->GetName(entity2), Eq("world"));

  entity_factory->DestroyQueuedEntities();
  EXPECT_THAT(system->GetName(entity1), Eq(""));
  EXPECT_THAT(system->GetName(entity2), Eq(""));
}

TEST_F(EntityFactoryTest, GetEntityToBlueprintMap) {
  auto entity_factory = registry_.Get<EntityFactory>();
  entity_factory->CreateSystem<NameSystem>();
  InitializeEntityFactory();
  auto& map = entity_factory->GetEntityToBlueprintMap();

  // Create a flatbuffer EntityDef with a NameDef component.  Save the
  // flatbuffer to disk and attempt to create an Entity from that saved
  // blueprint.
  flatbuffers::FlatBufferBuilder fbb1;
  std::vector<flatbuffers::Offset<ComponentDef>> components1;
  auto name_def1 = CreateNameDefDirect(fbb1, "hello");
  components1.push_back(
      CreateComponentDef(fbb1, ComponentDefType_NameDef, name_def1.Union()));
  FinishEntityDefBuffer(fbb1, CreateEntityDefDirect(fbb1, &components1));
  const void* data1 = fbb1.GetBufferPointer();
  const Entity entity1 = entity_factory->CreateFromBlueprint(data1, "one");

  flatbuffers::FlatBufferBuilder fbb2;
  std::vector<flatbuffers::Offset<ComponentDef>> components2;
  auto name_def2 = CreateNameDefDirect(fbb2, "world");
  components2.push_back(
      CreateComponentDef(fbb2, ComponentDefType_NameDef, name_def2.Union()));
  FinishEntityDefBuffer(fbb2, CreateEntityDefDirect(fbb2, &components2));
  const void* data2 = fbb2.GetBufferPointer();
  const Entity entity2 = entity_factory->CreateFromBlueprint(data2, "two");

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

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MissingDependencySystem);
