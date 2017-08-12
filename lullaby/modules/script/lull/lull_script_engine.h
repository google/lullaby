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

#ifndef LULLABY_MODULES_SCRIPT_LULL_LULL_SCRIPT_ENGINE_H_
#define LULLABY_MODULES_SCRIPT_LULL_LULL_SCRIPT_ENGINE_H_

#include <string>
#include <vector>

#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/util/registry.h"

namespace lull {

// ScriptEngine implementation for LullScript.  Loads and runs LullScript
// scripts.
class LullScriptEngine {
 public:
  explicit LullScriptEngine(Registry* registry);

  // Load a script from inline code. The debug_name is used when reporting error
  // messages.
  uint64_t LoadScript(const std::string& code, const std::string& debug_name);

  // Reloads a script, swapping out its code, but retaining its environment.
  void ReloadScript(uint64_t id, const std::string& code);

  // Run a loaded script.
  void RunScript(uint64_t id);

  // Register a function.
  template <typename Fn>
  void RegisterFunction(const std::string& name, const Fn& fn);

  // Unregister a function.
  void UnregisterFunction(const std::string& name) {}

  // Set a value in the script's environment.
  template <typename T>
  void SetValue(uint64_t id, const std::string& name, const T& value);

  // Get a value from the script's environment.
  template <typename T>
  bool GetValue(uint64_t id, const std::string& name, T* value);

 private:
  struct Script {
    ScriptEnv env;
    ScriptValue script;
    std::string debug_name;
  };

  Registry* registry_;
  uint64_t next_script_id_;
  std::unordered_map<uint64_t, Script> scripts_;
};

template <typename T>
void LullScriptEngine::SetValue(uint64_t id, const std::string& name,
                                const T& value) {
  auto iter = scripts_.find(id);
  if (iter != scripts_.end()) {
    ScriptEnv& env = iter->second.env;
    const HashValue symbol = Hash(name);
    const ScriptValue script_value = env.Create(value);
    env.SetValue(symbol, script_value);
  }
}

template <typename T>
bool LullScriptEngine::GetValue(uint64_t id, const std::string& name,
                                T* value) {
  auto iter = scripts_.find(id);
  if (iter == scripts_.end()) {
    return false;
  }

  ScriptEnv& env = iter->second.env;
  const HashValue symbol = Hash(name);
  const ScriptValue script_value = env.GetValue(symbol);
  if (script_value.Is<T>()) {
    *value = *script_value.Get<T>();
    return true;
  } else {
    return false;
  }
}

template <typename Fn>
void LullScriptEngine::RegisterFunction(const std::string& name, const Fn& fn) {
  auto* function_registry = registry_->Get<FunctionRegistry>();
  if (function_registry) {
    DCHECK(function_registry->IsFunctionRegistered(name));
  }
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LullScriptEngine);

#endif  // LULLABY_MODULES_SCRIPT_LULL_LULL_SCRIPT_ENGINE_H_
