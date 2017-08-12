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

#ifndef LULLABY_UTIL_BUILT_IN_FUNCTIONS_H_
#define LULLABY_UTIL_BUILT_IN_FUNCTIONS_H_

#include "mathfu/glsl_mappings.h"
#include "lullaby/util/hash.h"

namespace lull {

template <typename FunctionBinder>
void RegisterBuiltInFunctions(FunctionBinder* binder) {
  binder->RegisterFunction(
      "hash", [](const std::string& str) { return Hash(str.c_str()); });
  binder->RegisterFunction("vec2",
                           [](float x, float y) { return mathfu::vec2(x, y); });
  binder->RegisterFunction(
      "vec3", [](float x, float y, float z) { return mathfu::vec3(x, y, z); });
  binder->RegisterFunction("vec4", [](float x, float y, float z, float w) {
    return mathfu::vec4(x, y, z, w);
  });
  binder->RegisterFunction("quat", [](float s, float x, float y, float z) {
    return mathfu::quat(s, x, y, z);
  });
}

}  // namespace lull

#endif  // LULLABY_UTIL_BUILT_IN_FUNCTIONS_H_
