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
#include "redux/engines/script/script_engine.h"
#include "redux/systems/name/name_system.h"

namespace redux {
namespace {

using ::testing::Eq;

class NameSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    ScriptEngine::Create(&registry_);
    name_system_ = registry_.Create<NameSystem>(&registry_);
    registry_.Initialize();
    script_engine_ = registry_.Get<ScriptEngine>();
  }

  Registry registry_;
  NameSystem* name_system_;
  ScriptEngine* script_engine_;
};

TEST_F(NameSystemTest, SetGetFind) {
  const Entity entity(123);
  name_system_->SetName(entity, "hello");
  EXPECT_THAT(name_system_->GetName(entity), Eq("hello"));
  EXPECT_THAT(name_system_->FindEntity("hello").value(), Eq(entity));
}

TEST_F(NameSystemTest, Remove) {
  const Entity entity(123);
  name_system_->SetName(entity, "hello");
  EXPECT_THAT(name_system_->GetName(entity), Eq("hello"));
  EXPECT_THAT(name_system_->FindEntity("hello").value(), Eq(entity));

  name_system_->SetName(entity, "");
  EXPECT_THAT(name_system_->GetName(entity), Eq(""));
  EXPECT_FALSE(name_system_->FindEntity("hello").ok());
}

TEST_F(NameSystemTest, FindEntities) {
  const Entity entity1(123);
  name_system_->SetName(entity1, "hello");

  const Entity entity2(456);
  name_system_->SetName(entity2, "hello");

  const Entity entity3(789);
  name_system_->SetName(entity3, "goodbye");

  const auto set1 = name_system_->FindEntities("hello");
  const auto set2 = name_system_->FindEntities("goodbye");
  const auto set3 = name_system_->FindEntities("aloha");
  EXPECT_THAT(set1.size(), Eq(2));
  EXPECT_THAT(set2.size(), Eq(1));
  EXPECT_THAT(set3.size(), Eq(0));
}

TEST_F(NameSystemTest, ForEachEntity) {
  const Entity entity1(123);
  name_system_->SetName(entity1, "hello");

  const Entity entity2(456);
  name_system_->SetName(entity2, "hello");

  const Entity entity3(789);
  name_system_->SetName(entity3, "hello");

  name_system_->ForEachEntity("hello", [&](Entity entity) {
    EXPECT_TRUE(entity == entity1 || entity == entity2 || entity == entity3);
  });
}

TEST_F(NameSystemTest, FindFailsIfMultipleEntitesWithName) {
  const Entity entity1(123);
  name_system_->SetName(entity1, "hello");

  const Entity entity2(456);
  name_system_->SetName(entity2, "hello");

  EXPECT_FALSE(name_system_->FindEntity("hello").ok());
}

TEST_F(NameSystemTest, SetFromNameDef) {
  NameDef def;
  def.name = "hello";

  const Entity entity(123);
  name_system_->SetFromNameDef(entity, def);

  EXPECT_THAT(name_system_->GetName(entity), Eq("hello"));
  EXPECT_THAT(name_system_->FindEntity("hello").value(), Eq(entity));
}

TEST_F(NameSystemTest, AddFromScript) {
  Var result;
  result = script_engine_->RunNow("(rx.Name.SetName (entity 123) 'hello')");

  const Entity entity(123);
  EXPECT_THAT(name_system_->GetName(entity), Eq("hello"));
  EXPECT_THAT(name_system_->FindEntity("hello").value(), Eq(entity));

  result = script_engine_->RunNow("(rx.Name.GetName (entity 123))");
  EXPECT_TRUE(result.Is<std::string_view>());
  EXPECT_THAT(result.ValueOr<std::string_view>(""), Eq("hello"));

  result = script_engine_->RunNow("(rx.Name.FindEntities 'hello')");
  EXPECT_TRUE(result.Is<VarArray>());
  EXPECT_TRUE(result[0].Is<Entity>());
  EXPECT_THAT(result[0].ValueOr(kNullEntity), Eq(entity));
}

}  // namespace
}  // namespace redux
