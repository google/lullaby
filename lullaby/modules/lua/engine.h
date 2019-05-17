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

#ifndef LULLABY_MODULES_LUA_ENGINE_H_
#define LULLABY_MODULES_LUA_ENGINE_H_

#include <string>
#include <vector>

#include "lullaby/modules/function/call_native_function.h"
#include "lullaby/modules/lua/convert.h"
#include "lullaby/modules/lua/utils.h"
#include "lullaby/modules/script/script_engine.h"

namespace lull {
namespace script {
namespace lua {

// Lua specific script Engine. Loads and runs Lua 5.2 scripts.
class Engine : public IScriptEngine {
 public:
  Engine();

  ~Engine();

  static Language Lang() { return Language::Language_Lua5_2; }

  // Sets the file loader used by the require function.
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

  // Unregister a function with all language specific engines.
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
  using LuaLambda = std::function<int(lua_State*)>;
  struct FunctionInfo {
    FunctionInfo(const std::string& name, LuaLambda&& fn)
        : name(name), fn(fn) {}
    std::string name;
    LuaLambda fn;
  };

  template <typename T>
  struct LuaReturn;

  void CreateEnv();

  void RegisterFunctionImpl(const std::string& name, LuaLambda&& fn);

  uint64_t LoadScriptImpl(const std::string& code,
                          const std::string& debug_name);

  void AddScriptToRegistry(uint64_t id);

  void GetScriptFromRegistry(uint64_t id);

  static int LambdaWrapper(lua_State* lua);

  static int Include(lua_State* lua);

  int IncludeImpl(const std::string& file);

  lua_State* lua_;
  AssetLoader::LoadFileFn load_fn_;
  int next_script_id_;
  bool util_loaded_;
  std::unordered_map<HashValue, std::unique_ptr<FunctionInfo>> functions_;
  std::unordered_map<std::string, uint64_t> required_scripts_;
  size_t total_scripts_;
};

template <typename Fn>
void Engine::RegisterFunction(const std::string& name, const Fn& fn) {
  RegisterFunctionImpl(name, [name, fn](lua_State* lua) {
    LuaContext context((ConvertContext(lua)));
    if (!CallNativeFunction(&context, name.c_str(), fn)) {
      return -1;
    }
    return LuaReturn<decltype(&Fn::operator())>::kNumValuesOnStack;
  });
}

template <typename T>
void Engine::SetValue(uint64_t id, const std::string& name, const T& value) {
  using BaseT = typename std::decay<T>::type;
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  Popper popper(lua_, 2);
  GetScriptFromRegistry(id);
  lua_getupvalue(lua_, -1, 1);
  Convert<BaseT>::PushFromCppToLua(ConvertContext(lua_), value);
  lua_setfield(lua_, -2, name.c_str());
}

template <typename T>
bool Engine::GetValue(uint64_t id, const std::string& name, T* value) {
  using BaseT = typename std::decay<T>::type;
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  Popper popper(lua_, 2);
  GetScriptFromRegistry(id);
  lua_getupvalue(lua_, -1, 1);
  lua_getfield(lua_, -1, name.c_str());
  return Convert<BaseT>::PopFromLuaToCpp(ConvertContext(lua_), value);
}

// LuaReturn<Fn>::value is 0 if the function returns void, and 1 otherwise,
// because we have to tell Lua how many values we're returning.
template <typename F, typename Return, typename... Args>
struct Engine::LuaReturn<Return (F::*)(Args...) const> {
  constexpr static int kNumValuesOnStack = std::is_void<Return>::value ? 0 : 1;
};

}  // namespace lua
}  // namespace script
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::script::lua::Engine);

#endif  // LULLABY_MODULES_LUA_ENGINE_H_
