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

#ifndef LULLABY_MODULES_LUA_STACK_CHECKER_H_
#define LULLABY_MODULES_LUA_STACK_CHECKER_H_

#include "lua5.2/src/lualib.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace script {
namespace lua {

#ifndef NDEBUG

#define LUA_UTIL_EXPECT_STACK(lua, n) \
    StackChecker __stack_checker__(lua, n, __LINE__, __FILE__);

class StackChecker {
 public:
  StackChecker(lua_State* lua, int n, int line, const char* file)
      : lua_(lua), n_(n), line_(line), file_(file) {
    before_ = lua_gettop(lua);
  }

  ~StackChecker() {
    int delta = lua_gettop(lua_) - before_;
    CHECK(delta == n_) << "Lua stack checker failed on line " << line_ <<
        " of " << file_ << ": Expected stack to change by " << n_ <<
        " but it changed by " << delta;
  }

 private:
  lua_State* lua_;
  int n_;
  int line_;
  const char* file_;
  int before_;
};

#else  // NDEBUG
#define LUA_UTIL_EXPECT_STACK(lua, n)
#endif  // NDEBUG

}  // namespace lua
}  // namespace script
}  // namespace lull

#endif  // LULLABY_MODULES_LUA_STACK_CHECKER_H_
