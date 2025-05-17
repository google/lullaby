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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_TYPE_ID_H_
#define REDUX_TOOLS_SCENE_PIPELINE_TYPE_ID_H_

#include <cstdint>
#include <type_traits>

namespace redux::tool {

// A simple type identification system. Usage is:
//   TypeId type_id = GetTypeId<int>();
//   const bool is_int = Is<int>(type_id);

using TypeId = std::intptr_t;

static constexpr TypeId kInvalidTypeId = 0;

namespace detail {
template <typename T>
TypeId GetTypeIdHelper() {
  static bool dummy = false;
  return reinterpret_cast<TypeId>(&dummy);
}
}  // namespace detail

template <typename T>
TypeId GetTypeId() {
  return detail::GetTypeIdHelper<std::decay_t<T>>();
}

template <typename T>
bool Is(TypeId type_id) {
  return type_id == GetTypeId<T>();
}

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_TYPE_ID_H_
