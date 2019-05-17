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

#include "lullaby/systems/script/script_system.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/lullscript/lull_script_engine.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/script/script_engine.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/script_def_generated.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

class ScriptSystemTest : public ::testing::Test {
 protected:
  ScriptSystemTest() {
    script_engine_ = registry_.Create<ScriptEngine>(&registry_);
    script_engine_->CreateEngine<LullScriptEngine>();
    dispatcher_ = registry_.Create<Dispatcher>();
    entity_factory_ = registry_.Create<EntityFactory>(&registry_);
    entity_factory_->CreateSystem<DispatcherSystem>();
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    script_system_ = entity_factory_->CreateSystem<ScriptSystem>();
    binder_ = registry_.Create<FunctionBinder>(&registry_);
  }

  Registry registry_;
  Dispatcher* dispatcher_;
  EntityFactory* entity_factory_;
  TransformSystem* transform_system_;
  ScriptEngine* script_engine_;
  ScriptSystem* script_system_;
  FunctionBinder* binder_;
};

TEST_F(ScriptSystemTest, ScriptOnEventDef) {
  TransformDefT transform;
  ScriptOnEventDefT script_on_event;
  EventDefT event_def;
  event_def.global = true;
  event_def.event = "SomeEvent";
  script_on_event.inputs.push_back(event_def);
  script_on_event.script.code = "(setevent event)";
  script_on_event.script.debug_name = "SomeEventScript";
  script_on_event.script.language = Language_LullScript;
  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&script_on_event);

  EventWrapper event;
  binder_->RegisterFunction("setevent",
                            [&event](const EventWrapper& e) { event = e; });

  Entity entity = entity_factory_->Create(&blueprint);
  EXPECT_EQ(1u, script_engine_->GetTotalScripts());
  EXPECT_EQ(event.GetTypeId(), 0u);

  dispatcher_->Send(EventWrapper(Hash("SomeEvent")));
  EXPECT_EQ(1u, script_engine_->GetTotalScripts());
  EXPECT_EQ(event.GetTypeId(), Hash("SomeEvent"));

  entity_factory_->Destroy(entity);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
}

TEST_F(ScriptSystemTest, ScriptEveryFrameDef) {
  TransformDefT transform;
  ScriptEveryFrameDefT script_every_frame;
  script_every_frame.script.code = "(setdt delta_time)";
  script_every_frame.script.debug_name = "EveryFrameScript";
  script_every_frame.script.language = Language_LullScript;
  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&script_every_frame);

  double dt = -1;
  binder_->RegisterFunction("setdt", [&dt](double newdt) { dt = newdt; });

  Entity entity = entity_factory_->Create(&blueprint);
  EXPECT_EQ(1u, script_engine_->GetTotalScripts());
  EXPECT_EQ(dt, -1);

  script_system_->AdvanceFrame(std::chrono::seconds(123));
  EXPECT_EQ(1u, script_engine_->GetTotalScripts());
  EXPECT_EQ(dt, 123);

  transform_system_->Disable(entity);

  script_system_->AdvanceFrame(std::chrono::seconds(456));
  EXPECT_EQ(1u, script_engine_->GetTotalScripts());
  EXPECT_EQ(dt, 123);

  entity_factory_->Destroy(entity);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
}

TEST_F(ScriptSystemTest, ScriptOnCreateDef) {
  TransformDefT transform;
  ScriptOnCreateDefT script_on_create;
  script_on_create.script.code = "(setx 5)";
  script_on_create.script.debug_name = "OnCreateScript";
  script_on_create.script.language = Language_LullScript;
  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&script_on_create);

  int x = -1;
  binder_->RegisterFunction("setx", [&x](int newx) { x = newx; });

  Entity entity = entity_factory_->Create(&blueprint);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
  EXPECT_EQ(x, 5);

  entity_factory_->Destroy(entity);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
}

TEST_F(ScriptSystemTest, ScriptOnPostCreateInitDef) {
  TransformDefT transform;
  ScriptOnCreateDefT script_on_create;
  script_on_create.script.code = "(setx 5)";
  script_on_create.script.debug_name = "OnCreateScript";
  script_on_create.script.language = Language_LullScript;
  ScriptOnPostCreateInitDefT script_on_post_create_init;
  script_on_post_create_init.script.code = "(setx 7)";
  script_on_post_create_init.script.debug_name = "OnPostCreateInitScript";
  script_on_post_create_init.script.language = Language_LullScript;
  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&script_on_create);
  blueprint.Write(&script_on_post_create_init);

  int x = -1;
  binder_->RegisterFunction("setx", [&x](int newx) { x = newx; });

  Entity entity = entity_factory_->Create(&blueprint);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
  EXPECT_EQ(x, 7);

  entity_factory_->Destroy(entity);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
}

TEST_F(ScriptSystemTest, ScriptOnDestroyDef) {
  TransformDefT transform;
  ScriptOnDestroyDefT script_on_destroy;
  script_on_destroy.script.code = "(setx 5)";
  script_on_destroy.script.debug_name = "OnDestroyScript";
  script_on_destroy.script.language = Language_LullScript;
  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&script_on_destroy);

  int x = -1;
  binder_->RegisterFunction("setx", [&x](int newx) { x = newx; });

  Entity entity = entity_factory_->Create(&blueprint);
  EXPECT_EQ(1u, script_engine_->GetTotalScripts());
  EXPECT_EQ(x, -1);

  entity_factory_->Destroy(entity);
  EXPECT_EQ(0u, script_engine_->GetTotalScripts());
  EXPECT_EQ(x, 5);
}

}  // namespace
}  // namespace lull
