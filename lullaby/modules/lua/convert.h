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

#ifndef LULLABY_MODULES_LUA_CONVERT_H_
#define LULLABY_MODULES_LUA_CONVERT_H_

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "lua5.2/src/lauxlib.h"
#include "lua5.2/src/lualib.h"
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/function/call_native_function.h"
#include "lullaby/modules/lua/stack_checker.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/typeid.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace script {
namespace lua {

constexpr char kUtilRegistryKey[] = "LullabyUtil";
constexpr char kFuncRegistryKey[] = "LullabyFunc";
constexpr char kScriptRegistryKey[] = "LullabyScript";
constexpr char kCallbackRegistryKey[] = "LullabyCallback";
constexpr char kCallbackIdKey[] = "id";

struct ConvertContext {
  explicit ConvertContext(lua_State* lua) : lua(lua) {}
  lua_State* lua;
};

class LuaContext {
 public:
  explicit LuaContext(const ConvertContext& context) : context_(context) {}

  template <typename T>
  bool ArgToCpp(const char* func_name, size_t arg_index, T* value);

  template <typename T>
  bool ReturnFromCpp(const char* func_name, const T& value);

  bool CheckNumArgs(const char* func_name, size_t expected_args) const {
    const size_t num_args = lua_gettop(context_.lua);
    if (num_args != expected_args) {
      luaL_where(context_.lua, 1);
      lua_pushfstring(context_.lua, "%s expects %d args, but got %d", func_name,
                      expected_args, num_args);
      lua_concat(context_.lua, 2);
      return false;
    }
    return true;
  }

 private:
  ConvertContext context_;
};

struct Popper {
  explicit Popper(lua_State* lua, int n = 1) : lua(lua), n(n) {}
  ~Popper() { lua_pop(lua, n); }
  lua_State* lua;
  int n;
};

template <typename T,
          bool IsInt = std::is_integral<T>::value || std::is_enum<T>::value,
          bool IsFloat = std::is_floating_point<T>::value,
          bool IsMap = detail::IsMap<T>::kValue>
struct Convert;

template <>
struct Convert<bool, true, false, false> {
  static const char* GetLuaTypeName() { return "boolean"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     bool* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_isboolean(context.lua, -1)) {
      return false;
    }
    *value = lua_toboolean(context.lua, -1);
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      bool value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 1);
    lua_pushboolean(context.lua, value);
  }
};

template <typename T>
struct Convert<T, true, false, false> {
  static const char* GetLuaTypeName() { return "number"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context, T* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_isnumber(context.lua, -1)) {
      return false;
    }
    *value = static_cast<T>(lua_tointeger(context.lua, -1));
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context, T value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 1);
    lua_pushinteger(context.lua, static_cast<lua_Integer>(value));
  }
};

template <typename T>
struct Convert<T, false, true, false> {
  static const char* GetLuaTypeName() { return "number"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context, T* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_isnumber(context.lua, -1)) {
      return false;
    }
    *value = static_cast<T>(lua_tonumber(context.lua, -1));
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context, T value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 1);
    lua_pushnumber(context.lua, value);
  }
};

template <>
struct Convert<Clock::duration, false, false, false> {
  static const char* GetLuaTypeName() { return "number"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     Clock::duration* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_isnumber(context.lua, -1)) {
      return false;
    }
    *value = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<int64_t, std::nano>(
            static_cast<int64_t>(lua_tonumber(context.lua, -1))));
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const Clock::duration& value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 1);
    lua_pushnumber(context.lua, value.count());
  }
};

template <>
struct Convert<Entity, false, false, false> {
  static const char* GetLuaTypeName() { return "number"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     Entity* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_isnumber(context.lua, -1)) {
      return false;
    }
    *value = static_cast<int32_t>(lua_tointeger(context.lua, -1));
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      Entity value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 1);
    lua_pushinteger(context.lua, static_cast<lua_Integer>(value.AsUint32()));
  }
};

template <>
struct Convert<std::string, false, false, false> {
  static const char* GetLuaTypeName() { return "string"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     std::string* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_isstring(context.lua, -1)) {
      return false;
    }
    *value = lua_tostring(context.lua, -1);
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const std::string& value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 1);
    lua_pushstring(context.lua, value.c_str());
  }
};

