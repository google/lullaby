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

#ifndef LULLABY_MODULES_SERIALIZE_SERIALIZE_TRAITS_H_
#define LULLABY_MODULES_SERIALIZE_SERIALIZE_TRAITS_H_

#include <type_traits>

#include "lullaby/util/clock.h"
#include "lullaby/util/hash.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace detail {

// This trait is similar to std::is_fundamental, but also includes mathfu types.
template <typename T>
struct IsSerializeFundamental {
  using U = typename std::remove_cv<T>::type;
  static const bool kValue =
      std::is_enum<U>::value || std::is_same<U, bool>::value ||
      std::is_same<U, int8_t>::value || std::is_same<U, uint8_t>::value ||
      std::is_same<U, int16_t>::value || std::is_same<U, uint16_t>::value ||
      std::is_same<U, int32_t>::value || std::is_same<U, uint32_t>::value ||
      std::is_same<U, int64_t>::value || std::is_same<U, uint64_t>::value ||
      std::is_same<U, float>::value || std::is_same<U, double>::value ||
      std::is_same<U, Clock::duration>::value ||
      std::is_same<U, mathfu::vec2>::value ||
      std::is_same<U, mathfu::vec2i>::value ||
      std::is_same<U, mathfu::vec3>::value ||
      std::is_same<U, mathfu::vec3i>::value ||
      std::is_same<U, mathfu::vec4>::value ||
      std::is_same<U, mathfu::vec4i>::value ||
      std::is_same<U, mathfu::quat>::value ||
      std::is_same<U, mathfu::mat4>::value;
};

// Determines if |T| has a member function with the signature:
// void T::Serialize(Archive).
template <typename T, typename Archive>
class IsSerializable {
  // If type |U| has a member function with the signature
  // |void U::Serialize(Archive)|, then this function will be defined such
  // that it returns std::true_type.  If type |U| has no such function, SFINAE
  // will kick in and this function will not be defined. (See
  // https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error for more
  // information.)
  template <typename U>
  static typename std::is_same<void, decltype(std::declval<U>().Serialize(
                                         std::declval<Archive>()))>::type
  Helper(U*);

  // The fallback Helper function definition that returns std::false_type.
  // This function will be instantiated if the above function fails to be a
  // candidate for overload resolution.
  template <typename U>
  static std::false_type Helper(...);

 public:
  // The boolean value exposed by this class is the same as the value of the
  // return type of the static Helper function.
  static const bool kValue = decltype(Helper<T>(nullptr))::value;
};

// Determines if |Serializer| has a member function with the signature:
// void Serializer::Begin(HashValue);
template <typename Serializer>
class IsScopedSerializer {
  // If type |U| has a member function with the signature
  // |void U::Begin(HashValue)|, then this function will be defined such
  // that it returns std::true_type.  If type |U| has no such function, SFINAE
  // will kick in and this function will not be defined. (See
  // https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error for more
  // information.)
  template <typename U>
  static typename std::is_same<
      void, decltype(std::declval<U>().Begin(std::declval<HashValue>()))>::type
  Helper(U*);

  // The fallback Helper function definition that returns std::false_type.
  // This function will be instantiated if the above function fails to be a
  // candidate for overload resolution.
  template <typename U>
  static std::false_type Helper(...);

 public:
  // The boolean value exposed by this class is the same as the value of the
  // return type of the static Helper function.
  static const bool kValue = decltype(Helper<Serializer>(nullptr))::value;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_MODULES_SERIALIZE_SERIALIZE_TRAITS_H_
