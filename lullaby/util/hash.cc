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

#include "lullaby/util/hash.h"
#include <ctype.h>
#include <iostream>
#include <unordered_map>

#ifdef LULLABY_GENERATED_UNHASH_TABLE
#include "lullaby/util/generated_unhash_table.h"
#endif

namespace lull {

#ifdef LULLABY_DEBUG_HASH
namespace {
std::unordered_map<HashValue, std::string>& GetUnhashTable() {
#ifdef LULLABY_GENERATED_UNHASH_TABLE
  static auto unhash_table = new std::unordered_map<HashValue, std::string>(
      LULLABY_DEBUG_HASH_INITIAL_TABLE);
#else
  static auto unhash_table = new std::unordered_map<HashValue, std::string>();
#endif
  return *unhash_table;
}
}  // namespace
#endif

HashValue Hash(const char* str) {
  const size_t npos = -1;
  return Hash(str, npos);
}

HashValue Hash(const char* str, size_t len) {
  return Hash(kHashOffsetBasis, str, len);
}

HashValue Hash(HashValue basis, const char* str, size_t len) {
  if (str == nullptr || *str == 0 || len == 0) {
    return 0;
  }

#ifdef LULLABY_DEBUG_HASH
  std::string old_str =
      len == size_t(-1) ? std::string(str) : std::string(str, len);
#endif

  // A quick good hash, from:
  // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  // Specifically, the FNV-1a function.
  size_t count = 0;
  HashValue value = basis;
  while (*str && count < len) {
    value = (value ^ static_cast<unsigned char>(*str++)) * kHashPrimeMultiplier;
    ++count;
  }

#ifdef LULLABY_DEBUG_HASH
  GetUnhashTable()[value] = old_str.substr(0, count);
#endif

  return value;
}

HashValue Hash(HashValue prefix, string_view suffix) {
  HashValue safe_prefix = prefix != 0 ? prefix : kHashOffsetBasis;
  return Hash(safe_prefix, suffix.data(), suffix.length());
}

HashValue Hash(string_view str) { return Hash(str.data(), str.length()); }

HashValue HashCaseInsensitive(const char* str, size_t len) {
  if (str == nullptr || *str == 0 || len == 0) {
    return 0;
  }

#ifdef LULLABY_DEBUG_HASH
  std::string lower_str =
      len == size_t(-1) ? std::string(str) : std::string(str, len);
  for (char& c : lower_str) {
    c = static_cast<char>(tolower(c));
  }
#endif

  size_t count = 0;
  HashValue value = kHashOffsetBasis;
  while (*str && count < len) {
    value = (value ^ static_cast<unsigned char>(tolower(*str++))) *
            kHashPrimeMultiplier;
    ++count;
  }

#ifdef LULLABY_DEBUG_HASH
  GetUnhashTable()[value] = lower_str.substr(0, count);
#endif

  return value;
}

#ifdef LULLABY_DEBUG_HASH
string_view Unhash(HashValue value) {
  auto itr = GetUnhashTable().find(value);
  if (itr != GetUnhashTable().end()) {
    return itr->second;
  }
  return string_view();
}
#endif

HashValue HashCombine(HashValue lhs, HashValue rhs) {
  // Offset by the golden ratio to avoid mapping all zeros to all zeros.
  return lhs ^ (rhs + kHashGoldenRatio + (lhs << 6) + (lhs >> 2));
}

}  // namespace lull
