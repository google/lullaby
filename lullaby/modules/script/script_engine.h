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

#ifndef LULLABY_SCRIPT_SCRIPT_ENGINE_H_
#define LULLABY_SCRIPT_SCRIPT_ENGINE_H_

#include <string>

#include "lullaby/generated/script_def_generated.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/logging.h"

#ifdef LULLABY_SCRIPT_LUA
#include "lullaby/modules/script/lua/engine.h"
#endif
#include "lullaby/modules/script/lull/lull_script_engine.h"
#ifdef LULLABY_SCRIPT_JS
#include "lullaby/modules/script/javascript/engine.h"
#endif

namespace lull {

// A ScriptId is an opaque id that the ScriptEngine uses to manage scripts.
class ScriptId {
 public:
  ScriptId();

  // Returns whether the script is valid.
  bool IsValid() const;

 private:
  friend class ScriptEngine;

  ScriptId(Language lang, uint64_t id);

  Language lang_;
  uint64_t id_;
};

// The ScriptEngine loads and runs scripts by delegating to language specific
// engines.
class ScriptEngine {
 public:
  explicit ScriptEngine(Registry* registry);

  // Sets the function that will allow LullScript to invoke native functions via
  // a FunctionCall object.
  void SetFunctionCallHandler(FunctionCall::Handler handler);

  // Loads a script from a file, and infer the language from the filename.
  ScriptId LoadScript(const std::string& filename);

  // Loads a script from a file, and infer the language from the filename. The
  // debug_name is used when reporting error messages.
  ScriptId LoadScript(const std::string& filename,
                      const std::string& debug_name);

  // Loads a script from a file, with the given language. The debug_name is used
  // when reporting error messages.
  ScriptId LoadScript(const std::string& filename,
                      const std::string& debug_name, Language lang);

  // Loads a script from a string containing inline code for the given language.
  // The debug_name is used when reporting error messages.
  ScriptId LoadInlineScript(const std::string& code,
                            const std::string& debug_name, Language lang);

  // Reloads a script, swapping out its code, but retaining its environment.
  void ReloadScript(ScriptId id, const std::string& code);

  // Runs a loaded script.
  void RunScript(ScriptId id);

  // Register a function with all language specific engines.
  template <typename Fn>
  void RegisterFunction(const std::string& name, const Fn& function);

  // Unregister a function with all language specific engines.
  void UnregisterFunction(const std::string& name);

  // Set a value in the script's environment.
  template <typename T>
  void SetValue(ScriptId id, const std::string& name, const T& value);

  // Get a value from the script's environment.
  template <typename T>
  bool GetValue(ScriptId id, const std::string& name, T* t);

 private:
  Registry* registry_;

  LullScriptEngine lull_engine_;
#ifdef LULLABY_SCRIPT_LUA
  script::lua::Engine lua_engine_;
#endif
#ifdef LULLABY_SCRIPT_JS
  script::javascript::Engine js_engine_;
#endif
};

template <typename Fn>
void ScriptEngine::RegisterFunction(const std::string& name,
                                    const Fn& function) {
#ifdef LULLABY_SCRIPT_LUA
  lua_engine_.RegisterFunction(name, function);
#endif
#ifdef LULLABY_SCRIPT_JS
  js_engine_.RegisterFunction(name, function);
#endif
}

template <typename T>
void ScriptEngine::SetValue(ScriptId id, const std::string& name,
                            const T& value) {
  switch (id.lang_) {
    case Language_LullScript:
      lull_engine_.SetValue(id.id_, name, value);
      return;
#ifdef LULLABY_SCRIPT_LUA
    case Language_Lua5_2:
      lua_engine_.SetValue(id.id_, name, value);
      return;
#endif
#ifdef LULLABY_SCRIPT_JS
    case Language_JavaScript:
      js_engine_.SetValue(id.id_, name, value);
      return;
#endif
    default:
      LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
  }
}

template <typename T>
bool ScriptEngine::GetValue(ScriptId id, const std::string& name, T* t) {
  switch (id.lang_) {
    case Language_LullScript:
      return lull_engine_.GetValue<T>(id.id_, name, t);
#ifdef LULLABY_SCRIPT_LUA
    case Language_Lua5_2:
      return lua_engine_.GetValue<T>(id.id_, name, t);
#endif
#ifdef LULLABY_SCRIPT_JS
    case Language_JavaScript:
      return js_engine_.GetValue<T>(id.id_, name, t);
#endif
    default:
      LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
  }
  return false;
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScriptEngine);

#endif  // LULLABY_SCRIPT_SCRIPT_ENGINE_H_
