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

#ifndef LULLABY_MODULES_LUA_UTILS_H_
#define LULLABY_MODULES_LUA_UTILS_H_

#include <string>

#include "lua5.2/src/lualib.h"

namespace lull {
namespace script {
namespace lua {

struct ScriptReaderState {
  explicit ScriptReaderState(const std::string& script)
      : data(script.c_str()), size(script.size()) {}
  const char* data;
  size_t size;
};

const char* ScriptReader(lua_State* lua, void* data, size_t* size);
void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize);
int LuaPrint(lua_State* lua);
int LambdaWrapper(lua_State* lua);

}  // namespace lua
}  // namespace script
}  // namespace lull

#endif  // LULLABY_MODULES_LUA_UTILS_H_
