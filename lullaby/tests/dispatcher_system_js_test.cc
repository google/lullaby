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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/javascript/engine.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/script/script_engine.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/script_def_generated.h"

namespace lull {
namespace {

using ::testing::Eq;

class DispatcherSystemJsTest : public testing::Test {
 public:
  void SetUp() override {
    script_engine_ = registry_.Create<ScriptEngine>(&registry_);
    script_engine_->CreateEngine<script::javascript::Engine>();
    registry_.Create<FunctionBinder>(&registry_);
    registry_.Create<Dispatcher>();
    dispatcher_system_ = registry_.Create<DispatcherSystem>(&registry_);
    dispatcher_system_->Initialize();
  }

  Registry registry_;
  ScriptEngine* script_engine_;
  DispatcherSystem* dispatcher_system_;
};

TEST_F(DispatcherSystemJsTest, ConnectSendInScript) {
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var myEvent = null;
      var target = 42;
      lull.Dispatcher.Connect(target, hash("myEvent"),
                              (event) => { myEvent = event; });
      var toSend = {
        type: hash("myEvent"),
        data: {
          myInt: {type: hash("int32_t"), data: 123},
        }
      };
      lull.Dispatcher.Send(target, toSend);
      )",
      "Connect", Language_JavaScript);
  script_engine_->RunScript(id);

  EventWrapper result;
  EXPECT_TRUE(script_engine_->GetValue(id, "myEvent", &result));
  EXPECT_EQ(result.GetTypeId(), Hash("myEvent"));
  auto* myInt = result.GetValue<int>(Hash("myInt"));
  ASSERT_NE(myInt, nullptr);
  EXPECT_EQ(*myInt, 123);
}

TEST_F(DispatcherSystemJsTest, ConnectSendExternally) {
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var myEvent = null;
      var target = 42;
      lull.Dispatcher.Connect(target, hash("myEvent"),
                              (event) => { myEvent = event; });
      )",
      "Connect", Language_JavaScript);
  script_engine_->RunScript(id);

  const Entity target = 42;
  EventWrapper event(Hash("myEvent"));
  event.SetValue(Hash("myInt"), 123);
  dispatcher_system_->Send(target, event);

  EventWrapper result;
  EXPECT_TRUE(script_engine_->GetValue(id, "myEvent", &result));
  EXPECT_EQ(result.GetTypeId(), Hash("myEvent"));
  auto* myInt = result.GetValue<int>(Hash("myInt"));
  ASSERT_NE(myInt, nullptr);
  EXPECT_EQ(*myInt, 123);
}

TEST_F(DispatcherSystemJsTest, DisconnectInScript) {
  std::vector<int> observed;
  script_engine_->RegisterFunction(
      "observe", [&observed](int input) { observed.push_back(input); });
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var count = 0;
      var target = 42;
      var incrementId = lull.Dispatcher.Connect(
          target, hash("increment"), (event) => { count++; });

      var toSend = {
        type: hash("increment"),
        data: {}
      };
      observe(count);
      lull.Dispatcher.Send(target, toSend);
      observe(count);
      lull.Dispatcher.Disconnect(target, hash("increment"), incrementId);
      lull.Dispatcher.Send(target, toSend);
      observe(count);
      )",
      "Disconnect", Language_JavaScript);
  script_engine_->RunScript(id);
  int count = -1;
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 1);

  EXPECT_THAT(observed, Eq(std::vector<int>{0, 1, 1}));
}

TEST_F(DispatcherSystemJsTest, DisconnectExternally) {
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var count = 0;
      var target = 42;
      var incrementId = lull.Dispatcher.Connect(
          target, hash("increment"), (event) => { count++; });
      lull.Dispatcher.Connect(target, hash("disconnect"), (event) => {
          lull.Dispatcher.Disconnect(target, hash("increment"), incrementId);
      });
      )",
      "Disconnect", Language_JavaScript);
  script_engine_->RunScript(id);
  int count = -1;
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 0);

  const Entity target = 42;
  EventWrapper increment(Hash("increment"));
  dispatcher_system_->Send(target, increment);
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 1);

  EventWrapper disconnect(Hash("disconnect"));
  dispatcher_system_->Send(target, disconnect);
  dispatcher_system_->Send(target, increment);
  count = -1;
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 1);
}

}  // namespace
}  // namespace lull
