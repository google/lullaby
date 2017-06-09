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

#ifndef LULLABY_UTIL_HASH_H_
#define LULLABY_UTIL_HASH_H_

#include <stdint.h>
#include <cstddef>

#include "lullaby/util/string_view.h"

// String hashing function used by various parts of Lullaby.  It uses the
// following algorithm:
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//
// Note: The has algorithm is implemented twice: once in Hash and once in
// ConstHash.  It is important to keep both implementations the same if a
// new algorithm is ever chosen.
namespace lull {

using HashValue = unsigned int;

constexpr HashValue kHashOffsetBasis = 0x84222325;
constexpr HashValue kHashPrimeMultiplier = 0x000001b3;

HashValue Hash(const char* str);
HashValue Hash(const char* str, size_t len);
HashValue Hash(string_view str);
HashValue HashCaseInsensitive(const char* str, size_t len);

namespace detail {

// Helper function for performing the recursion for the compile time hash.
template <std::size_t N>
inline constexpr HashValue ConstHash(const char (&str)[N], int start,
                                     HashValue hash) {
  // We need to perform the static_cast to uint64_t otherwise MSVC complains
  // about integral constant overflow (warning C4307).
  return (start == N || start == N - 1)
             ? hash
             : ConstHash(
                   str, start + 1,
                   static_cast<HashValue>(
                       (hash ^ static_cast<unsigned char>(str[start])) *
                       static_cast<uint64_t>(kHashPrimeMultiplier)));
}

}  // namespace detail

// Compile-time hash function.
template <std::size_t N>
inline constexpr HashValue ConstHash(const char (&str)[N]) {
  return N <= 1 ? 0 : detail::ConstHash(str, 0, kHashOffsetBasis);
}

// Functor for using hashable types in STL containers.
template <class T>
struct Hasher {
  std::size_t operator()(const T& value) const { return Hash(value); }
};

}  // namespace lull

#endif  // LULLABY_UTIL_HASH_H_
