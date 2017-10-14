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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/script/lull/functions/testing_macros.h"
#include "lullaby/modules/script/lull/script_env.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(ScriptFunctionsTypesTest, Casts) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(int8 1)", int8_t(1));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint8 2)", uint8_t(2));
  LULLABY_TEST_SCRIPT_VALUE(env, "(int16 3)", int16_t(3));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint16 4)", uint16_t(4));
  LULLABY_TEST_SCRIPT_VALUE(env, "(int32 5)", int32_t(5));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint32 6)", uint32_t(6));
  LULLABY_TEST_SCRIPT_VALUE(env, "(int64 7)", int64_t(7));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint64 8)", uint64_t(8));
  LULLABY_TEST_SCRIPT_VALUE(env, "(float 9)", 9.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(double 10)", 10.0);

  LULLABY_TEST_SCRIPT_VALUE(env, "(int8 1.f)", int8_t(1));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint8 2.f)", uint8_t(2));
  LULLABY_TEST_SCRIPT_VALUE(env, "(int16 3.f)", int16_t(3));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint16 4.f)", uint16_t(4));
  LULLABY_TEST_SCRIPT_VALUE(env, "(int32 5.f)", int32_t(5));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint32 6.f)", uint32_t(6));
  LULLABY_TEST_SCRIPT_VALUE(env, "(int64 7.f)", int64_t(7));
  LULLABY_TEST_SCRIPT_VALUE(env, "(uint64 8.f)", uint64_t(8));
  LULLABY_TEST_SCRIPT_VALUE(env, "(float 9.f)", 9.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(double 10.f)", 10.0);
}


}  // namespace
}  // namespace lull