namespace detail {

template <typename... Args>
struct StructFromLuaToCpp;

template <typename T, typename... Args>
struct StructFromLuaToCpp<const char*, T*, Args...> {
  static inline bool Call(const ConvertContext& context, const char* field_name,
                          T* value, Args... args) {
    LUA_UTIL_EXPECT_STACK(context.lua, 0);
    lua_getfield(context.lua, -1, field_name);
    if (!Convert<T>::PopFromLuaToCpp(context, value)) {
      return false;
    }
    return StructFromLuaToCpp<Args...>::Call(context, args...);
  }
};

template <>
struct StructFromLuaToCpp<> {
  static inline bool Call(const ConvertContext& context) { return true; }
};

template <typename... Args>
struct StructFromCppToLua;

template <typename T, typename... Args>
struct StructFromCppToLua<const char*, T, Args...> {
  static inline void Call(const ConvertContext& context, int id,
                          const char* field_name, const T& value,
                          Args... args) {
    LUA_UTIL_EXPECT_STACK(context.lua, 0);
    Convert<T>::PushFromCppToLua(context, value);
    lua_setfield(context.lua, id, field_name);
    StructFromCppToLua<Args...>::Call(context, id, args...);
  }
};

template <>
struct StructFromCppToLua<> {
  static inline void Call(const ConvertContext& context, int id) {}
};

template <typename... Args>
inline bool PopStructFromLuaToCpp(const ConvertContext& context, Args... args) {
  LUA_UTIL_EXPECT_STACK(context.lua, -1);
  Popper popper(context.lua);
  if (!lua_istable(context.lua, -1)) {
    return false;
  }
  lua_checkstack(context.lua, 1);
  return StructFromLuaToCpp<Args...>::Call(context, args...);
}

template <typename... Args>
inline void PushStructFromCppToLua(const ConvertContext& context,
                                   const char* metatable, Args... args) {
  LUA_UTIL_EXPECT_STACK(context.lua, 1);
  lua_checkstack(context.lua, 2);
  lua_newtable(context.lua);
  int id = lua_gettop(context.lua);
  if (metatable != nullptr) {
    lua_getfield(context.lua, LUA_REGISTRYINDEX, kUtilRegistryKey);
    lua_getfield(context.lua, lua_gettop(context.lua), metatable);
    lua_setmetatable(context.lua, id);
    lua_pop(context.lua, 1);
  }
  StructFromCppToLua<Args...>::Call(context, id, args...);
}

}  // namespace detail

template <typename T>
struct Convert<Optional<T>, false, false, false> {
  static const char* GetLuaTypeName() {
    return "value or nil";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     Optional<T>* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    T t;
    if (lua_isnil(context.lua, -1)) {
      Popper popper(context.lua);
      value->reset();
      return true;
    } else if (!Convert<T>::PopFromLuaToCpp(context, &t)) {
      return false;
    }
    *value = t;
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const Optional<T>& value) {
    if (value) {
      Convert<T>::PushFromCppToLua(context, *value);
    } else {
      LUA_UTIL_EXPECT_STACK(context.lua, 1);
      lua_checkstack(context.lua, 1);
      lua_pushnil(context.lua);
    }
  }
};

template <typename T>
struct Convert<mathfu::Vector<T, 2>, false, false, false> {
  static const char* GetLuaTypeName() {
    return "table like {x=number, y=number}";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     mathfu::Vector<T, 2>* value) {
    return detail::PopStructFromLuaToCpp(context, "x", &value->x, "y",
                                         &value->y);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const mathfu::Vector<T, 2>& value) {
    detail::PushStructFromCppToLua(context, "vec2_metatable", "x", value.x, "y",
                                   value.y);
  }
};

template <typename T>
struct Convert<mathfu::Vector<T, 3>, false, false, false> {
  static const char* GetLuaTypeName() {
    return "table like {x=number, y=number, z=number}";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     mathfu::Vector<T, 3>* value) {
    return detail::PopStructFromLuaToCpp(context, "x", &value->x, "y",
                                         &value->y, "z", &value->z);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const mathfu::Vector<T, 3>& value) {
    detail::PushStructFromCppToLua(context, "vec3_metatable", "x", value.x, "y",
                                   value.y, "z", value.z);
  }
};

