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

#ifndef LULLABY_MODULES_JAVASCRIPT_ENGINE_H_
#define LULLABY_MODULES_JAVASCRIPT_ENGINE_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/modules/javascript/convert.h"
#include "lullaby/modules/script/script_engine.h"
#include "lullaby/util/hash.h"

namespace lull {
namespace script {
namespace javascript {

// Javascript script Engine. Loads and runs JS scripts on v8 engine.
class Engine : public IScriptEngine {
 public:
  Engine();

  ~Engine();

  static Language Lang() { return Language::Language_JavaScript; }

  // Sets the file loader used by the include function.
  void SetLoadFileFunction(const AssetLoader::LoadFileFn& load_fn) override;

  // Load a script from a file.
  uint64_t LoadScript(const std::string& filename);

  // Load a script from inline code. The debug_name is used when reporting error
  // messages.
  uint64_t LoadScript(const std::string& code,
                      const std::string& debug_name) override;

  // Reloads a script, swapping out its code, but retaining its environment.
  void ReloadScript(uint64_t id, const std::string& code) override;

  // Run a loaded script.
  void RunScript(uint64_t id) override;

  // Unloads a script.
  void UnloadScript(uint64_t id) override;

  // Register a function. This function will be available to all subsequently
  // loaded scripts, but not to scripts that were already loaded.
  void RegisterFunction(const std::string& name, ScriptableFn fn) override;

  template <typename Fn>
  void RegisterFunction(const std::string& name, const Fn& fn);

  // Unregister a function. This also removes it from all loaded scripts such
  // that calling it will throw an exception.
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

 private:
  v8::MaybeLocal<v8::Script> CompileScript(const std::string& code,
                                           const std::string& debug_name);

  v8::Global<v8::Context> CreateEnv();

  using JsLambda =
      std::function<void(const v8::FunctionCallbackInfo<v8::Value>&)>;

  void RegisterFunctionImpl(const std::string& name, JsLambda&& fn);

  v8::Local<v8::Object> IncludeImpl(const v8::FunctionCallbackInfo<v8::Value>&);

  void TriggerGarbageCollection();

  struct FunctionInfo {
    FunctionInfo(const std::string& name, JsLambda&& fn) : name(name), fn(fn) {}
    std::string name;
    JsLambda fn;
  };
  std::unordered_map<HashValue, std::unique_ptr<FunctionInfo>> functions_;

  struct ScriptInfo {
    std::string name;
    v8::Global<v8::Script> script;
    v8::Global<v8::Context> context;
  };
  std::unordered_map<uint64_t, std::unique_ptr<ScriptInfo>> scripts_;

  std::function<void(v8::Isolate*)> script_maintainer_fn_;

  std::unordered_map<std::string, uint64_t> included_scripts_;

  AssetLoader::LoadFileFn load_fn_;
  v8::ArrayBuffer::Allocator* allocator_;
  v8::Isolate* isolate_;
};

template <typename Fn>
void Engine::RegisterFunction(const std::string& name, const Fn& fn) {
  RegisterFunctionImpl(
      name, [name, fn](const v8::FunctionCallbackInfo<v8::Value>& args) {
        JsContext context(args);
        CallNativeFunction(&context, name.c_str(), fn);
      });
}

template <typename T>
void Engine::SetValue(uint64_t id, const std::string& name, const T& value) {
  using BaseT = typename std::decay<T>::type;
  ScriptInfo* script_info = reinterpret_cast<ScriptInfo*>(id);

  IsolateContextLocker lock(isolate_, script_info->context);

  v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate_, name.c_str());
  v8::Local<v8::Value> js_value = Convert<BaseT>::CppToJs(isolate_, value);

  v8::Local<v8::Object> global = lock.GetLocalContext()->Global();
  global->Set(key, js_value);
}

template <typename T>
bool Engine::GetValue(uint64_t id, const std::string& name, T* value) {
  using BaseT = typename std::decay<T>::type;
  ScriptInfo* script_info = reinterpret_cast<ScriptInfo*>(id);

  IsolateContextLocker lock(isolate_, script_info->context);

  v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate_, name.c_str());
  v8::Local<v8::Object> global = lock.GetLocalContext()->Global();
  v8::Local<v8::Value> js_value =
      global->Get(lock.GetLocalContext(), key).ToLocalChecked();
  if (js_value.IsEmpty() || js_value->IsUndefined()) {
    return false;
  }

  return Convert<BaseT>::JsToCpp(isolate_, js_value, value);
}

}  // namespace javascript
}  // namespace script
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::script::javascript::Engine);

#endif  // LULLABY_MODULES_JAVASCRIPT_ENGINE_H_
