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

#include "lullaby/modules/lua/engine.h"

#include <utility>

#include "lullaby/modules/lua/stack_checker.h"
#include "lullaby/modules/lua/util_script.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace script {
namespace lua {

Engine::Engine() : next_script_id_(1), util_loaded_(false), total_scripts_(0) {
  lua_ = lua_newstate(LuaAlloc, nullptr);
  luaL_openlibs(lua_);
  lua_checkstack(lua_, 2);
  lua_newtable(lua_);
  lua_setfield(lua_, LUA_REGISTRYINDEX, kFuncRegistryKey);
  lua_newtable(lua_);
  lua_setfield(lua_, LUA_REGISTRYINDEX, kScriptRegistryKey);

  lua_newtable(lua_);
  lua_pushinteger(lua_, 1);
  lua_setfield(lua_, -2, kCallbackIdKey);
  lua_setfield(lua_, LUA_REGISTRYINDEX, kCallbackRegistryKey);

  RegisterFunctionImpl("print", LuaPrint);
  RegisterFunctionImpl("include", Include);
  RegisterFunction("detail_MultiplyQuatByScalar",
                   [](mathfu::quat q, float s) { return q * s; });
  RegisterFunction("detail_MultiplyQuatByVec3",
                   [](mathfu::quat q, mathfu::vec3 v) { return q * v; });
  RegisterFunction("detail_MultiplyQuatByQuat",
                   [](mathfu::quat q, mathfu::quat r) { return q * r; });
}

Engine::~Engine() { lua_close(lua_); }

namespace {

static constexpr char kTextFormat[] = "t";

void CopyTable(lua_State* lua, int from_id, int to_id) {
  LUA_UTIL_EXPECT_STACK(lua, 0);
  lua_checkstack(lua, 3);
  lua_pushnil(lua);                      // +1
  while (lua_next(lua, from_id) != 0) {  // +1 while looping, -1 when done.
    lua_pushvalue(lua, -2);              // +1
    lua_insert(lua, -2);                 // 0
    lua_settable(lua, to_id);            // -2
  }
}

void AddFunctionToTable(
    lua_State* lua, int table_id, const std::string& name, int func_id) {
  LUA_UTIL_EXPECT_STACK(lua, 0);
  const size_t i = name.find('.');
  lua_checkstack(lua, 2);
  if (i == std::string::npos) {
    lua_pushvalue(lua, func_id);                            // +1
    lua_setfield(lua, table_id, name.c_str());              // -1
  } else {
    const std::string sub_table_name = name.substr(0, i);
    lua_getfield(lua, table_id, sub_table_name.c_str());    // +1
    if (lua_isnil(lua, -1)) {
      lua_pop(lua, 1);                                      // -1
      lua_newtable(lua);                                    // +1
      lua_pushvalue(lua, -1);                               // +1
      lua_setfield(lua, table_id, sub_table_name.c_str());  // -1
    }
    AddFunctionToTable(lua, lua_gettop(lua), name.substr(i + 1), func_id);
    lua_pop(lua, 1);                                        // -1
  }
}

template <typename Types>
struct ConverterImpl : public ConverterImpl<typename Types::Rest> {
  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     Variant* value) {
    if (auto* cpp_value = value->Get<typename Types::First>()) {
      return Convert<typename Types::First>::PopFromLuaToCpp(context,
                                                             cpp_value);
    }
    return ConverterImpl<typename Types::Rest>::PopFromLuaToCpp(context, value);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const Variant& value) {
    if (auto* cpp_value = value.Get<typename Types::First>()) {
      Convert<typename Types::First>::PushFromCppToLua(context, *cpp_value);
    } else {
      ConverterImpl<typename Types::Rest>::PushFromCppToLua(context, value);
    }
  }
};

template <>
struct ConverterImpl<EmptyList> {
  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     Variant* value) {
    return Convert<Variant>::PopFromLuaToCpp(context, value);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const Variant& value) {
    Convert<Variant>::PushFromCppToLua(context, value);
  }
};

struct Converter : ConverterImpl<ScriptableTypes> {};

}  // namespace

void Engine::SetLoadFileFunction(const AssetLoader::LoadFileFn& load_fn) {
  load_fn_ = load_fn;
}

uint64_t Engine::LoadScript(const std::string& filename) {
  if (!load_fn_) {
    LOG(ERROR) << "No LoadFileFn. Call SetLoadFileFunction first.";
    return 0;
  }
  std::string data;
  if (!load_fn_(filename.c_str(), &data)) {
    return 0;
  }
  return LoadScript(data, filename);
}

