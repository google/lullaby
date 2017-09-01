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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_FRAME_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_FRAME_H_

#include "lullaby/modules/script/lull/script_arg_list.h"
#include "lullaby/modules/script/lull/script_types.h"
#include "lullaby/util/typed_pointer.h"

namespace lull {

class ScriptEnv;

// Represents a single function call-frame for a script.
//
// This class captures the necessary parts of the AST needed to call a block of
// code, whether it's a script function/macro or a native callback.  It also
// provides storage for the return value of the executed code block.
class ScriptFrame : public ScriptArgList {
 public:
  // Type aliases for enabling functions depending on whether T is a pointer.
  template <typename T>
  using EnableIfPointer = std::enable_if<std::is_pointer<T>::value, void>;
  template <typename T>
  using EnableIfNotPointer = std::enable_if<!std::is_pointer<T>::value, void>;

  // Constructs the ScriptFrame with a given argument list.
  ScriptFrame(ScriptEnv* env, ScriptValue args)
      : ScriptArgList(env, std::move(args)) {}

  // Returns the ScriptEnv associated with the callframe.
  ScriptEnv* GetEnv() { return env_; }

  // Returns the arguments associated with the callframe.  This will return the
  // "current" argument based on how often Next()/EvalNext() has been called.
  ScriptValue GetArgs() const { return args_; }

  // Sets the return value resulting from the execution of the code associated
  // with the callframe.
  void Return(ScriptValue value) { return_value_ = std::move(value); }

  template <typename Value>
  typename EnableIfNotPointer<Value>::type Return(const Value& value) {
    Return(ScriptValue::Create(value));  // env_->Create(value);
  }

  template <typename Value>
  typename EnableIfPointer<Value>::type Return(const Value& value) {
    Return(TypedPointer(value));
  }

  // Gets the return value that was set by calling Return().
  ScriptValue GetReturnValue() const { return return_value_; }

  // Indicates that an error was encountered during the processing of the
  // callframe.
  void Error(const char* message);

 private:
  ScriptValue return_value_;
};
}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_FRAME_H_
