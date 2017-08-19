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

#ifndef LULLABY_UTIL_DETAIL_TYPE_UTIL_H_
#define LULLABY_UTIL_DETAIL_TYPE_UTIL_H_

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/util/optional.h"

namespace lull {

class EventWrapper;

namespace detail {

template <typename T>
struct IsString {
  static constexpr bool kValue = false;
};

template <>
struct IsString<std::string> {
  static constexpr bool kValue = true;
};

template <typename T>
struct IsEventWrapper {
  static constexpr bool kValue = false;
};

template <>
struct IsEventWrapper<EventWrapper> {
  static constexpr bool kValue = true;
};

template <typename T>
struct IsVector {
  static constexpr bool kValue = false;
};

template <typename T>
struct IsVector<std::vector<T>> {
  static constexpr bool kValue = true;
};

template <typename T>
struct IsMap {
  static constexpr bool kValue = false;
  static constexpr bool kUnordered = false;
};

template <typename K, typename V>
struct IsMap<std::map<K, V>> {
  static constexpr bool kValue = true;
  static constexpr bool kUnordered = false;
};

template <typename K, typename V>
struct IsMap<std::unordered_map<K, V>> {
  static constexpr bool kValue = true;
  static constexpr bool kUnordered = true;
};

template <typename T>
struct IsOptional {
  static constexpr bool kValue = false;
};

template <typename T>
struct IsOptional<Optional<T>> {
  static constexpr bool kValue = true;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_UTIL_DETAIL_TYPE_UTIL_H_
