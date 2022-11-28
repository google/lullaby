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
#include "redux/engines/script/redux/script_env.h"
#include "redux/engines/script/redux/testing.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(ScriptEnvTest, ReadEval) {
  ScriptEnv env;
  ScriptValue script = env.Read("(do 123)");
  ScriptValue res = env.Eval(script);
  REDUX_CHECK_SCRIPT_VALUE(res, 123);
}

TEST(ScriptEnvTest, Exec) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(do 123)");
  REDUX_CHECK_SCRIPT_VALUE(res, 123);
}

TEST(ScriptEnvTest, SetGetValue) {
  ScriptEnv env;
  ScriptValue res;

  env.SetValue(ConstHash("foo"), 123);
  res = env.GetValue(ConstHash("foo"));
  REDUX_CHECK_SCRIPT_VALUE(res, 123);

  env.Exec("(= foo 456)");
  res = env.GetValue(ConstHash("foo"));
  REDUX_CHECK_SCRIPT_VALUE(res, 456);

  env.Exec("(= bar 789)");
  res = env.GetValue(ConstHash("bar"));
  REDUX_CHECK_SCRIPT_VALUE(res, 789);
}

TEST(ScriptEnvTest, Eval) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(eval 123)");
  REDUX_CHECK_SCRIPT_VALUE(res, 123);
}

TEST(ScriptEnvTest, Begin) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(begin 1 2 3)");
  REDUX_CHECK_SCRIPT_VALUE(res, 3);
}

TEST(ScriptEnvTest, Do) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(do 1 2 3)");
  REDUX_CHECK_SCRIPT_VALUE(res, 3);
}

TEST(ScriptEnvTest, DoReturn) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(do 1 (return 2) 3)");
  REDUX_CHECK_SCRIPT_VALUE(res, 2);
}

TEST(ScriptEnvTest, DoReturnNil) {
  ScriptEnv env;
  ScriptValue res = env.Exec("(do (return) 2)");
  EXPECT_TRUE(res.IsNil());
}

TEST(ScriptEnvTest, VarAssign) {
  ScriptEnv env;
  ScriptValue res;
  res = env.Exec("(do (= foo 123) foo)");
  REDUX_CHECK_SCRIPT_VALUE(res, 123);

  res = env.Exec("(do (var foo 123) foo)");
  REDUX_CHECK_SCRIPT_VALUE(res, 123);

  res = env.Exec("(do (= foo 123) (begin (= foo 456)) foo)");
  REDUX_CHECK_SCRIPT_VALUE(res, 456);

  res = env.Exec("(do (= foo 123) (begin (var foo 456)) foo)");
  REDUX_CHECK_SCRIPT_VALUE(res, 123);
}

TEST(ScriptEnvTest, Def) {
  ScriptEnv env;
  ScriptValue res = env.Exec(
      "(do"
      "  (def foo (x y)"
      "     (+ x (+ y y))"
      "  )"
      "  (foo 1 2)"
      ")");
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, Macro) {
  ScriptEnv env;
  env.Exec(
      "(do"
      "  (= a 1)"
      "  (= b 1)"
      "  (def f (x) (+ x x))"
      "  (macro m (x) (+ x x))"
      ")");

  ScriptValue res1 = env.Exec("(f (= a (+ a 1)))");
  REDUX_CHECK_SCRIPT_VALUE(res1, 4);
  ScriptValue res2 = env.Exec("(m (= b (+ b 1)))");
  REDUX_CHECK_SCRIPT_VALUE(res2, 5);
}

TEST(ScriptEnvTest, Lambda) {
  ScriptEnv env;
  ScriptValue res = env.Exec(
      "(do "
      "  (= foo (lambda (x y) (+ x (+ y y))))"
      "  (foo 1 2)"
      ")");
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, PassLambda) {
  ScriptEnv env;
  env.Exec(
      "(def foo (fn x y)"
      "  (fn x y)"
      ")");

  ScriptValue res;
  res = env.Exec("(foo (lambda (x y) (+ x y)) 1 2)");
  REDUX_CHECK_SCRIPT_VALUE(res, 3);

  res = env.Exec("(foo (lambda (x y) (* x y)) 1 2)");
  REDUX_CHECK_SCRIPT_VALUE(res, 2);
}