uint64_t Engine::LoadScript(const std::string& code,
                            const std::string& debug_name) {
  // Lazy load the util script, because it depends on some functions that are
  // loaded after the Engine constructor, and copy its environment to util_id_.
  if (!util_loaded_) {
    LUA_UTIL_EXPECT_STACK(lua_, 0);
    uint64_t util_script = LoadScriptImpl(detail::kUtilScript, "UtilScript");
    RunScript(util_script);
    GetScriptFromRegistry(util_script);
    lua_getupvalue(lua_, -1, 1);
    lua_setfield(lua_, LUA_REGISTRYINDEX, kUtilRegistryKey);
    lua_pop(lua_, 1);
    util_loaded_ = true;
  }
  return LoadScriptImpl(code, debug_name);
}

uint64_t Engine::LoadScriptImpl(const std::string& code,
                                const std::string& debug_name) {
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  Popper popper(lua_);
  lua_checkstack(lua_, 2);
  ScriptReaderState reader_state(code);
  const int err = lua_load(lua_, ScriptReader, &reader_state,
                           debug_name.c_str(), kTextFormat);
  if (err != 0) {
    LOG(ERROR) << "Error loading script: " << lua_tostring(lua_, -1);
    return 0;
  }
  int script_id = next_script_id_++;
  AddScriptToRegistry(script_id);
  CreateEnv();
  lua_setupvalue(lua_, -2, 1);
  SetValue(script_id, "debug_name", debug_name);
  return script_id;
}

void Engine::ReloadScript(uint64_t id, const std::string& code) {
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  Popper popper(lua_, 2);
  std::string debug_name;
  GetValue(id, "debug_name", &debug_name);
  lua_checkstack(lua_, 1);
  GetScriptFromRegistry(id);
  ScriptReaderState reader_state(code);
  const int err = lua_load(lua_, ScriptReader, &reader_state,
                           debug_name.c_str(), kTextFormat);
  if (err != 0) {
    LOG(ERROR) << "Error reloading script: " << lua_tostring(lua_, -1);
    return;
  }
  lua_upvaluejoin(lua_, -1, 1, -2, 1);
  AddScriptToRegistry(id);
}

void Engine::RunScript(uint64_t id) {
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  lua_checkstack(lua_, 2);
  GetScriptFromRegistry(id);
  if (lua_pcall(lua_, 0, LUA_MULTRET, 0) != 0) {
    LOG(ERROR) << "Script error: " << lua_tostring(lua_, -1);
    lua_pop(lua_, 1);
  }
}

void Engine::UnloadScript(uint64_t id) {
  // Unload the script by inserting nil into the registry entry. GC will clean
  // it up later.
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  lua_checkstack(lua_, 3);
  lua_getfield(lua_, LUA_REGISTRYINDEX, kScriptRegistryKey);
  lua_pushinteger(lua_, id);
  lua_pushnil(lua_);
  lua_settable(lua_, -3);
  lua_pop(lua_, 1);
  --total_scripts_;
}

size_t Engine::GetTotalScripts() const {
  return total_scripts_ - (util_loaded_ ? 1 : 0);
}

void Engine::CreateEnv() {
  LUA_UTIL_EXPECT_STACK(lua_, 1);

  // Make sure there's enough room on the stack. See comments on each function.
  lua_checkstack(lua_, 2);

  lua_newtable(lua_);         // +1 item on the stack.
  int id = lua_gettop(lua_);  // 0

  // Copy the global environment.
  lua_pushinteger(lua_, LUA_RIDX_GLOBALS);  // +1
  lua_gettable(lua_, LUA_REGISTRYINDEX);    // 0
  CopyTable(lua_, lua_gettop(lua_), id);    // 0
  lua_pop(lua_, 1);                         // -1

  // Add all the registered functions.
  lua_getfield(lua_, LUA_REGISTRYINDEX, kFuncRegistryKey);  // +1
  for (const auto& kv : functions_) {
    lua_getfield(lua_, lua_gettop(lua_), kv.second->name.c_str());  // +1
    AddFunctionToTable(lua_, id, kv.second->name, lua_gettop(lua_));
    lua_pop(lua_, 1);  // -1
  }
  lua_pop(lua_, 1);  // -1
}

void Engine::RegisterFunctionImpl(const std::string& name, LuaLambda&& fn) {
  functions_.emplace(
      Hash(name.c_str()),
      std::unique_ptr<FunctionInfo>(new FunctionInfo(name, std::move(fn))));
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  lua_checkstack(lua_, 3);
  lua_getfield(lua_, LUA_REGISTRYINDEX, kFuncRegistryKey);
  lua_pushlightuserdata(lua_, this);
  lua_pushstring(lua_, name.c_str());
  lua_pushcclosure(lua_, LambdaWrapper, 2);
  lua_setfield(lua_, -2, name.c_str());
  lua_pop(lua_, 1);
}