template <typename T>
struct Convert<mathfu::Vector<T, 4>, false, false, false> {
  static const char* GetLuaTypeName() {
    return "table like {x=number, y=number, z=number, w=number}";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     mathfu::Vector<T, 4>* value) {
    return detail::PopStructFromLuaToCpp(context, "x", &value->x, "y",
                                         &value->y, "z", &value->z, "w",
                                         &value->w);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const mathfu::Vector<T, 4>& value) {
    detail::PushStructFromCppToLua(context, "vec4_metatable", "x", value.x, "y",
                                   value.y, "z", value.z, "w", value.w);
  }
};

template <>
struct Convert<mathfu::quat, false, false, false> {
  static const char* GetLuaTypeName() {
    return "table like {x=number, y=number, z=number, s=number}";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     mathfu::quat* value) {
    float x, y, z, s;
    const bool ret = detail::PopStructFromLuaToCpp(context, "x", &x, "y", &y,
                                                   "z", &z, "s", &s);
    if (ret) {
      value->set_vector(mathfu::vec3(x, y, z));
      value->set_scalar(s);
    }
    return ret;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const mathfu::quat& value) {
    detail::PushStructFromCppToLua(context, "quat_metatable", "x",
                                   value.vector().x, "y", value.vector().y, "z",
                                   value.vector().z, "s", value.scalar());
  }
};

template <typename T>
struct Convert<mathfu::Rect<T>, false, false, false> {
  static const char* GetLuaTypeName() {
    static std::string type = std::string("table like {pos=") +
                              Convert<mathfu::Vector<T, 2>>::GetLuaTypeName() +
                              std::string(", size=") +
                              Convert<mathfu::Vector<T, 2>>::GetLuaTypeName() +
                              std::string("}");
    return type.c_str();
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     mathfu::Rect<T>* value) {
    return detail::PopStructFromLuaToCpp(context, "pos", &value->pos, "size",
                                         &value->size);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const mathfu::Rect<T>& value) {
    detail::PushStructFromCppToLua(context, nullptr, "pos", value.pos, "size",
                                   value.size);
  }
};

template <>
struct Convert<Aabb, false, false, false> {
  static const char* GetLuaTypeName() {
    return "table like {min=vec3, max=vec3}";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     Aabb* value) {
    return detail::PopStructFromLuaToCpp(
        context, "min", &value->min, "max", &value->max);
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const Aabb& value) {
    detail::PushStructFromCppToLua(
        context, nullptr, "min", value.min, "max", value.max);
  }
};

template <>
struct Convert<mathfu::mat4, false, false, false> {
  static const char* GetLuaTypeName() {
    return "table like {c0=vec4, c1=vec4, c2=vec4, c3=vec4}";
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     mathfu::mat4* value) {
    return detail::PopStructFromLuaToCpp(
        context, "c0", &value->GetColumn(0), "c1", &value->GetColumn(1), "c2",
        &value->GetColumn(2), "c3", &value->GetColumn(3));
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const mathfu::mat4& value) {
    detail::PushStructFromCppToLua(
        context, nullptr, "c0", value.GetColumn(0), "c1", value.GetColumn(1),
        "c2", value.GetColumn(2), "c3", value.GetColumn(3));
  }
};

template <typename T>
struct Convert<std::vector<T>, false, false, false> {
  static const char* GetLuaTypeName() {
    static std::string type =
        std::string("table of ") + Convert<T>::GetLuaTypeName();
    return type.c_str();
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     std::vector<T>* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_istable(context.lua, -1)) {
      return false;
    }
    lua_checkstack(context.lua, 2);
    std::vector<T> v;
    T t;
    lua_pushnil(context.lua);
    while (lua_next(context.lua, -2)) {
      if (!Convert<T>::PopFromLuaToCpp(context, &t)) {
        lua_pop(context.lua, 1);
        return false;
      }
      v.push_back(t);
    }
    *value = std::move(v);
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const std::vector<T>& value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 3);
    lua_newtable(context.lua);
    int id = lua_gettop(context.lua);
    for (size_t i = 0; i < value.size(); ++i) {
      lua_pushnumber(context.lua, static_cast<lua_Number>(i + 1));
      Convert<T>::PushFromCppToLua(context, value[i]);
      lua_settable(context.lua, id);
    }
  }
};

template <typename M>
struct Convert<M, false, false, true> {
  using K = typename M::key_type;
  using V = typename M::mapped_type;

  static const char* GetLuaTypeName() {
    static std::string type =
        std::string("table mapping ") + Convert<K>::GetLuaTypeName() +
        std::string(" to ") + Convert<V>::GetLuaTypeName();
    return type.c_str();
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context, M* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_istable(context.lua, -1)) {
      return false;
    }
    lua_checkstack(context.lua, 2);

