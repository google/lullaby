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

TEST(ScriptTypeOfTest, IsNil) {
  ScriptEnv env;
  env.SetValue(ConstHash("nillie"), Var());

  REDUX_CHECK_SCRIPT_RESULT(env, "(nil? 1)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(nil? 2.0f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(nil? 'hello')", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(nil? nillie)", true);
}

TEST(ScriptTypeOfTest, TypeOf) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(typeof 1)", GetTypeId<int>());
  REDUX_CHECK_SCRIPT_RESULT(env, "(typeof 2.0f)", GetTypeId<float>());
  REDUX_CHECK_SCRIPT_RESULT(env, "(typeof 'hi')", GetTypeId<std::string>());
}

TEST(ScriptTypeOfTest, Is) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(is? 1.0f int)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(is? 1.0f float)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(is? 'hello' std::string)", true);
}

}  // namespace
}  // namespace redux
