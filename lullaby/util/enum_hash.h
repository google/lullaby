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

#ifndef LULLABY_UTIL_ENUM_HASH_H_
#define LULLABY_UTIL_ENUM_HASH_H_

#include <stddef.h>
#include <type_traits>

namespace lull {

// Useful when using an enum as a key in an unordered_container.
//
// This is necessary for some older compilers and is harmless otherwise.
struct EnumHash {
  template <typename T>
  typename std::enable_if<std::is_enum<T>::value, size_t>::type
  operator()(T t) const {
    return static_cast<size_t>(t);
  }
};

}  // namespace lull

#endif  // LULLABY_UTIL_ENUM_HASH_H_