    // If the map is keyed by HashValues, hash any string keys in the table, so
    // that setting a string key in the Lua table sets the corresponding hash
    // key in the C++ map.
    if (std::is_same<K, HashValue>::value) {
      lua_getfield(context.lua, LUA_REGISTRYINDEX, kUtilRegistryKey);
      lua_getfield(context.lua, lua_gettop(context.lua), "hash_table_keys");
      lua_remove(context.lua, -2);
      lua_insert(context.lua, -2);
      lua_call(context.lua, 1, 1);
    }

    M m;
    K key;
    V val;
    lua_pushnil(context.lua);
    while (lua_next(context.lua, -2)) {
      if (!Convert<V>::PopFromLuaToCpp(context, &val)) {
        lua_pop(context.lua, 1);
        return false;
      }
      lua_pushvalue(context.lua, -1);
      if (!Convert<K>::PopFromLuaToCpp(context, &key)) {
        lua_pop(context.lua, 1);
        return false;
      }
      m[key] = val;
    }
    *value = std::move(m);
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const M& value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 3);
    lua_newtable(context.lua);
    int id = lua_gettop(context.lua);

    // If the map is keyed by HashValues, add the serializable_metatable, so
    // that on the Lua side, we can look up a hash key by its corresponding
    // string key.
    if (std::is_same<K, HashValue>::value) {
      lua_getfield(context.lua, LUA_REGISTRYINDEX, kUtilRegistryKey);
      lua_getfield(context.lua, lua_gettop(context.lua),
                   "serializable_metatable");
      lua_setmetatable(context.lua, id);
      lua_pop(context.lua, 1);
    }

    for (const auto& kv : value) {
      Convert<K>::PushFromCppToLua(context, kv.first);
      Convert<V>::PushFromCppToLua(context, kv.second);
      lua_settable(context.lua, id);
    }
  }
};

template <>
struct Convert<EventWrapper, false, false, false> {
  static const char* GetLuaTypeName() {
    static std::string type =
        std::string("table like {type=") +
        Convert<HashValue>::GetLuaTypeName() + std::string(", data=") +
        Convert<VariantMap>::GetLuaTypeName() + std::string("}");
    return type.c_str();
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     EventWrapper* value) {
    HashValue type;
    VariantMap data;
    if (!detail::PopStructFromLuaToCpp(context, "type", &type, "data", &data)) {
      return false;
    }
    *value = EventWrapper(type);
    value->SetValues(data);
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const EventWrapper& value) {
    detail::PushStructFromCppToLua(context, nullptr, "type", value.GetTypeId(),
                                   "data", *value.GetValues());
  }
};

namespace detail {

struct PopSerializable {
  const ConvertContext& context;
  bool ret;
  explicit PopSerializable(const ConvertContext& context)
      : context(context), ret(true) {}

  template <typename T>
  void operator()(T* ptr, HashValue key) {
    LUA_UTIL_EXPECT_STACK(context.lua, 0);
    Convert<HashValue>::PushFromCppToLua(context, key);
    lua_gettable(context.lua, -2);
    if (!Convert<T>::PopFromLuaToCpp(context, ptr)) {
      ret = false;
    }
  }

  bool IsDestructive() { return true; }
};

struct PushSerializable {
  const ConvertContext& context;
  int id;
  PushSerializable(const ConvertContext& context, int id)
      : context(context), id(id) {}

  template <typename T>
  void operator()(T* ptr, HashValue key) {
    LUA_UTIL_EXPECT_STACK(context.lua, 0);
    Convert<HashValue>::PushFromCppToLua(context, key);
    Convert<T>::PushFromCppToLua(context, *ptr);
    lua_settable(context.lua, id);
  }

  bool IsDestructive() { return false; }
};

}  // namespace detail

template <typename T>
struct Convert<T, false, false, false> {
  static const char* GetLuaTypeName() {
    static std::string type = std::string("table like ") + GetTypeName<T>();
    return type.c_str();
  }

