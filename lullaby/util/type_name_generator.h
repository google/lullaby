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

#ifndef LULLABY_UTIL_TYPE_NAME_GENERATOR_H_
#define LULLABY_UTIL_TYPE_NAME_GENERATOR_H_

#include "lullaby/util/common_types.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/typeid.h"

namespace lull {

/// Generates a std::string for a given type |T|.  This is currently limted to:
///
/// - All types registered with Lullaby's TypeId system.
/// - Optional<T> objects, where T is one of the supported types.
/// - std::vector<T> objects, where T is one of the supported types.
/// - std::map<K, V> objects, where K and V are one of the supported types.
/// - std::unordered_map<K, V> objects, where K and V are one of the supported
///   types.
class TypeNameGenerator {
 public:
  /// Returns the name of the specified type |T|.
  template <typename T>
  static std::string Generate() {
    return GenerateImpl<T>();
  }

 private:
  template <typename T>
  using IsOptional = detail::IsOptional<T>;
  template <typename T>
  using IsVector = detail::IsVector<T>;
  template <typename T>
  using IsMap = detail::IsMap<T>;

  template <typename T>
  struct HasTypeName {
    static const bool kValue =
        !IsOptional<T>::kValue && !IsVector<T>::kValue && !IsMap<T>::kValue;
  };

  template <typename T>
  static auto GenerateImpl() ->
      typename std::enable_if<HasTypeName<T>::kValue, std::string>::type {
    return ::lull::GetTypeName<T>();
  }

  template <typename T>
  static auto GenerateImpl() ->
      typename std::enable_if<IsOptional<T>::kValue, std::string>::type {
    const std::string value = Generate<typename T::ValueType>();
    return std::string("lull::Optional<") + value + std::string(">");
  }

  template <typename T>
  static auto GenerateImpl() ->
      typename std::enable_if<IsVector<T>::kValue, std::string>::type {
    const std::string value = Generate<typename T::value_type>();
    return std::string("std::vector<") + value + std::string(">");
  }

  template <typename T>
  static auto GenerateImpl() ->
      typename std::enable_if<IsMap<T>::kValue, std::string>::type {
    const std::string map =
        IsMap<T>::kUnordered ? "std::unordered_map" : "std::map";
    const std::string key = Generate<typename T::key_type>();
    const std::string value = Generate<typename T::mapped_type>();
    return map + "<" + key + ", " + value + ">";
  }
};

}  // namespace lull

#endif  // LULLABY_UTIL_TYPE_NAME_GENERATOR_H_
