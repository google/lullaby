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

#include "redux/engines/script/function_binder.h"

namespace redux {

FunctionBinder::FunctionBinder(Registry* registry) : registry_(registry) {}

FunctionBinder::~FunctionBinder() {
  auto* engine = GetScriptEngine();
  for (const std::string& name : functions_) {
    engine->UnregisterFunction(name);
  }
}

ScriptEngine* FunctionBinder::GetScriptEngine() {
  if (script_engine_ == nullptr) {
    script_engine_ = registry_->Get<ScriptEngine>();
    CHECK(script_engine_ != nullptr);
  }
  return script_engine_;
}

}  // namespace redux
