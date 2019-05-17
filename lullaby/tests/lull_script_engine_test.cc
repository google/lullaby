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

#include "lullaby/modules/lullscript/lull_script_engine.h"

#include "gtest/gtest.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/time.h"

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
  ScriptEngine* engine_;

  void SetUp() override {
    engine_ = registry_.Create<ScriptEngine>(&registry_);
    engine_->CreateEngine<LullScriptEngine>();
    registry_.Create<FunctionBinder>(&registry_);
  }
};

TEST_F(LullScriptEngineTest, SimpleScript) {
  const char* src = "(= y (+ (* (+ x 3) 2) 1))";

  const ScriptId id =
      engine_->LoadInlineScript(src, "script", Language_LullScript);
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
  const ScriptId id =
      engine_->LoadInlineScript(src, "script", Language_LullScript);
  engine_->RunScript(id);
  int value = 0;

  EXPECT_TRUE(engine_->GetValue(id, "a", &value));
  EXPECT_EQ(12, value);
  EXPECT_TRUE(engine_->GetValue(id, "b", &value));
  EXPECT_EQ(6, value);
}

TEST_F(LullScriptEngineTest, MultipleScripts) {
  const ScriptId id1 = engine_->LoadInlineScript("(= x (+ x 1))", "script1",
                                                 Language_LullScript);
  const ScriptId id2 = engine_->LoadInlineScript("(= x (+ x 1))", "script2",
                                                 Language_LullScript);
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
  const ScriptId id =
      engine_->LoadInlineScript("(= x 5)", "script", Language_LullScript);
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

  const ScriptId id =
      engine_->LoadInlineScript(src, "script", Language_LullScript);
  engine_->SetValue(id, "obj", Serializable("baz", 123));
  engine_->RunScript(id);

  int value = 0;
  EXPECT_TRUE(engine_->GetValue(id, "y", &value));
  EXPECT_EQ(2, value);
}

TEST_F(LullScriptEngineTest, UnloadScript) {
  const ScriptId id =
      engine_->LoadInlineScript("(= x 5)", "script", Language_LullScript);
  engine_->RunScript(id);

  int value = 0;
  EXPECT_TRUE(engine_->GetValue(id, "x", &value));
  EXPECT_EQ(5, value);

  size_t total_scripts = engine_->GetTotalScripts();
  engine_->UnloadScript(id);
  engine_->RunScript(id);

  EXPECT_FALSE(engine_->GetValue(id, "x", &value));
  EXPECT_EQ(total_scripts - 1, engine_->GetTotalScripts());
}

TEST_F(LullScriptEngineTest, Duration) {
  FunctionBinder* binder = registry_.Get<FunctionBinder>();
  binder->RegisterFunction("AddDuration", [](Clock::duration d) {
    return d + DurationFromSeconds(1);
  });
  const char* src =
      "(do "
      "  (= sec1234 (AddDuration (durationFromMilliseconds 234)))"
      "  (= eq0 (== ms05 sec1))"
      "  (= eq1 (== ms1 sec1))"
      "  (= eq2 (== ms1 (+ ms05 sec05)))"
      "  (= eq3 (== sec1 (durationFromSeconds 1)))"
      "  (= eq4 (== ms05 (durationFromMilliseconds 500)))"
      "  (= eq5 (== 0.5 (secondsFromDuration sec05)))"
      "  (= eq6 (== 1000 (millisecondsFromDuration ms1)))"
      ")";

  const ScriptId id =
      engine_->LoadInlineScript(src, "script", Language_LullScript);
  engine_->SetValue(id, "sec1", DurationFromSeconds(1));
  engine_->SetValue(id, "sec05", DurationFromSeconds(0.5));
  engine_->SetValue(id, "ms1", DurationFromMilliseconds(1000));
  engine_->SetValue(id, "ms05", DurationFromMilliseconds(500));
  engine_->RunScript(id);

  Clock::duration duration_value;
  bool eq0, eq1, eq2, eq3, eq4, eq5, eq6;
  EXPECT_TRUE(engine_->GetValue(id, "sec1234", &duration_value));
  EXPECT_TRUE(engine_->GetValue(id, "eq0", &eq0));
  EXPECT_TRUE(engine_->GetValue(id, "eq1", &eq1));
  EXPECT_TRUE(engine_->GetValue(id, "eq2", &eq2));
  EXPECT_TRUE(engine_->GetValue(id, "eq3", &eq3));
  EXPECT_TRUE(engine_->GetValue(id, "eq4", &eq4));
  EXPECT_TRUE(engine_->GetValue(id, "eq5", &eq5));
  EXPECT_TRUE(engine_->GetValue(id, "eq6", &eq6));
  EXPECT_EQ(duration_value, DurationFromMilliseconds(1234));
  EXPECT_FALSE(eq0);
  EXPECT_TRUE(eq1);
  EXPECT_TRUE(eq2);
  EXPECT_TRUE(eq3);
  EXPECT_TRUE(eq4);
  EXPECT_TRUE(eq5);
  EXPECT_TRUE(eq6);
}

TEST_F(LullScriptEngineTest, Optional) {
  Optional<int> p, q;
  FunctionBinder* binder = registry_.Get<FunctionBinder>();
  binder->RegisterFunction("SavePQ",
                           [&p, &q](Optional<int> opt1, Optional<int> opt2) {
                             p = opt1;
                             q = opt2;
                           });
  const char* src = R"(
      (do
        (= a 1)
        (= b null)
        (= c x)
        (= d y)
        (SavePQ null 3)
      ))";

  const ScriptId id =
      engine_->LoadInlineScript(src, "script", Language_LullScript);
  const Optional<int> x = 2;
  const Optional<int> y;
  engine_->SetValue(id, "x", x);
  engine_->SetValue(id, "y", y);
  engine_->RunScript(id);

  Optional<int> a, b, c, d;
  EXPECT_TRUE(engine_->GetValue(id, "a", &a));
  EXPECT_TRUE(engine_->GetValue(id, "b", &b));
  EXPECT_TRUE(engine_->GetValue(id, "c", &c));
  EXPECT_TRUE(engine_->GetValue(id, "d", &d));
  EXPECT_EQ(a, Optional<int>(1));
  EXPECT_EQ(b, NullOpt);
  EXPECT_EQ(c, Optional<int>(2));
  EXPECT_EQ(d, NullOpt);
  EXPECT_EQ(p, NullOpt);
  EXPECT_EQ(q, Optional<int>(3));
}

}  // namespace
}  // namespace lull
