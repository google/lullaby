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

#include "lullaby/modules/lullscript/lull_script_engine.h"

#include "absl/memory/memory.h"

namespace lull {

uint64_t LullScriptEngine::LoadScript(const std::string& code,
                                      const std::string& debug_name) {
  const uint64_t id = ++next_script_id_;
  CHECK_NE(id, 0) << "Overflow on script id generation.";
  Script& script = scripts_.emplace(id, Script(base_env_)).first->second;
  script.debug_name = debug_name;
  script.script = script.env.Read(code);
  return id;
}

void LullScriptEngine::ReloadScript(uint64_t id, const std::string& code) {
  auto iter = scripts_.find(id);
  if (iter != scripts_.end()) {
    iter->second.script = iter->second.env.Read(code);
  }
}

void LullScriptEngine::RunScript(uint64_t id) {
  auto iter = scripts_.find(id);
  if (iter != scripts_.end()) {
    iter->second.env.Eval(iter->second.script);
  }
}

void LullScriptEngine::UnloadScript(uint64_t id) { scripts_.erase(id); }

void LullScriptEngine::RegisterFunction(const std::string& name,
                                        ScriptableFn fn) {
  base_env_.Register(name, fn);
}

void LullScriptEngine::UnregisterFunction(const std::string& name) {
  // Because of the way lullscript symbol tables are implemented, it's not
  // practical to unregister a function.  Instead, we re-bind the symbol to
  // a no-op function that always indicates an error.
  RegisterFunction(name, [](IContext*) { return -1; });
}

void LullScriptEngine::SetValue(uint64_t id, const std::string& name,
                                const Variant& value) {
  auto iter = scripts_.find(id);
  if (iter != scripts_.end()) {
    ScriptValue script_value = ScriptValue::CreateFromVariant(value);

    ScriptEnv& env = iter->second.env;
    env.SetValue(Symbol(name), std::move(script_value));
  }
}

bool LullScriptEngine::GetValue(uint64_t id, const std::string& name,
                                Variant* value) {
  auto iter = scripts_.find(id);
  if (iter == scripts_.end()) {
    return false;
  }

  ScriptEnv& env = iter->second.env;
  const ScriptValue script_value = env.GetValue(Symbol(name));
  const Variant* var = script_value.GetVariant();
  if (var) {
    *value = *var;
    return true;
  } else {
    return false;
  }
}

size_t LullScriptEngine::GetTotalScripts() const { return scripts_.size(); }

std::unique_ptr<ScriptEnv> LullScriptEngine::MakeEnv() const {
  return absl::make_unique<ScriptEnv>(base_env_);
}

}  // namespace lull
