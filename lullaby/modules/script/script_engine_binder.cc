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

#include "lullaby/modules/script/script_engine_binder.h"

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/built_in_functions.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {
constexpr uint32_t kInvalidIndex = 0;
}  // namespace

ScriptEngineBinder::ScriptEngineBinder(Registry* registry)
    : registry_(registry), indices_generator_(0) {
  auto* binder = registry->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }
  script_engine_ = registry->Get<ScriptEngine>();
  if (!script_engine_) {
    LOG(DFATAL) << "No ScriptEngine.";
    return;
  }

  // Note: This is applied to the ScriptEngine, not the FunctionBinder.
  // Currently FunctionBinder automatically does this to itself when it's
  // constructed. If FunctionBinder is created before ScriptEngine is created,
  // then we can't register the same functions through the FunctionBinder again.
  // If you are creating FunctionBinder after ScriptEngine, then you don't need
  // to call this.
  binder->RegisterFunction(
      "lull.ScriptEngine.RegisterBuiltInFunctions",
      [this]() { RegisterBuiltInFunctions(script_engine_); });
  binder->RegisterFunction(
      "lull.ScriptEngine.LoadScript",
      [this](const std::string& filename) { return LoadScript(filename); });
  binder->RegisterFunction(
      "lull.ScriptEngine.LoadInlineScript",
      [this](const std::string& code, const std::string& debug_name, int lang) {
        return LoadInlineScript(code, debug_name, lang);
      });
  // TODO".
  binder->RegisterFunction("lull.ScriptEngine.ReloadInlineScript",
                           [this](uint32_t index, const std::string& code) {
                             ReloadInlineScript(index, code);
                           });
  binder->RegisterFunction("lull.ScriptEngine.RunScript",
                           [this](uint32_t index) { RunScript(index); });
  binder->RegisterFunction("lull.ScriptEngine.UnloadScript",
                           [this](uint32_t index) { UnloadScript(index); });
}

ScriptEngineBinder::~ScriptEngineBinder() {
  auto* binder = registry_->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->UnregisterFunction("lull.ScriptEngine.RegisterBuiltInFunctions");
  binder->UnregisterFunction("lull.ScriptEngine.LoadScript");
  binder->UnregisterFunction("lull.ScriptEngine.LoadInlineScript");
  binder->UnregisterFunction("lull.ScriptEngine.ReloadInlineScript");
  binder->UnregisterFunction("lull.ScriptEngine.RunScript");
  binder->UnregisterFunction("lull.ScriptEngine.UnloadScript");
}

ScriptEngine* ScriptEngineBinder::CreateEngine(Registry* registry) {
  return registry->Create<ScriptEngine>(registry);
}

void ScriptEngineBinder::CreateJavascriptEngine(Registry* registry) {
}

void ScriptEngineBinder::CreateBinder(Registry* registry) {
  registry->Create<ScriptEngineBinder>(registry);
}

uint32_t ScriptEngineBinder::SetScriptId(const ScriptId& id) {
  if (!id.IsValid()) {
    return kInvalidIndex;
  }
  const uint32_t index = ++indices_generator_;
  CHECK_NE(index, 0) << "Overflow on script id generation.";
  ids_[index] = id;
  return index;
}

const ScriptId* ScriptEngineBinder::GetScriptId(uint32_t index) const {
  const auto iter = ids_.find(index);
  if (iter == ids_.end()) {
    return nullptr;
  }
  return &iter->second;
}

void ScriptEngineBinder::RemoveScriptId(uint32_t index) { ids_.erase(index); }

uint32_t ScriptEngineBinder::LoadScript(const std::string& filename) {
  ScriptId id = script_engine_->LoadScript(filename);
  return SetScriptId(id);
}

uint32_t ScriptEngineBinder::LoadInlineScript(const std::string& code,
                                              const std::string& debug_name,
                                              int lang) {
  ScriptId id = script_engine_->LoadInlineScript(code, debug_name,
                                                static_cast<Language>(lang));
  return SetScriptId(id);
}

void ScriptEngineBinder::ReloadInlineScript(uint32_t index,
                                            const std::string& code) {
  const ScriptId* id = GetScriptId(index);
  if (id) {
    script_engine_->ReloadScript(*id, code);
  }
}

void ScriptEngineBinder::RunScript(uint32_t index) {
  const ScriptId* id = GetScriptId(index);
  if (id) {
    script_engine_->RunScript(*id);
  }
}

void ScriptEngineBinder::UnloadScript(uint32_t index) {
  const ScriptId* id = GetScriptId(index);
  if (id) {
    script_engine_->UnloadScript(*id);
    RemoveScriptId(index);
  }
}

}  // namespace lull