int Engine::LambdaWrapper(lua_State* lua) {
  int ret = -1;
  void* userdata = lua_touserdata(lua, lua_upvalueindex(1));
  Engine* engine = reinterpret_cast<Engine*>(userdata);
  {
    // Scope the c++ string object so it is properly destructed. We are using
    // lua5_2++ compiled with exceptions, so lua_error will throw one. But,
    // Lullaby is compiled without exceptions, so this code will not
    // automatically destruct for us.
    const std::string name = lua_tostring(lua, lua_upvalueindex(2));
    const auto it = engine->functions_.find(Hash(name.c_str()));
    if (it == engine->functions_.end()) {
      luaL_where(lua, 1);
      lua_pushfstring(lua, "Tried to call an unregistered function: %s",
                      name.c_str());
      lua_concat(lua, 2);
    } else {
      ret = it->second->fn(lua);
    }
  }
  if (ret == -1) {
    return lua_error(lua);
  }
  return ret;
}

int Engine::Include(lua_State* lua) {
  LUA_UTIL_EXPECT_STACK(lua, 0);
  if (lua_gettop(lua) != 1) {
    return luaL_error(lua, "include expects exactly 1 argument");
  }
  if (!lua_isstring(lua, 1)) {
    return luaL_error(lua, "include expects a string");
  }
  const char* file = lua_tostring(lua, 1);
  void* userdata = lua_touserdata(lua, lua_upvalueindex(1));
  Engine* engine = reinterpret_cast<Engine*>(userdata);
  return engine->IncludeImpl(file);
}

int Engine::IncludeImpl(const std::string& file) {
  auto itr = required_scripts_.find(file);
  uint64_t id;
  if (itr == required_scripts_.end()) {
    id = LoadScript(file);
    if (id == 0) {
      return luaL_error(lua_, "Could't find file: %s", file.c_str());
    }
    RunScript(id);
    required_scripts_[file] = id;
  } else {
    id = itr->second;
  }
  lua_pop(lua_, 1);
  GetScriptFromRegistry(id);
  lua_getupvalue(lua_, -1, 1);
  lua_remove(lua_, -2);
  return 1;
}

void Engine::UnregisterFunction(const std::string& name) {
  functions_.erase(Hash(name.c_str()));
}

void Engine::GetScriptFromRegistry(uint64_t id) {
  // Get the script with the given id from the script registry and leave it at
  // the top of the stack.
  LUA_UTIL_EXPECT_STACK(lua_, 1);
  lua_checkstack(lua_, 2);
  lua_getfield(lua_, LUA_REGISTRYINDEX, kScriptRegistryKey);
  lua_pushinteger(lua_, id);
  lua_gettable(lua_, -2);
  lua_remove(lua_, -2);
}

void Engine::AddScriptToRegistry(uint64_t id) {
  // Add the script at the top of the stack to the script registry at the given
  // id, but leave it at the top of the stack.
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  lua_checkstack(lua_, 3);
  lua_getfield(lua_, LUA_REGISTRYINDEX, kScriptRegistryKey);
  lua_pushinteger(lua_, id);
  lua_pushvalue(lua_, -3);
  lua_settable(lua_, -3);
  lua_pop(lua_, 1);
  ++total_scripts_;
}

void Engine::RegisterFunction(const std::string& name, ScriptableFn fn) {
  RegisterFunctionImpl(name, [name, fn](lua_State* lua) {
    ContextAdaptor<LuaContext> context((ConvertContext(lua)));
    return fn(&context);
  });
}

void Engine::SetValue(uint64_t id, const std::string& name,
                      const Variant& value) {
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  Popper popper(lua_, 2);
  GetScriptFromRegistry(id);
  lua_getupvalue(lua_, -1, 1);
  Converter::PushFromCppToLua(ConvertContext(lua_), value);
  lua_setfield(lua_, -2, name.c_str());
}

bool Engine::GetValue(uint64_t id, const std::string& name, Variant* value) {
  LUA_UTIL_EXPECT_STACK(lua_, 0);
  Popper popper(lua_, 2);
  GetScriptFromRegistry(id);
  lua_getupvalue(lua_, -1, 1);
  lua_getfield(lua_, -1, name.c_str());
  return Converter::PopFromLuaToCpp(ConvertContext(lua_), value);
}

}  // namespace lua
}  // namespace script
}  // namespace lull
