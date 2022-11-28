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

#include "redux/modules/base/hash.h"

#include <cstddef>
#include <string_view>

namespace redux {

HashValue Hash(const char* str) {
  const std::size_t npos = -1;
  return Hash(str, npos, HashValue::kOffsetBasis);
}

HashValue Hash(const char* str, std::size_t len) {
  return Hash(str, len, HashValue::kOffsetBasis);
}

HashValue Hash(const char* str, std::size_t len, HashValue::Rep basis) {
  if (str == nullptr || *str == 0 || len == 0) {
    return HashValue(0);
  }

  std::size_t count = 0;
  HashValue::Rep value = basis;
  while (count < len && *str) {
    value = (value ^ static_cast<unsigned char>(*str++)) *
            HashValue::kPrimeMultiplier;
    ++count;
  }
  return HashValue(value);
}

HashValue Hash(std::string_view str) {
  return Hash(str.data(), str.length(), HashValue::kOffsetBasis);
}

HashValue Hash(HashValue prefix, const char* suffix) {
  if (suffix == nullptr || *suffix == 0) {
    return prefix;
  }
  const std::size_t npos = -1;
  HashValue::Rep safe_prefix =
      prefix.get() != 0 ? prefix.get() : HashValue::kOffsetBasis;
  return Hash(suffix, npos, safe_prefix);
}

HashValue Hash(HashValue prefix, std::string_view suffix) {
  if (suffix.length() == 0) {
    return prefix;
  }
  HashValue::Rep safe_prefix =
      prefix.get() != 0 ? prefix.get() : HashValue::kOffsetBasis;
  return Hash(suffix.data(), suffix.length(), safe_prefix);
}

HashValue Combine(HashValue lhs, HashValue rhs) {
  // Offset by the golden ratio to avoid mapping all zeros to all zeros.
  return HashValue(lhs.get() ^ (rhs.get() + HashValue::kGoldenRatio +
                                (lhs.get() << 6) + (lhs.get() >> 2)));
}

}  // namespace redux
