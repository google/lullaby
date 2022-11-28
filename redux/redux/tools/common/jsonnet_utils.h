/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_TOOLS_COMMON_JSONNET_UTILS_
#define REDUX_TOOLS_COMMON_JSONNET_UTILS_

#include <functional>
#include <string>

#include "absl/container/flat_hash_map.h"

namespace redux::tool {

using JsonnetVarMap = absl::flat_hash_map<std::string, std::string>;

// Converts a jsonnet string into a json string.
std::string JsonnetToJson(const char* jsonnet, const char* filename,
                          const JsonnetVarMap& ext_vars = {});

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_JSONNET_UTILS_
