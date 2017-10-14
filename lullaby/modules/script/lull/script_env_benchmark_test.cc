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

#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace {

using ::testing::Eq;

static void RegisterTestFunctions(FunctionBinder* binder) {
  binder->RegisterFunction("FlipBool", [](bool x) { return !x; });
  binder->RegisterFunction(
      "AddInt16", [](int16_t x) { return static_cast<int16_t>(x + 1); });
  binder->RegisterFunction(
      "AddInt32", [](int32_t x) { return static_cast<int32_t>(x + 1); });
  binder->RegisterFunction(
      "AddInt64", [](int64_t x) { return static_cast<int64_t>(x + 1); });
  binder->RegisterFunction(
      "AddUInt16", [](uint16_t x) { return static_cast<uint16_t>(x + 1); });
  binder->RegisterFunction(
      "AddUInt32", [](uint32_t x) { return static_cast<uint32_t>(x + 1); });
  binder->RegisterFunction(
      "AddUInt64", [](uint64_t x) { return static_cast<uint64_t>(x + 1); });
  binder->RegisterFunction("RepeatString",
                           [](const std::string& s) { return s + s; });
  binder->RegisterFunction("RotateVec2", [](const mathfu::vec2& v) {
    return mathfu::vec2(v.y, v.x);
  });
  binder->RegisterFunction("RotateVec3", [](const mathfu::vec3& v) {
    return mathfu::vec3(v.y, v.z, v.x);
  });
  binder->RegisterFunction("RotateVec4", [](const mathfu::vec4& v) {
    return mathfu::vec4(v.y, v.z, v.w, v.x);
  });
  binder->RegisterFunction("RotateQuat", [](const mathfu::quat& v) {
    return mathfu::quat(v.vector().x, v.vector().y, v.vector().z, v.scalar());
  });
}

static const char* kBenchmarkTestSrc =
    "(do "
    "(= bool (FlipBool true)) "
    "(= int16 (AddInt32 123)) "
    "(= int32 (AddInt32 123)) "
    "(= int64 (AddInt32 123)) "
    "(= uint16 (AddInt32 123)) "
    "(= uint32 (AddInt32 123)) "
    "(= uint64 (AddInt32 123)) "
    "(= entity (AddInt32 123)) "
    "(= entity (AddInt32 123)) "
    "(= str (RepeatString 'abc')) "
    "(= v2 (RotateVec2 (vec2 1.0f 2.0f))) "
    "(= v3 (RotateVec3 (vec3 1.0f 2.0f 3.0f))) "
    "(= v4 (RotateVec4 (vec4 1.0f 2.0f 3.0f 4.0f))) "
    "(= qt (RotateQuat (quat 1.0f 2.0f 3.0f 4.0f))) "
    ")";

static void Benchmark(benchmark::State& state) {
  Registry registry;
  FunctionBinder binder(&registry);
  RegisterTestFunctions(&binder);

  ScriptEnv env;
  env.SetFunctionCallHandler(
      [&binder](FunctionCall* call) { binder.Call(call); });

  auto script = env.Read(kBenchmarkTestSrc);
  while (state.KeepRunning()) {
    env.Eval(script);
  }
}
BENCHMARK(Benchmark);

// This test verifies that the benchmark code actually behaves correctly.
TEST(ScriptEnvBenchmarkTest, BenchmarkTestVerification) {
  Registry registry;
  FunctionBinder binder(&registry);
  RegisterTestFunctions(&binder);

  ScriptEnv env;
  env.SetFunctionCallHandler(
      [&binder](FunctionCall* call) { binder.Call(call); });

  auto script = env.Read(kBenchmarkTestSrc);
  env.Eval(script);

  EXPECT_THAT(*env.GetValue(Symbol("bool")).Get<bool>(), Eq(false));
  EXPECT_THAT(*env.GetValue(Symbol("int16")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("int32")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("int64")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("uint16")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("uint32")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("uint64")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("entity")).Get<int32_t>(), Eq(124));
  EXPECT_THAT(*env.GetValue(Symbol("str")).Get<std::string>(),
              Eq(std::string("abcabc")));

  auto v2 = *env.GetValue(Symbol("v2")).Get<mathfu::vec2>();
  auto v3 = *env.GetValue(Symbol("v3")).Get<mathfu::vec3>();
  auto v4 = *env.GetValue(Symbol("v4")).Get<mathfu::vec4>();
  auto qt = *env.GetValue(Symbol("qt")).Get<mathfu::quat>();
  EXPECT_THAT(v2, Eq(mathfu::vec2(2, 1)));
  EXPECT_THAT(v3, Eq(mathfu::vec3(2, 3, 1)));
  EXPECT_THAT(v4, Eq(mathfu::vec4(2, 3, 4, 1)));
  EXPECT_THAT(qt.vector(), Eq(mathfu::quat(2, 3, 4, 1).vector()));
  EXPECT_THAT(qt.scalar(), Eq(mathfu::quat(2, 3, 4, 1).scalar()));
}

}  // namespace
}  // namespace lull
