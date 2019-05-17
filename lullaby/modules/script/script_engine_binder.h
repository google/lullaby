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

#ifndef LULLABY_MODULES_SCRIPT_SCRIPT_ENGINE_BINDER_H_
#define LULLABY_MODULES_SCRIPT_SCRIPT_ENGINE_BINDER_H_

#include "lullaby/modules/script/script_engine.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/typeid.h"

namespace lull {

/// A simple utility class that adds some function bindings for ScriptEngine,
/// and removes them when destroyed. It provides a way for script contexts to be
/// created and then later rereferenced again by integer ids. We don't implement
/// this for the underlying ScriptEngine so it is safer.
class ScriptEngineBinder {
 public:
  explicit ScriptEngineBinder(Registry* registry);
  ~ScriptEngineBinder();

  /// Create and register a new ScriptEngine in the Registry.
  static ScriptEngine* CreateEngine(Registry* registry);

  /// Create and register a new JavascriptEngine in the Registry.
  static void CreateJavascriptEngine(Registry* registry);

  /// Create and register this binder in the Registry. These functions must be
  /// called in this order so that cleanup happens properly when Registry is
  /// destroyed.
  static void CreateBinder(Registry* registry);

 private:
  // Will check IsValid() and not add if it isn't.
  uint32_t SetScriptId(const ScriptId& id);
  const ScriptId* GetScriptId(uint32_t index) const;
  void RemoveScriptId(uint32_t index);

  uint32_t LoadScript(const std::string& filename);
  uint32_t LoadInlineScript(const std::string& code,
                            const std::string& debug_name, int lang);
  void ReloadInlineScript(uint32_t index, const std::string& code);
  void RunScript(uint32_t index);
  void UnloadScript(uint32_t index);

  Registry* registry_;
  ScriptEngine* script_engine_;
  // Autoincrementing value for generating unique indices.
  uint32_t indices_generator_;
  std::unordered_map<uint32_t, ScriptId> ids_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScriptEngineBinder);

#endif  // LULLABY_MODULES_SCRIPT_SCRIPT_ENGINE_BINDER_H_
