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

ScriptEngine::ScriptEngine(Registry* registry) : registry_(registry) {
#ifdef LULLABY_SCRIPT_LUA
  AssetLoader* assetLoader = registry->Get<AssetLoader>();
  CHECK(assetLoader) << "No asset loader";
  lua_engine_.SetLoadFileFunction(assetLoader->GetLoadFunction());
#endif
#ifdef LULLABY_SCRIPT_JS
  AssetLoader* assetLoader = registry->Get<AssetLoader>();
  CHECK(assetLoader) << "No asset loader";
  js_engine_.SetLoadFileFunction(assetLoader->GetLoadFunction());
#endif
}

void ScriptEngine::SetFunctionCallHandler(FunctionCall::Handler handler) {
  lull_engine_.SetFunctionCallHandler(std::move(handler));
}

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
  switch (lang) {
    case Language_LullScript:
      return ScriptId(lang, lull_engine_.LoadScript(code, debug_name));
#ifdef LULLABY_SCRIPT_LUA
    case Language_Lua5_2:
      return ScriptId(lang, lua_engine_.LoadScript(code, debug_name));
#endif
#ifdef LULLABY_SCRIPT_JS
    case Language_JavaScript:
      return ScriptId(lang, js_engine_.LoadScript(code, debug_name));
#endif
    default:
      LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(lang);
      return ScriptId();
  }
}

void ScriptEngine::ReloadScript(ScriptId id, const std::string& code) {
  switch (id.lang_) {
    case Language_LullScript:
      lull_engine_.ReloadScript(id.id_, code);
      return;
#ifdef LULLABY_SCRIPT_LUA
    case Language_Lua5_2:
      lua_engine_.ReloadScript(id.id_, code);
      return;
#endif
#ifdef LULLABY_SCRIPT_JS
    case Language_JavaScript:
      js_engine_.ReloadScript(id.id_, code);
      return;
#endif
    default:
      LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
  }
}

void ScriptEngine::RunScript(ScriptId id) {
  switch (id.lang_) {
    case Language_LullScript:
      lull_engine_.RunScript(id.id_);
      return;
#ifdef LULLABY_SCRIPT_LUA
    case Language_Lua5_2:
      lua_engine_.RunScript(id.id_);
      return;
#endif
#ifdef LULLABY_SCRIPT_JS
    case Language_JavaScript:
      js_engine_.RunScript(id.id_);
      return;
#endif
    default:
      LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
  }
}

void ScriptEngine::UnregisterFunction(const std::string& name) {
#ifdef LULLABY_SCRIPT_LUA
  lua_engine_.UnregisterFunction(name);
#endif
#ifdef LULLABY_SCRIPT_JS
  js_engine_.UnregisterFunction(name);
#endif
}

}  // namespace lull