TEST(ScriptEnvTest, Scope) {
  ScriptEnv env;
  env.Exec("(= x 0)");
  env.Exec("(def foo (x y) (+ x y))");
  ScriptValue res = env.Exec("(foo (+ x 1) x)");
  REDUX_CHECK_SCRIPT_VALUE(res, 1);
}

TEST(ScriptEnvTest, Call) {
  ScriptEnv env;
  env.Exec("(def foo (x y) (+ x (+ y y)))");

  ScriptValue res = env.Call(ConstHash("foo"), 1, 2);
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, CallVarSpan) {
  ScriptEnv env;
  env.Exec("(def foo (x y) (+ x (+ y y)))");

  std::vector<Var> args;
  args.emplace_back(1);
  args.emplace_back(2);
  ScriptValue res = env.CallVarSpan(ConstHash("foo"), absl::Span<Var>(args));
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, CallVarTable) {
  ScriptEnv env;
  env.Exec("(def foo (x y) (+ x (+ y y)))");

  VarTable args;
  args[ConstHash("y")] = 2;
  args[ConstHash("x")] = 1;

  ScriptValue res = env.CallVarTable(ConstHash("foo"), args);
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, CallNativeScriptFrameFunction) {
  ScriptEnv env;
  env.RegisterFunction(ConstHash("foo"), [](ScriptFrame* frame) {
    ScriptValue arg1 = frame->EvalNext();
    ScriptValue arg2 = frame->EvalNext();
    const int x = *arg1.Get<int>();
    const int y = *arg2.Get<int>();
    frame->Return(x + y + y);
  });

  ScriptValue res = env.Exec("(foo 1 2)");
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, CallNativeFunction) {
  ScriptEnv env;
  env.RegisterFunction(ConstHash("foo"),
                       [](int x, int y) { return x + y + y; });

  ScriptValue res = env.Exec("(foo 1 2)");
  REDUX_CHECK_SCRIPT_VALUE(res, 5);
}

TEST(ScriptEnvTest, Recurse) {
  ScriptEnv env;
  ScriptValue res = env.Exec(
      "(do"
      "  (def fact (n)"
      "    (if (<= n 1)"
      "      1"
      "      (* n (fact (- n 1)))"
      "    )"
      "  )"
      "  (fact 4)"
      ")");
  REDUX_CHECK_SCRIPT_VALUE(res, 24);
}

TEST(ScriptEnvTest, Print) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(? 'hello world')");
  REDUX_CHECK_SCRIPT_VALUE(res, std::string("hello world"));

  // Multiple things to print should be separated by a space.
  res = env.Exec("(? 'hello' 'world')");
  REDUX_CHECK_SCRIPT_VALUE(res, std::string("hello world"));
}

TEST(ScriptEnvTest, Hash) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(hash 'hello')", Hash("hello"));
}

TEST(ScriptEnvTest, Casts) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(int8 1)", int8_t(1));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint8 2)", uint8_t(2));
  REDUX_CHECK_SCRIPT_RESULT(env, "(int16 3)", int16_t(3));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint16 4)", uint16_t(4));
  REDUX_CHECK_SCRIPT_RESULT(env, "(int32 5)", int32_t(5));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint32 6)", uint32_t(6));
  REDUX_CHECK_SCRIPT_RESULT(env, "(int64 7)", int64_t(7));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint64 8)", uint64_t(8));
  REDUX_CHECK_SCRIPT_RESULT(env, "(float 9)", 9.0f);
  REDUX_CHECK_SCRIPT_RESULT(env, "(double 10)", 10.0);

  REDUX_CHECK_SCRIPT_RESULT(env, "(int8 1.f)", int8_t(1));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint8 2.f)", uint8_t(2));
  REDUX_CHECK_SCRIPT_RESULT(env, "(int16 3.f)", int16_t(3));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint16 4.f)", uint16_t(4));
  REDUX_CHECK_SCRIPT_RESULT(env, "(int32 5.f)", int32_t(5));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint32 6.f)", uint32_t(6));
  REDUX_CHECK_SCRIPT_RESULT(env, "(int64 7.f)", int64_t(7));
  REDUX_CHECK_SCRIPT_RESULT(env, "(uint64 8.f)", uint64_t(8));
  REDUX_CHECK_SCRIPT_RESULT(env, "(float 9.f)", 9.0f);
  REDUX_CHECK_SCRIPT_RESULT(env, "(double 10.f)", 10.0);
}

}  // namespace
}  // namespace redux
