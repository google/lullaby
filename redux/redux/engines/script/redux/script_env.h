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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_ENV_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_ENV_H_

#include <random>

#include "absl/types/span.h"
#include "redux/engines/script/call_native_function.h"
#include "redux/engines/script/redux/script_frame_context.h"
#include "redux/engines/script/redux/script_stack.h"
#include "redux/engines/script/redux/script_types.h"
#include "redux/engines/script/redux/script_value.h"
#include "redux/modules/base/function_traits.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_table.h"

namespace redux {

// The environment for execution of a script.
//
// It evaluates the abstract syntax tree (AST) representation of a script into a
// single resulting value. Internally, it uses a ScriptStack to store data types
// as needed. On construction, the global table is setup to store all the
// "built-in" functions.  Additional global variables can be set by calling
// SetValue or RegisterFunction.
//
// It also provides useful functions for evaluating source code directly or
// converting source into the AST for evaluation.
class ScriptEnv {
 public:
  explicit ScriptEnv(ScriptStack* globals = nullptr);

  // Converts source code into an AST stored in a ScriptValue.
  ScriptValue Read(std::string_view src);

  // Evaluates the AST represented by the ScriptValue.
  ScriptValue Eval(const ScriptValue& script);

  // Executes the source code by effectively calling Read then Eval.
  ScriptValue Exec(std::string_view src);

  // Associates the |value| with |symbol| in the internal
  // ScriptScopedSymbolTable.  When called outside the context of a running
  // script, this effectively creates a global variable in the ScriptEnv.
  void SetValue(HashValue id, ScriptValue value);

  // Similar to SetValue, but where SetValue will update any active binding,
  // LetValue will only update a binding that exists in the current scope, or
  // introduce a new binding in the current scope if necessary.
  void LetValue(HashValue id, ScriptValue value);

  // Gets the value associated with |symbol| from the ScriptStack.
  ScriptValue GetValue(HashValue id);

  // Registers a native function.
  template <typename Fn>
  void RegisterFunction(HashValue id, const Fn& fn);

  // Calls a function defined in a script (eg. def or macro) with the args.
  template <typename... Args>
  ScriptValue Call(HashValue id, Args&&... args);
  ScriptValue CallVarSpan(HashValue id, absl::Span<Var> args);
  ScriptValue CallVarTable(HashValue id, const VarTable& kwargs);

  // Prints an error associated with the Var.
  void Error(const char* msg, const ScriptValue& context);

  // Starts a new scope.
  void PushScope();

  // Pops the current scope.
  void PopScope();

 private:
  enum ValueType {
    kSetPrimitive,
    kLetPrimitive,
    kFunction,
    kMacro,
  };

  ScriptValue DoImpl(const ScriptValue& body);
  ScriptValue SetImpl(const ScriptValue& args, ValueType type);
  ScriptValue CallInternal(const ScriptValue& callable,
                           const ScriptValue& args);
  bool AssignArgs(const ScriptValue* params, const ScriptValue* args,
                  bool eval);

  ScriptStack stack_;
  ScriptStack* globals_ = nullptr;
  std::mt19937 rng_engine_;
};

template <typename Fn>
constexpr bool IsScriptFrameFn() {
  using Traits = FunctionTraits<Fn>;
  if constexpr (!std::is_void_v<typename Traits::ReturnType>) {
    return false;
  } else if constexpr (Traits::kNumArgs != 1) {
    return false;
  } else if constexpr (!std::is_same_v<typename Traits::template Arg<0>::Type,
                                       ScriptFrame*>) {
    return false;
  } else {
    return true;
  }
}

template <typename Fn>
void ScriptEnv::RegisterFunction(HashValue id, const Fn& fn) {
  if constexpr (IsScriptFrameFn<Fn>()) {
    SetValue(id, NativeFunction(fn));
  } else {
    auto wrapped_fn = [&](ScriptFrame* frame) {
      ScriptFrameContext context(frame);
      CallNativeFunction(&context, fn);
    };
    SetValue(id, NativeFunction(wrapped_fn));
  }
}

template <typename... Args>
ScriptValue ScriptEnv::Call(HashValue id, Args&&... args) {
  Var values[] = {
      std::forward<Args>(args)...,
  };
  return CallVarSpan(id, absl::Span<Var>(values));
}
}  // namespace redux

REDUX_SETUP_TYPEID(redux::ScriptEnv);

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_ENV_H_
