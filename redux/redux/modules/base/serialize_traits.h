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

#ifndef REDUX_MODULES_BASE_SERIALIZE_TRAITS_H_
#define REDUX_MODULES_BASE_SERIALIZE_TRAITS_H_

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "absl/container/flat_hash_map.h"

// A collection of type traits to help identify some non-primitive types that we
// want to support for serialization.

namespace redux {

// IsPointer<T>: is T a pointer
template <typename T>
using IsPointerT = std::is_pointer<T>;
template <typename T>
inline constexpr bool IsPointerV = IsPointerT<T>::value;

// IsUniquePtr<T>: is T an std::unique_ptr.
template <typename T>
struct IsUniquePtrT : std::false_type {};
template <typename U, typename D>
struct IsUniquePtrT<std::unique_ptr<U, D>> : std::true_type {};
template <typename T>
inline constexpr bool IsUniquePtrV = IsUniquePtrT<T>::value;

// IsOptional<T>: is T an std::optional.
template <typename T>
struct IsOptionalT : std::false_type {};
template <typename U>
struct IsOptionalT<std::optional<U>> : std::true_type {};
template <typename T>
inline constexpr bool IsOptionalV = IsOptionalT<T>::value;

// IsVector<T>: is T an std::vector.
template <typename T>
struct IsVectorT : std::false_type {};
template <typename U, typename A>
struct IsVectorT<std::vector<U, A>> : std::true_type {};
template <typename T>
inline constexpr bool IsVectorV = IsVectorT<T>::value;

// IsSpan<T>: is T an absl::Span.
template <typename T>
struct IsSpanT : std::false_type {};
template <typename U>
struct IsSpanT<absl::Span<U>> : std::true_type {};
template <typename T>
inline constexpr bool IsSpanV = IsSpanT<T>::value;

// IsMap<T>: is T an absl::flat_hash_map.
template <typename T>
struct IsMapT : std::false_type {};
template <typename K, typename V, typename H>
struct IsMapT<absl::flat_hash_map<K, V, H>> : std::true_type {};
template <typename T>
inline constexpr bool IsMapV = IsMapT<T>::value;

}  // namespace redux

#endif  // REDUX_MODULES_BASE_SERIALIZE_TRAITS_H_
