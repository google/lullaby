/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_LULLSCRIPT_LULL_SCRIPT_ENGINE_H_
#define LULLABY_MODULES_LULLSCRIPT_LULL_SCRIPT_ENGINE_H_

#include <string>
#include <vector>

#include "lullaby/modules/function/function_call.h"
#include "lullaby/modules/function/variant_converter.h"
#include "lullaby/modules/lullscript/script_env.h"
#include "lullaby/modules/script/script_engine.h"

namespace lull {

// ScriptEngine implementation for LullScript.  Loads and runs LullScript
// scripts.
class LullScriptEngine : public IScriptEngine {
 public:
  LullScriptEngine() {}

  static Language Lang() { return Language::Language_LullScript; }

  // LullScript doesn't have an include statement, so this is a no-op.
  void SetLoadFileFunction(const AssetLoader::LoadFileFn& fn) override {}

  // Load a script from inline code. The debug_name is used when reporting error
  // messages.
  uint64_t LoadScript(const std::string& code,
                      const std::string& debug_name) override;

  // Reloads a script, swapping out its code, but retaining its environment.
  void ReloadScript(uint64_t id, const std::string& code) override;

  // Run a loaded script.
  void RunScript(uint64_t id) override;

  // Unload a loaded script.
  void UnloadScript(uint64_t id) override;

  // Register a function to be callable from script.
  void RegisterFunction(const std::string& name, ScriptableFn) override;

  // Unregister a function.
  void UnregisterFunction(const std::string& name) override;

  // Set a value in the script's environment.
  void SetValue(uint64_t id, const std::string& name,
                const Variant& value) override;
  template <typename T>
  void SetValue(uint64_t id, const std::string& name, const T& value);

  // Get a value from the script's environment.
  bool GetValue(uint64_t id, const std::string& name, Variant* value) override;
  template <typename T>
  bool GetValue(uint64_t id, const std::string& name, T* value);

  // Returns the number of scripts currently loaded.
  size_t GetTotalScripts() const override;

  // Returns a new lullscript environment initialized from the base environment.
  std::unique_ptr<ScriptEnv> MakeEnv() const;

 private:
  struct Script {
    Script() {}
    explicit Script(const ScriptEnv& env) : env(env) {}

    ScriptEnv env;
    ScriptValue script;
    std::string debug_name;
  };

  uint64_t next_script_id_ = 0;
  ScriptEnv base_env_;
  std::unordered_map<uint64_t, Script> scripts_;
};

template <typename T>
void LullScriptEngine::SetValue(uint64_t id, const std::string& name,
                                const T& value) {
  Variant var;
  VariantConverter::ToVariant(value, &var);
  SetValue(id, name, var);
}

template <typename T>
bool LullScriptEngine::GetValue(uint64_t id, const std::string& name,
                                T* value) {
  Variant var;
  if (GetValue(id, name, &var)) {
    return VariantConverter::FromVariant(var, value);
  }
  return false;
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LullScriptEngine);

#endif  // LULLABY_MODULES_LULLSCRIPT_LULL_SCRIPT_ENGINE_H_