  static inline bool PopFromLuaToCpp(const ConvertContext& context, T* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    Popper popper(context.lua);
    if (!lua_istable(context.lua, -1)) {
      return false;
    }
    lua_checkstack(context.lua, 2);
    lua_getfield(context.lua, LUA_REGISTRYINDEX, kUtilRegistryKey);
    lua_getfield(context.lua, lua_gettop(context.lua), "hash_table_keys");
    lua_remove(context.lua, -2);
    lua_pushvalue(context.lua, -2);
    lua_call(context.lua, 1, 1);
    detail::PopSerializable serializer(context);
    value->Serialize(serializer);
    lua_pop(context.lua, 1);
    return serializer.ret;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const T& value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 3);
    lua_newtable(context.lua);
    int id = lua_gettop(context.lua);
    lua_getfield(context.lua, LUA_REGISTRYINDEX, kUtilRegistryKey);
    lua_getfield(context.lua, lua_gettop(context.lua),
                 "serializable_metatable");
    lua_setmetatable(context.lua, id);
    lua_pop(context.lua, 1);
    detail::PushSerializable serializer(context, id);

    // PushSerializable doesn't actually alter the value, but it still needs a
    // non-const reference.
    const_cast<T&>(value).Serialize(serializer);
  }
};

namespace detail {

template <typename... Args>
struct LuaFunctionCallerImpl;

template <typename FirstArg, typename... Args>
struct LuaFunctionCallerImpl<FirstArg, Args...> {
  static void Call(const ConvertContext& context, int num_args, bool is_void,
                   FirstArg firstArg, Args... args) {
    using FirstArgType = typename std::decay<FirstArg>::type;
    Convert<FirstArgType>::PushFromCppToLua(context, firstArg);
    LuaFunctionCallerImpl<Args...>::Call(context, num_args, is_void, args...);
  }
};

template <>
struct LuaFunctionCallerImpl<> {
  static void Call(const ConvertContext& context, int num_args, bool is_void) {
    if (lua_pcall(context.lua, num_args, is_void ? 0 : 1, 0) != 0) {
      lua_pop(context.lua, 1);
    }
  }
};

inline void GetFunction(const ConvertContext& context, int id) {
  LUA_UTIL_EXPECT_STACK(context.lua, 1);
  lua_checkstack(context.lua, 2);
  lua_getfield(context.lua, LUA_REGISTRYINDEX, kCallbackRegistryKey);
  lua_pushinteger(context.lua, id);
  lua_gettable(context.lua, -2);
  lua_remove(context.lua, -2);
}

class FuncDeleter {
 public:
  FuncDeleter(const ConvertContext& context, int id)
      : context_(context), id_(id) {}

  ~FuncDeleter() {
    LUA_UTIL_EXPECT_STACK(context_.lua, 0);
    lua_checkstack(context_.lua, 3);
    Popper popper(context_.lua);
    lua_getfield(context_.lua, LUA_REGISTRYINDEX, kCallbackRegistryKey);
    lua_pushinteger(context_.lua, id_);
    lua_pushnil(context_.lua);
    lua_settable(context_.lua, -3);
  }

 private:
  ConvertContext context_;
  int id_;
};

template <typename Sig>
class LuaFunctionCaller;

template <typename Return, typename... Args>
class LuaFunctionCaller<Return(Args...)> {
 public:
  LuaFunctionCaller(const ConvertContext& context, int id)
      : context_(context), id_(id), deleter_(new FuncDeleter(context, id)) {}

  Return operator()(Args... args) {
    LUA_UTIL_EXPECT_STACK(context_.lua, 0);
    lua_checkstack(context_.lua, sizeof...(Args) + 1);  // Args + function.
    GetFunction(context_, id_);
    LuaFunctionCallerImpl<Args...>::Call(context_, sizeof...(Args), false,
                                         args...);
    Return ret;
    if (!Convert<Return>::PopFromLuaToCpp(context_, &ret)) {
      LOG(ERROR) << "Callback expects the return type to be "
                 << Convert<Return>::GetLuaTypeName();
    }
    return ret;
  }

 private:
  ConvertContext context_;
  int id_;
  std::shared_ptr<FuncDeleter> deleter_;
};

template <typename... Args>
class LuaFunctionCaller<void(Args...)> {
 public:
  LuaFunctionCaller(const ConvertContext& context, int id)
      : context_(context), id_(id), deleter_(new FuncDeleter(context, id)) {}

  void operator()(Args... args) {
    LUA_UTIL_EXPECT_STACK(context_.lua, 0);
    lua_checkstack(context_.lua, sizeof...(Args) + 1);  // Args + function.
    GetFunction(context_, id_);
    LuaFunctionCallerImpl<Args...>::Call(context_, sizeof...(Args), true,
                                         args...);
  }

