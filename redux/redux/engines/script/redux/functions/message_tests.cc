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
#include "redux/modules/dispatcher/message.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Ne;

TEST(ScriptMessageTest, MessageType) {
  ScriptEnv env;
  env.Exec("(= foo (make-msg :foo))");
  env.Exec("(= bar (make-msg :bar {(:a 'a')}))");
  env.Exec("(= baz (make-msg :baz {(:a 'a') (:b 123) (:d 'd')}))");

  REDUX_CHECK_SCRIPT_RESULT(env, "(msg-type foo)", ConstHash("foo").get());
  REDUX_CHECK_SCRIPT_RESULT(env, "(msg-type bar)", ConstHash("bar").get());
  REDUX_CHECK_SCRIPT_RESULT(env, "(msg-type baz)", ConstHash("baz").get());
}

TEST(ScriptMessageTest, MessageGet) {
  ScriptEnv env;
  env.Exec("(= foo (make-msg :foo))");
  env.Exec("(= bar (make-msg :bar {(:a 'a')}))");
  env.Exec("(= baz (make-msg :baz {(:a 'a') (:b 123) (:d 'd')}))");

  REDUX_CHECK_SCRIPT_RESULT(env, "(msg-get foo :b)", nullptr);
  REDUX_CHECK_SCRIPT_RESULT(env, "(msg-get baz :b)", 123);
  REDUX_CHECK_SCRIPT_RESULT(env, "(msg-get-or bar :b 345)", 345);
}
}  // namespace
}  // namespace redux
