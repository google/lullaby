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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_TESTING_H_
#define REDUX_ENGINES_SCRIPT_REDUX_TESTING_H_

#define REDUX_CHECK_SCRIPT_VALUE(Var_, Value_)         \
  {                                                    \
    using T = std::decay<decltype(Value_)>::type;      \
    if constexpr (std::is_same_v<T, std::nullptr_t>) { \
      EXPECT_TRUE(Var_.IsNil());                       \
    } else {                                           \
      const auto* ptr = (Var_).Get<T>();               \
      EXPECT_THAT(ptr, ::testing::NotNull());          \
      if (ptr) {                                       \
        EXPECT_THAT(*ptr, ::testing::Eq(Value_));      \
      }                                                \
    }                                                  \
  }

#define REDUX_CHECK_SCRIPT_RESULT(Env_, Cmd_, Value_) \
  {                                                   \
    const ScriptValue res = Env_.Exec(Cmd_);          \
    REDUX_CHECK_SCRIPT_VALUE(res, Value_);            \
  }

#define REDUX_CHECK_SCRIPT_RESULT_NIL(Env_, Cmd_) \
  {                                               \
    const ScriptValue res = Env_.Exec(Cmd_);      \
    EXPECT_TRUE(res.IsNil());                     \
  }

#endif  // REDUX_ENGINES_SCRIPT_REDUX_TESTING_H_
