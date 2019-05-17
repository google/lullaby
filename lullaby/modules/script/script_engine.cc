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

#include "lullaby/modules/script/script_engine.h"

#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"

namespace lull {

namespace {

Language InferLanguageFromFileName(const std::string& filename) {
  size_t dot = filename.find_last_of('.');
  if (dot == std::string::npos) {
    return Language_Unknown;
  }

  std::string suffix = filename.substr(dot + 1);
  if (suffix == "ls" || suffix == "lsb") {
    return Language_LullScript;
  } else if (suffix == "lua") {
    return Language_Lua5_2;
  } else if (suffix == "js") {
    return Language_JavaScript;
  }

  return Language_Unknown;
}

}  // namespace

ScriptId::ScriptId() : lang_(Language_Unknown) {}

ScriptId::ScriptId(Language lang, uint64_t id) : lang_(lang), id_(id) {}

bool ScriptId::IsValid() const { return lang_ != Language_Unknown; }

ScriptEngine::ScriptEngine(Registry* registry) : registry_(registry) {}

ScriptId ScriptEngine::LoadScript(const std::string& filename) {
  return LoadScript(filename, filename);
}

ScriptId ScriptEngine::LoadScript(const std::string& filename,
                                  const std::string& debug_name) {
  Language lang = InferLanguageFromFileName(filename);
  if (lang == Language_Unknown) {
    LOG(ERROR) << "Couldn't infer script language from filename: " << filename;
    return ScriptId();
  }
  return LoadScript(filename, debug_name, lang);
}

ScriptId ScriptEngine::LoadScript(const std::string& filename,
                                  const std::string& debug_name,
                                  Language lang) {
  auto script = registry_->Get<AssetLoader>()->LoadNow<SimpleAsset>(filename);
  return LoadInlineScript(script->GetStringData(), debug_name, lang);
}

ScriptId ScriptEngine::LoadInlineScript(const std::string& code,
                                        const std::string& debug_name,
                                        Language lang) {
  auto it = engines_.find(lang);
  if (it == engines_.end()) {
    LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(lang);
    return ScriptId();
  }
  return ScriptId(lang, it->second->LoadScript(code, debug_name));
}

void ScriptEngine::ReloadScript(ScriptId id, const std::string& code) {
  auto it = engines_.find(id.lang_);
  if (it == engines_.end()) {
    LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
    return;
  }
  it->second->ReloadScript(id.id_, code);
}

void ScriptEngine::RunScript(ScriptId id) {
  auto it = engines_.find(id.lang_);
  if (it == engines_.end()) {
    LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
    return;
  }
  it->second->RunScript(id.id_);
}

void ScriptEngine::UnloadScript(ScriptId id) {
  auto it = engines_.find(id.lang_);
  if (it == engines_.end()) {
    LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
    return;
  }
  it->second->UnloadScript(id.id_);
}

void ScriptEngine::UnregisterFunction(const std::string& name) {
  for (auto& kv : engines_) {
    kv.second->UnregisterFunction(name);
  }
}

size_t ScriptEngine::GetTotalScripts() const {
  size_t total = 0;
  for (auto& kv : engines_) {
    total += kv.second->GetTotalScripts();
  }
  return total;
}

}  // namespace lull
