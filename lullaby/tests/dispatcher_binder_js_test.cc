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
#include "lullaby/modules/dispatcher/dispatcher_binder.h"
#include "lullaby/modules/javascript/engine.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/script/script_engine.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/script_def_generated.h"

namespace lull {
namespace {

using ::testing::Eq;

class DispatcherBinderJsTest : public testing::Test {
 public:
  void SetUp() override {
    script_engine_ = registry_.Create<ScriptEngine>(&registry_);
    script_engine_->CreateEngine<script::javascript::Engine>();
    registry_.Create<FunctionBinder>(&registry_);
    dispatcher_ = registry_.Create<Dispatcher>();
    registry_.Create<DispatcherBinder>(&registry_);
  }

  Registry registry_;
  ScriptEngine* script_engine_;
  Dispatcher* dispatcher_;
};

TEST_F(DispatcherBinderJsTest, ConnectGlobalSendInScript) {
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var myEvent = null;
      lull.Dispatcher.ConnectGlobal(hash("myEvent"),
                                    (event) => { myEvent = event; });
      var toSend = {
        type: hash("myEvent"),
        data: {
          myInt: {type: hash("int32_t"), data: 123},
        }
      };
      lull.Dispatcher.SendGlobal(toSend);
      )",
      "ConnectGlobal", Language_JavaScript);
  script_engine_->RunScript(id);

  EventWrapper result;
  EXPECT_TRUE(script_engine_->GetValue(id, "myEvent", &result));
  EXPECT_EQ(result.GetTypeId(), Hash("myEvent"));
  auto* myInt = result.GetValue<int>(Hash("myInt"));
  ASSERT_NE(myInt, nullptr);
  EXPECT_EQ(*myInt, 123);
}

TEST_F(DispatcherBinderJsTest, ConnectGlobalSendExternally) {
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var myEvent = null;
      lull.Dispatcher.ConnectGlobal(hash("myEvent"),
                                    (event) => { myEvent = event; });
      )",
      "ConnectGlobal", Language_JavaScript);
  script_engine_->RunScript(id);

  EventWrapper event(Hash("myEvent"));
  event.SetValue(Hash("myInt"), 123);
  dispatcher_->Send(event);

  EventWrapper result;
  EXPECT_TRUE(script_engine_->GetValue(id, "myEvent", &result));
  EXPECT_EQ(result.GetTypeId(), Hash("myEvent"));
  auto* myInt = result.GetValue<int>(Hash("myInt"));
  ASSERT_NE(myInt, nullptr);
  EXPECT_EQ(*myInt, 123);
}

TEST_F(DispatcherBinderJsTest, DisconnectGlobalInScript) {
  std::vector<int> observed;
  script_engine_->RegisterFunction(
      "observe", [&observed](int input) { observed.push_back(input); });
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var count = 0;
      var incrementId = lull.Dispatcher.ConnectGlobal(
          hash("increment"), (event) => { count++; });

      var toSend = {
        type: hash("increment"),
        data: {}
      };
      observe(count);
      lull.Dispatcher.SendGlobal(toSend);
      observe(count);
      lull.Dispatcher.DisconnectGlobal(hash("increment"), incrementId);
      lull.Dispatcher.SendGlobal(toSend);
      observe(count);
      )",
      "DisconnectGlobal", Language_JavaScript);
  script_engine_->RunScript(id);
  int count = -1;
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 1);

  EXPECT_THAT(observed, Eq(std::vector<int>{0, 1, 1}));
}

TEST_F(DispatcherBinderJsTest, DisconnectGlobalExternally) {
  ScriptId id = script_engine_->LoadInlineScript(
      R"(
      var count = 0;
      var incrementId = lull.Dispatcher.ConnectGlobal(
          hash("increment"), (event) => { count++; });
      lull.Dispatcher.ConnectGlobal(hash("disconnect"), (event) => {
          lull.Dispatcher.DisconnectGlobal(hash("increment"), incrementId);
      });
      )",
      "DisconnectGlobal", Language_JavaScript);
  script_engine_->RunScript(id);
  int count = -1;
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 0);

  EventWrapper increment(Hash("increment"));
  dispatcher_->Send(increment);
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 1);

  EventWrapper disconnect(Hash("disconnect"));
  dispatcher_->Send(disconnect);
  dispatcher_->Send(increment);
  count = -1;
  EXPECT_TRUE(script_engine_->GetValue(id, "count", &count));
  EXPECT_EQ(count, 1);
}

}  // namespace
}  // namespace lull
