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

#include "lullaby/modules/lua/utils.h"

#include "lua5.2/src/lauxlib.h"
#include "lullaby/modules/lua/stack_checker.h"

namespace lull {
namespace script {
namespace lua {

const char* ScriptReader(lua_State* lua, void* data, size_t* size) {
  auto* script = reinterpret_cast<ScriptReaderState*>(data);
  if (script->size == 0) {
    return nullptr;
  }
  *size = script->size;
  script->size = 0;
  return script->data;
}

void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
  if (nsize == 0) {
    free(ptr);
    return nullptr;
  }
  return realloc(ptr, nsize);
}

int LuaPrint(lua_State* lua) {
  LUA_UTIL_EXPECT_STACK(lua, 0);
  int n = lua_gettop(lua);
  lua_getglobal(lua, "tostring");
  std::string msg;
  for (int i = 1; i <= n; i++) {
    lua_pushvalue(lua, -1);
    lua_pushvalue(lua, i);
    lua_call(lua, 1, 1);
    const char* s = lua_tostring(lua, -1);
    if (s == NULL) {
      return luaL_error(lua, "tostring must return a string to print");
    }
    if (i > 1) {
      msg += "\t";
    }
    msg += s;
    lua_pop(lua, 1);
  }
  lua_pop(lua, 1);
  LOG(INFO) << msg;
  return 0;
}

}  // namespace lua
}  // namespace script
}  // namespace lull
