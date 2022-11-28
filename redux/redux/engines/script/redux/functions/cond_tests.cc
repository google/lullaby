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

TEST(ScriptCondTest, Cond) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(cond (true 1) (false 2))", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(cond (false 1) (true 2))", 2);
}

TEST(ScriptCondTest, If) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(if true 1 2)", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(if false 1 2)", 2);
}

}  // namespace
}  // namespace redux
