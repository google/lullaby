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

#ifndef LULLABY_SCRIPT_LULL_SCRIPT_ENV_H_
#define LULLABY_SCRIPT_LULL_SCRIPT_ENV_H_

#include <stdint.h>
#include <cstddef>
#include "lullaby/modules/function/function_call.h"
#include "lullaby/modules/script/lull/script_scoped_symbol_table.h"
#include "lullaby/modules/script/lull/script_types.h"
#include "lullaby/modules/script/lull/script_value.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/variant.h"

namespace lull {

// The environment for execution of a script.
//
// This class acts as the way to interact with LullScript.  Its primary purpose
// is to evaluate the abstract syntax tree (AST) representation of a script into
// a single resulting value.
//
// In addition to evaluating scripts, the ScriptEnv uses a
// ScriptScopedSymbolTable to store ScriptValues.  On construction, the global
// table is setup to store all the "built-in" functions of LullScript.  Addition
// global variables can be set by calling SetValue or Register (for functions).
//
// Finally, it provides useful functions for evaluating source code directly or
// converting source into the AST for evaluation.
class ScriptEnv {
 public:
  using PrintFn = std::function<void(std::string)>;

  ScriptEnv();

  // Sets the handler which allows scripts to call functions registered
  // externally via a FunctionCall object.
  void SetFunctionCallHandler(FunctionCall::Handler handler);

  // Sets the function to use for print commands.
  void SetPrintFunction(PrintFn fn);

  // Converts source code into byte code.
  ScriptByteCode Compile(string_view src);

  // Converts byte code into an AST stored in a ScriptValue.
  ScriptValue Load(const ScriptByteCode& code);

  // Converts source code into an AST stored in a ScriptValue.
  ScriptValue Read(string_view src);

  // Evaluates the AST represented by the ScriptValue.
  ScriptValue LoadOrRead(Span<uint8_t> code);

  // Evaluates the AST represented by the ScriptValue.
  ScriptValue Eval(ScriptValue script);

  // Executes the source code by effectively calling Read then Eval.
  ScriptValue Exec(string_view src);

  // Associates the |value| with |symbol| in the internal
  // ScriptScopedSymbolTable.  When called outside the context of a running
  // script, this effectively creates a global variable in the ScriptEnv.
  void SetValue(const Symbol& symbol, ScriptValue value);

  // Gets the value associated with |symbol| from the internal
  // ScriptScopedSymbolTable.
  ScriptValue GetValue(const Symbol& symbol) const;

  // Registers a NativeFunction.
  void Register(string_view id, NativeFunction fn);

  // Calls a function defined in a script (eg. def or macro) function with the
  // given args.
  template <typename... Args>
  ScriptValue Call(const Symbol& id, Args&&... args);
  template <typename... Args>
  ScriptValue Call(string_view id, Args&&... args);

  // Calls a function defined in a script (eg. def or macro) function with the
  // given args stored as an array.
  ScriptValue CallWithArray(const Symbol& id, Span<ScriptValue> args);
  ScriptValue CallWithArray(string_view id, Span<ScriptValue> args);

  // Calls a function defined in a script (eg. def or macro) function with the
  // given args stored in a VariantMap.
  ScriptValue CallWithMap(const Symbol& id, const VariantMap& kwargs);
  ScriptValue CallWithMap(string_view id, const VariantMap& kwargs);

  // Creates a new ScriptValue.
  template <typename T>
  ScriptValue Create(T&& value);

  // Prints an error associated with the ScriptValue.
  void Error(const char* err, const ScriptValue& context);

  // Starts a new scope.
  void PushScope();

  // Pops the current scope.
  void PopScope();

 private:
  enum ValueType {
    kPrimitive,
    kFunction,
    kMacro,
  };

  ScriptValue DoImpl(const ScriptValue& body);

  ScriptValue SetImpl(const ScriptValue& args, ValueType type);

  ScriptValue CallInternal(ScriptValue fn, const ScriptValue& args);

  ScriptValue InvokeFunctionCall(const Symbol& id, const ScriptValue& args);

  bool AssignArgs(ScriptValue params, ScriptValue args, bool eval);

  ScriptScopedSymbolTable table_;
  PrintFn print_fn_ = nullptr;
  FunctionCall::Handler call_handler_ = nullptr;

  ScriptEnv(const ScriptEnv& rhs);
  ScriptEnv& operator=(const ScriptEnv& rhs);
};

template <typename... Args>
ScriptValue ScriptEnv::Call(string_view id, Args&&... args) {
  return Call(Symbol(id), std::forward<Args>(args)...);
}

template <typename... Args>
ScriptValue ScriptEnv::Call(const Symbol& id, Args&&... args) {
  ScriptValue values[] = {
      Create(std::forward<Args>(args))...,
  };
  return CallWithArray(id, values);
}

template <typename T>
ScriptValue ScriptEnv::Create(T&& value) {
  return ScriptValue::Create(std::forward<T>(value));
}

// Simple type to manange script environment scopes.
class ScriptEnvScope {
 public:
  explicit ScriptEnvScope(ScriptEnv* env) : env_(env) {
    if (env_) {
      env_->PushScope();
    }
  }

  ~ScriptEnvScope() {
    if (env_) {
      env_->PopScope();
    }
  }

 private:
  ScriptEnv* env_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScriptEnv);

#endif  // LULLABY_SCRIPT_LULL_SCRIPT_ENV_H_
