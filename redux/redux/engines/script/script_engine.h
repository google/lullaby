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

#ifndef REDUX_ENGINES_SCRIPT_SCRIPT_ENGINE_H_
#define REDUX_ENGINES_SCRIPT_SCRIPT_ENGINE_H_

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/status/status.h"
#include "redux/engines/script/call_native_function.h"
#include "redux/engines/script/script.h"
#include "redux/engines/script/script_call_context.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/var/var.h"
#include "magic_enum.hpp"

namespace redux {

// Manages script assets and executes them using an underlying vm.
class ScriptEngine {
 public:
  static void Create(Registry* registry);

  virtual ~ScriptEngine() = default;

  // Runs a snippet of code immediately.
  Var RunNow(std::string_view code);

  // Loads a script from the given uri. Returns null if unable to load the
  // script.
  std::unique_ptr<Script> LoadScript(std::string_view uri);

  // Loads a script from a string containing inline code/bytecode for the given
  // language. The debug_name is used when reporting error messages.
  std::unique_ptr<Script> ReadScript(std::string_view code,
                                     std::string_view debug_name = "");

  // Register a function with all language specific engines.
  template <typename Fn>
  void RegisterFunction(std::string_view name, const Fn& fn);

  // Unregister a function with all language specific engines.
  void UnregisterFunction(std::string_view name);

  // Registers an enum with all the language specific engines. This is done by
  // creating global variables with the given prefix followed by the identifier
  // and assigned the native enumeration value.
  //
  // For example, given
  //   enum class Days : ushort { Mon, Tue, Wed };
  //
  // Calling:
  //   script_engine->RegisterEnum<Days>("Days");
  //
  // Will register the following global values with the ScriptEngine:
  //    Days.Mon = (native value)Days::Mon
  //    Days.Tue = (native value)Days::Tue
  //    Days.Wed = (native value)Days::Wed
  template <typename En>
  void RegisterEnum(std::string_view prefix);

 protected:
  explicit ScriptEngine(Registry* registry);

  using ScriptableFn = std::function<absl::StatusCode(ScriptCallContext*)>;
  void DoRegisterFunction(std::string_view name, ScriptableFn fn);
  void DoSetEnumValue(std::string_view name, Var value);

  Registry* registry_;
};

template <typename Fn>
void ScriptEngine::RegisterFunction(std::string_view name, const Fn& fn) {
  auto wrapped_fn = [=](ScriptCallContext* context) {
    return CallNativeFunction(context, fn);
  };
  DoRegisterFunction(name, wrapped_fn);
}

template <typename En>
void ScriptEngine::RegisterEnum(std::string_view prefix) {
  for (const En& value : magic_enum::enum_values<En>()) {
    const std::string name = std::string(prefix) + std::string(".") +
                             std::string(magic_enum::enum_name(value));
    DoSetEnumValue(name, value);
  }
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ScriptEngine);

#endif  // REDUX_ENGINES_SCRIPT_SCRIPT_ENGINE_H_
