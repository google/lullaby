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

#ifndef REDUX_MODULES_BASE_HASH_H_
#define REDUX_MODULES_BASE_HASH_H_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

// String hashing function used by various parts of redux. It uses the
// FNV-1a algorithm from:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
//
// Note: The hash algorithm is implemented twice: once in Hash and once in
// ConstHash. It is important to keep both implementations the same if a new
// algorithm is ever chosen.

namespace redux {

// ValueType for storing hash results that also supports comparison operations.
class HashValue {
 public:
  // Underlying representation for HashValue.
  using Rep = std::uint32_t;

  // Constants used for hashing routines.
  static constexpr Rep kOffsetBasis = 0x84222325;
  static constexpr Rep kPrimeMultiplier = 0x000001b3;
  static constexpr Rep kGoldenRatio = 0x9e3779b9;  // 2^32 / 1.61803399

  HashValue() = default;
  constexpr explicit HashValue(Rep value) : value_(value) {}

  constexpr Rep get() const { return value_; }

  template <typename H>
  friend H AbslHashValue(H h, const HashValue& hash) {
    return H::combine(std::move(h), hash.value_);
  }

 private:
  Rep value_ = 0;
};

// Hash functions.
HashValue Hash(const char* str);
HashValue Hash(const char* str, std::size_t len);
HashValue Hash(const char* str, std::size_t len, HashValue::Rep basis);
HashValue Hash(std::string_view str);
HashValue Hash(HashValue prefix, const char* suffix);
HashValue Hash(HashValue prefix, std::string_view suffix);
HashValue Combine(HashValue lhs, HashValue rhs);

namespace detail {

// Helper function for performing the recursion for the compile time hash.
template <std::size_t N>
inline constexpr HashValue::Rep ConstHashRecursive(const char (&str)[N],
                                                   int start,
                                                   HashValue::Rep hash) {
  // We need to perform the static_cast to uint64_t otherwise MSVC complains
  // about integral constant overflow (warning C4307).
  return (start == N || start == N - 1)
             ? hash
             : ConstHashRecursive(
                   str, start + 1,
                   static_cast<HashValue::Rep>(
                       (hash ^ static_cast<unsigned char>(str[start])) *
                       static_cast<uint64_t>(HashValue::kPrimeMultiplier)));
}

template <std::size_t N>
inline constexpr HashValue::Rep ConstHash(const char (&str)[N]) {
  return N == 1
             ? 0
             : detail::ConstHashRecursive(str, 0, HashValue::kOffsetBasis);
}

}  // namespace detail

// Compile-time hash function.
template <std::size_t N>
inline constexpr HashValue ConstHash(const char (&str)[N]) {
  return HashValue(detail::ConstHash(str));
}

// Hash of an already hashed value; just return it.
inline HashValue Hash(HashValue h) { return h; }

// HashValue comparison functions.
inline bool operator==(HashValue lhs, HashValue rhs) {
  return lhs.get() == rhs.get();
}

inline bool operator!=(HashValue lhs, HashValue rhs) {
  return lhs.get() != rhs.get();
}

inline bool operator<(HashValue lhs, HashValue rhs) {
  return lhs.get() < rhs.get();
}

inline bool operator<=(HashValue lhs, HashValue rhs) {
  return lhs.get() <= rhs.get();
}

inline bool operator>(HashValue lhs, HashValue rhs) {
  return lhs.get() > rhs.get();
}

inline bool operator>=(HashValue lhs, HashValue rhs) {
  return lhs.get() >= rhs.get();
}

}  // namespace redux

#endif  // REDUX_MODULES_BASE_HASH_H_
