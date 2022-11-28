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

namespace redux {
namespace {

using ::testing::Eq;

class ScriptTests : public ::testing::Test {
 protected:
  void SetUp() override {
    // We assume the redux script engine for our tests.
    ScriptEngine::Create(&registry_);
    engine_ = registry_.Get<ScriptEngine>();
  }

  Registry registry_;
  ScriptEngine* engine_ = nullptr;
};

TEST_F(ScriptTests, Run) {
  auto script = engine_->ReadScript("(do 123)");
  auto res = script->Run();
  EXPECT_TRUE(res.Is<int>());
  EXPECT_THAT(res.ValueOr(0), Eq(123));
}

TEST_F(ScriptTests, SetValue) {
  auto script = engine_->ReadScript("(+ foo 12)");
  script->SetValue("foo", 34);

  auto res = script->Run();
  EXPECT_TRUE(res.Is<int>());
  EXPECT_THAT(res.ValueOr(0), Eq(46));
}

TEST_F(ScriptTests, GetValue) {
  auto script = engine_->ReadScript("(= foo 456)");
  script->Run();
  auto res = script->GetValue<int>("foo");
  EXPECT_TRUE(res.has_value());
  EXPECT_THAT(res.value(), Eq(456));
}

TEST_F(ScriptTests, GetValueConvert) {
  auto script = engine_->ReadScript("(= foo 456)");
  script->Run();
  auto res = script->GetValue<float>("foo");
  EXPECT_TRUE(res.has_value());
  EXPECT_THAT(res.value(), Eq(456.0f));
}

}  // namespace
}  // namespace redux
