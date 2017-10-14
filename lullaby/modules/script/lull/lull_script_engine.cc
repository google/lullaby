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

#include "lullaby/modules/script/lull/lull_script_engine.h"

namespace lull {

void LullScriptEngine::SetFunctionCallHandler(FunctionCall::Handler handler) {
  handler_ = std::move(handler);
}

uint64_t LullScriptEngine::LoadScript(const std::string& code,
                                      const std::string& debug_name) {
  const uint64_t id = ++next_script_id_;
  Script& script = scripts_[id];
  script.env.SetFunctionCallHandler(handler_);
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

}  // namespace lull
