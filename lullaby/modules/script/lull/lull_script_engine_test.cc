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

#include "lullaby/modules/script/lull/lull_script_engine.h"

#include "gtest/gtest.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace {

struct Serializable {
  Serializable(std::string name, int value)
      : name(std::move(name)), value(value) {}
  std::string name;
  int value;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&name, ConstHash("name"));
    archive(&value, ConstHash("value"));
  }
};

class LullScriptEngineTest : public testing::Test {
 public:
  Registry registry_;
  LullScriptEngine* engine_;

  void SetUp() override {
    auto* binder = registry_.Create<FunctionBinder>(&registry_);
    engine_ = registry_.Create<LullScriptEngine>();
    engine_->SetFunctionCallHandler(
        [binder](FunctionCall* call) { binder->Call(call); });
  }
};

TEST_F(LullScriptEngineTest, SimpleScript) {
  const char* src = "(= y (+ (* (+ x 3) 2) 1))";

  const uint64_t id = engine_->LoadScript(src, "script");
  engine_->SetValue(id, "x", 10);
  engine_->RunScript(id);

  int value = 0;
  EXPECT_TRUE(engine_->GetValue(id, "y", &value));
  EXPECT_EQ(27, value);
}

TEST_F(LullScriptEngineTest, RegisterFunction) {
  int x = 10;
  FunctionBinder* binder = registry_.Get<FunctionBinder>();
  binder->RegisterFunction("Foo", [x](int y) { return x + y; });
  binder->RegisterFunction("lull.Foo", [x](int y) { return x - y; });

  const char* src =
      "(do"
      "  (= a (Foo 2))"
      "  (= b (lull.Foo 4))"
      ")";
  const uint64_t id = engine_->LoadScript(src, "script");
  engine_->RunScript(id);
  int value = 0;

  EXPECT_TRUE(engine_->GetValue(id, "a", &value));
  EXPECT_EQ(12, value);
  EXPECT_TRUE(engine_->GetValue(id, "b", &value));
  EXPECT_EQ(6, value);
}

TEST_F(LullScriptEngineTest, MultipleScripts) {
  const uint64_t id1 = engine_->LoadScript("(= x (+ x 1))", "script1");
  const uint64_t id2 = engine_->LoadScript("(= x (+ x 1))", "script2");
  int value1 = 10;
  engine_->SetValue(id1, "x", value1);

  int value2 = 20;
  engine_->SetValue(id2, "x", value2);

  engine_->RunScript(id1);
  EXPECT_TRUE(engine_->GetValue(id1, "x", &value1));
  EXPECT_EQ(11, value1);

  engine_->RunScript(id1);
  EXPECT_TRUE(engine_->GetValue(id1, "x", &value1));
  EXPECT_EQ(12, value1);

  engine_->RunScript(id2);
  EXPECT_TRUE(engine_->GetValue(id2, "x", &value2));
  EXPECT_EQ(21, value2);

  engine_->RunScript(id1);
  EXPECT_TRUE(engine_->GetValue(id1, "x", &value1));
  EXPECT_EQ(13, value1);

  engine_->RunScript(id2);
  EXPECT_TRUE(engine_->GetValue(id2, "x", &value2));
  EXPECT_EQ(22, value2);

  engine_->RunScript(id1);
  EXPECT_TRUE(engine_->GetValue(id1, "x", &value1));
  EXPECT_EQ(14, value1);
}

TEST_F(LullScriptEngineTest, ReloadScript) {
  const uint64_t id = engine_->LoadScript("(= x 5)", "script");
  engine_->RunScript(id);

  int value = 0;
  EXPECT_TRUE(engine_->GetValue(id, "x", &value));
  EXPECT_EQ(5, value);

  engine_->ReloadScript(id, "(= y (* x 2))");
  engine_->RunScript(id);

  EXPECT_TRUE(engine_->GetValue(id, "y", &value));
  EXPECT_EQ(10, value);
}

TEST_F(LullScriptEngineTest, SerializableObjects) {
  const char* src = "(= y (map-size obj))";

  const uint64_t id = engine_->LoadScript(src, "script");
  engine_->SetValue(id, "obj", Serializable("baz", 123));
  engine_->RunScript(id);

  int value = 0;
  EXPECT_TRUE(engine_->GetValue(id, "y", &value));
  EXPECT_EQ(2, value);
}

TEST_F(LullScriptEngineTest, UnloadScript) {
  const uint64_t id = engine_->LoadScript("(= x 5)", "script");
  engine_->RunScript(id);

  int value = 0;
  EXPECT_TRUE(engine_->GetValue(id, "x", &value));
  EXPECT_EQ(5, value);

  engine_->UnloadScript(id);
  engine_->RunScript(id);

  EXPECT_FALSE(engine_->GetValue(id, "x", &value));
}

}  // namespace
}  // namespace lull
