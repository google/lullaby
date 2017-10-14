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

#include "lullaby/modules/script/lull/script_env.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(ScriptEnvTest, CompileLoadEval) {
  ScriptEnv env;
  ScriptByteCode code = env.Compile("(+ 1 1)");

  ScriptValue script = env.Load(code);
  ScriptValue res = env.Eval(script);

  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  ScriptByteCode copy = code;
  ScriptValue script2 = env.Load(copy);
  ScriptValue res2 = env.Eval(script2);

  EXPECT_THAT(res2.Is<int>(), Eq(true));
  EXPECT_THAT(*res2.Get<int>(), Eq(2));
}

TEST(ScriptEnvTest, ReadEval) {
  ScriptEnv env;
  ScriptValue script = env.Read("(+ 1 1)");
  ScriptValue res = env.Eval(script);

  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

TEST(ScriptEnvText, Exec) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(+ 1 1)");

  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

TEST(ScriptEnvText, SetGetValue) {
  ScriptEnv env;
  ScriptValue res;

  env.SetValue(Symbol("foo"), env.Create(123));
  res = env.GetValue(Symbol("foo"));
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(123));

  env.Exec("(= foo 456)");
  res = env.GetValue(Symbol("foo"));
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(456));

  env.Exec("(= bar 789)");
  res = env.GetValue(Symbol("bar"));
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(789));

  env.SetValue(Symbol("foo"), env.Create(Hash("bar")));
  res = env.Exec("(do foo)");
  EXPECT_THAT(res.Is<HashValue>(), Eq(true));
  EXPECT_THAT(*res.Get<HashValue>(), Eq(Hash("bar")));
}

TEST(ScriptEnvTest, Def) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(do (def foo (x y) (+ x (+ y y))) (foo 1 2))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, Call) {
  ScriptEnv env;
  env.Exec("(def foo (x y) (+ x (+ y y)))");

  ScriptValue res = env.Call("foo", 1, 2);
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, CallScope) {
  ScriptEnv env;
  env.Exec("(= x 0)");
  env.Exec("(def foo (x y) (+ x y))");
  ScriptValue res = env.Exec("(foo (+ x 1) x)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));
}

TEST(ScriptEnvTest, CallWithArray) {
  ScriptEnv env;
  env.Exec("(def foo (x y) (+ x (+ y y)))");

  ScriptValue args[] = {
      env.Create(1),
      env.Create(2),
  };
  ScriptValue res = env.CallWithArray("foo", {args});
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, CallWithMap) {
  ScriptEnv env;
  env.Exec("(def foo (x y) (+ x (+ y y)))");

  VariantMap args;
  args[Hash("x")] = 1;
  args[Hash("y")] = 2;

  ScriptValue res = env.CallWithMap("foo", args);
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, CallNativeFunction) {
  auto fn = [](ScriptFrame* frame) {
    auto arg1 = frame->EvalNext();
    auto arg2 = frame->EvalNext();
    const int x = *arg1.Get<int>();
    const int y = *arg2.Get<int>();
    frame->Return(x + y + y);
  };

  ScriptEnv env;
  env.Register("foo", NativeFunction{fn});

  ScriptValue res = env.Eval(env.Read("(foo 1 2)"));

  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, CallFunctionBinder) {
  Registry registry;
  FunctionBinder binder(&registry);
  binder.RegisterFunction("foo", [](int x, int y) { return x + y + y; });

  ScriptEnv env;
  env.SetFunctionCallHandler(
      [&binder](FunctionCall* call) { binder.Call(call); });

  ScriptValue res = env.Eval(env.Read("(foo 1 2)"));

  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, Recurse) {
  ScriptEnv env;

  const char* src =
      "(do"
      "  (def fact (n)"
      "    (if (<= n 1)"
      "      1"
      "      (* n (fact (- n 1)))"
      "    )"
      "  )"
      "  (fact 4)"
      ")";
  ScriptValue res = env.Eval(env.Read(src));

  EXPECT_THAT(res.IsNil(), Eq(false));
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(24));
}

TEST(ScriptEnvTest, Macro) {
  ScriptEnv env;

  const char* src =
      "(do"
      "  (= a 1)"
      "  (= b 1)"
      "  (def f (x) (+ x x))"
      "  (macro m (x) (+ x x))"
      ")";
  env.Exec(src);

  ScriptValue res1 = env.Exec("(f (= a (+ a 1)))");
  EXPECT_THAT(res1.Is<int>(), Eq(true));
  EXPECT_THAT(*res1.Get<int>(), Eq(4));

  ScriptValue res2 = env.Exec("(m (= b (+ b 1)))");
  EXPECT_THAT(res2.Is<int>(), Eq(true));
  EXPECT_THAT(*res2.Get<int>(), Eq(5));
}

TEST(ScriptEnvTest, Do) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(do 1 2 3)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptEnvTest, DoReturn) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(do 1 (return 2) 3)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

}  // namespace
}  // namespace lull
