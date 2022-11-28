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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_FRAME_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_FRAME_H_

#include "redux/engines/script/redux/script_types.h"

namespace redux {

class ScriptEnv;

// Represents a single function call frame for a script.
//
// This class captures the necessary parts of the AST needed to call a block of
// code, whether it's a script function/macro or a native callback. It also
// provides storage for the return value of the executed code block.
//
// Individual arguments can be "popped" off the list by calling Next() or
// EvalNext(). Next() returns the next value in the arglist, whereas EvalNext()
// returns the evaluated result of the next value.  The difference between using
// Next() and EvalNext() is effectively the difference between a function call
// and a macro call.
class ScriptFrame {
 public:
  // Constructs the ScriptFrame with a given argument list and an optional
  // location in which to store the return value.
  ScriptFrame(ScriptEnv* env, const ScriptValue& args);

  // Returns true if there is another argument in the list.
  bool HasNext() const;

  // Returns the next argument.
  const ScriptValue& Next();

  // Evaluates the next argument and returns its result.
  ScriptValue EvalNext();

  // Returns the ScriptEnv associated with the callframe.
  ScriptEnv* GetEnv() { return env_; }

  // Returns the arguments associated with the callframe.  This will return the
  // "current" argument based on how often Next()/EvalNext() has been called.
  const ScriptValue& GetArgs() const { return *args_; }

  // Sets the return value resulting from the execution of the code associated
  // with the callframe.
  template <typename Value>
  void Return(Value&& value) {
    return_value_ = std::forward<Value>(value);
  }

  ScriptValue ReleaseReturnValue() { return std::move(return_value_); }

  // Indicates that an error was encountered during the processing of the
  // callframe.
  void Error(const char* message);

 private:
  ScriptEnv* env_ = nullptr;
  const ScriptValue* args_ = nullptr;
  ScriptValue return_value_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_FRAME_H_
