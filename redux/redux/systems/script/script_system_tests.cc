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
#include "absl/functional/bind_front.h"
#include "redux/systems/dispatcher/dispatcher_system.h"
#include "redux/systems/script/script_system.h"

namespace redux {
namespace {

using ::testing::Eq;

class ScriptSystemTest : public testing::Test {
 public:
  ScriptSystemTest() {
    ScriptEngine::Create(&registry_);
    entity_factory_ = registry_.Create<EntityFactory>(&registry_);
    dispatcher_system_ = registry_.Create<DispatcherSystem>(&registry_);
    script_system_ = entity_factory_->CreateSystem<ScriptSystem>();
    script_engine_ = registry_.Get<ScriptEngine>();
    registry_.Initialize();

    auto test_value_fn = [&](Entity entity, int value) {
      test_entity_ = entity;
      test_value_ = value;
    };
    script_engine_->RegisterFunction("TestValue", test_value_fn);

    auto test_time_fn = [&](Entity entity, absl::Duration value) {
      test_entity_ = entity;
      test_duration_ = value;
    };
    script_engine_->RegisterFunction("TestTime", test_time_fn);

    auto test_msg_fn = [&](Entity entity, const Message& value) {
      test_entity_ = entity;
      test_message_ = value;
    };
    script_engine_->RegisterFunction("TestMsg", test_msg_fn);
  }

 protected:
  Registry registry_;
  ScriptSystem* script_system_;
  DispatcherSystem* dispatcher_system_;
  ScriptEngine* script_engine_;
  EntityFactory* entity_factory_;

  int test_value_ = 0;
  Entity test_entity_ = kNullEntity;
  absl::Duration test_duration_ = absl::ZeroDuration();
  Message test_message_;
};

TEST_F(ScriptSystemTest, OnCreate) {
  const Entity entity = entity_factory_->Create();

  ScriptDef def;
  def.code = "(TestValue $entity 456)";
  def.type = ScriptTriggerType::OnCreate;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_value_, Eq(456));
  EXPECT_THAT(test_entity_, Eq(entity));
}

TEST_F(ScriptSystemTest, OnEnable) {
  const Entity entity = entity_factory_->Create();

  ScriptDef def;
  def.code = "(TestValue $entity 456)";
  def.type = ScriptTriggerType::OnEnable;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_value_, Eq(0));
  EXPECT_THAT(test_entity_, Eq(kNullEntity));
  entity_factory_->Disable(entity);
  entity_factory_->Enable(entity);

  EXPECT_THAT(test_value_, Eq(456));
  EXPECT_THAT(test_entity_, Eq(entity));
}

TEST_F(ScriptSystemTest, OnDisable) {
  const Entity entity = entity_factory_->Create();

  ScriptDef def;
  def.code = "(TestValue $entity 456)";
  def.type = ScriptTriggerType::OnDisable;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_value_, Eq(0));
  EXPECT_THAT(test_entity_, Eq(kNullEntity));
  entity_factory_->Disable(entity);
  EXPECT_THAT(test_value_, Eq(456));
  EXPECT_THAT(test_entity_, Eq(entity));
}

TEST_F(ScriptSystemTest, OnDestroy) {
  const Entity entity = entity_factory_->Create();

  ScriptDef def;
  def.code = "(TestValue $entity 456)";
  def.type = ScriptTriggerType::OnDestroy;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_value_, Eq(0));
  EXPECT_THAT(test_entity_, Eq(kNullEntity));
  entity_factory_->DestroyNow(entity);
  EXPECT_THAT(test_value_, Eq(456));
  EXPECT_THAT(test_entity_, Eq(entity));
}

TEST_F(ScriptSystemTest, OnUpdate) {
  const Entity entity = entity_factory_->Create();

  ScriptDef def;
  def.code = "(TestTime $entity $delta_time)";
  def.type = ScriptTriggerType::OnUpdate;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_entity_, Eq(kNullEntity));
  EXPECT_THAT(test_duration_, Eq(absl::ZeroDuration()));
  script_system_->Update(absl::Seconds(2));
  EXPECT_THAT(test_entity_, Eq(entity));
  EXPECT_THAT(test_duration_, Eq(absl::Seconds(2)));
}

TEST_F(ScriptSystemTest, OnLateUpdate) {
  const Entity entity = entity_factory_->Create();

  ScriptDef def;
  def.code = "(TestTime $entity $delta_time)";
  def.type = ScriptTriggerType::OnLateUpdate;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_entity_, Eq(kNullEntity));
  EXPECT_THAT(test_duration_, Eq(absl::ZeroDuration()));
  script_system_->LateUpdate(absl::Seconds(2));
  EXPECT_THAT(test_entity_, Eq(entity));
  EXPECT_THAT(test_duration_, Eq(absl::Seconds(2)));
}

TEST_F(ScriptSystemTest, OnEvent) {
  const Entity entity = entity_factory_->Create();
  const HashValue event_hash = ConstHash("Event");
  const HashValue value_hash = ConstHash("value");
  const TypeId event_type = TypeId(event_hash.get());

  ScriptDef def;
  def.code = "(TestMsg $entity $message)";
  def.type = ScriptTriggerType::OnEvent;
  def.event = event_hash;
  script_system_->AddFromScriptDef(entity, def);

  EXPECT_THAT(test_entity_, Eq(kNullEntity));
  EXPECT_THAT(test_duration_, Eq(absl::ZeroDuration()));
  Message msg(event_type);
  msg.SetValue(value_hash, 456);
  dispatcher_system_->SendToEntityNow(entity, msg);
  EXPECT_THAT(test_entity_, Eq(entity));
  EXPECT_THAT(test_message_.GetTypeId(), Eq(event_type));
  EXPECT_THAT(test_message_.ValueOr(value_hash, 0), Eq(456));
}

}  // namespace
}  // namespace redux