 private:
  ConvertContext context_;
  int id_;
  std::shared_ptr<FuncDeleter> deleter_;
};

template <typename T>
struct LuaReturn;

template <typename Return, typename... Args>
struct LuaReturn<Return(Args...)> {
  constexpr static int kNumValuesOnStack = std::is_void<Return>::value ? 0 : 1;
};

using LambdaType = std::function<int(lua_State*)>;

inline int LambdaWrapper(lua_State* lua) {
  void* userdata = lua_touserdata(lua, 1);
  auto* fn = static_cast<LambdaType*>(userdata);
  // Userdata is first arg, remove it. All remaining argments follow and are
  // decremented to their expected place.
  lua_remove(lua, 1);
  const int ret = (*fn)(lua);
  if (ret == -1) {
    return lua_error(lua);
  }
  return ret;
}

inline int DeleteLambda(lua_State* lua) {
  void* userdata = lua_touserdata(lua, 1);
  auto* fn = static_cast<LambdaType*>(userdata);
  // The allocated memory of userdata itself is managed by lua, but we need to
  // destruct any other memory (mirrors the placement new as below: "new (fn)").
  fn->~LambdaType();
  return 0;
}

}  // namespace detail

template <typename FnSig>
struct Convert<std::function<FnSig>, false, false, false> {
  static const char* GetLuaTypeName() { return "function"; }

  static inline bool PopFromLuaToCpp(const ConvertContext& context,
                                     std::function<FnSig>* value) {
    LUA_UTIL_EXPECT_STACK(context.lua, -1);
    lua_checkstack(context.lua, 3);
    Popper popper(context.lua, 2);
    if (!lua_isfunction(context.lua, -1)) {
      return false;
    }

    lua_getfield(context.lua, LUA_REGISTRYINDEX, kCallbackRegistryKey);
    lua_getfield(context.lua, -1, kCallbackIdKey);
    int id = static_cast<int>(lua_tointeger(context.lua, -1));
    lua_pushvalue(context.lua, -3);
    lua_settable(context.lua, -3);
    lua_pushinteger(context.lua, id + 1);
    lua_setfield(context.lua, -2, kCallbackIdKey);
    *value = detail::LuaFunctionCaller<FnSig>(context, id);
    return true;
  }

  static inline void PushFromCppToLua(const ConvertContext& context,
                                      const std::function<FnSig>& value) {
    LUA_UTIL_EXPECT_STACK(context.lua, 1);
    lua_checkstack(context.lua, 3);
    // We use userdata instead of cclosure because lua only observes __gc for
    // userdata, and __call can mimic the behavior of functions.
    auto* fn = static_cast<detail::LambdaType*>(
        lua_newuserdata(context.lua, sizeof(detail::LambdaType)));
    new (fn) detail::LambdaType([value](lua_State* lua) {
      LuaContext context((ConvertContext(lua)));
      if (!CallNativeFunction(&context, "anonymous function", value)) {
        return -1;
      }
      return detail::LuaReturn<FnSig>::kNumValuesOnStack;
    });
    lua_newtable(context.lua);
    // These metatable functions will all be called with the userdata as the
    // first argument.
    lua_pushcclosure(context.lua, detail::LambdaWrapper, 0);
    lua_setfield(context.lua, -2, "__call");
    lua_pushcclosure(context.lua, detail::DeleteLambda, 0);
    lua_setfield(context.lua, -2, "__gc");
    lua_setmetatable(context.lua, -2);
  }
};

template <typename T>
bool LuaContext::ArgToCpp(const char* func_name, size_t arg, T* value) {
  if (!Convert<T>::PopFromLuaToCpp(context_, value)) {
    luaL_where(context_.lua, 1);
    lua_pushfstring(context_.lua, "%s expects the type of arg %d to be %s",
                    func_name, arg + 1, Convert<T>::GetLuaTypeName());
    lua_concat(context_.lua, 2);
    return false;
  }
  return true;
}

template <typename T>
bool LuaContext::ReturnFromCpp(const char* func_name, const T& value) {
  Convert<T>::PushFromCppToLua(context_, value);
  return true;
}

}  // namespace lua
}  // namespace script
}  // namespace lull

#endif  // LULLABY_MODULES_LUA_CONVERT_H_
