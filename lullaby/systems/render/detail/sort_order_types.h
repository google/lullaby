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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_SORT_ORDER_TYPE_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_SORT_ORDER_TYPE_H_

#include <inttypes.h>
#include <string.h>
#include <string>
#include "lullaby/util/logging.h"

// By default when calculating the sort order, we store the root component IDs
// in the top 4 bits, and each successive level in additional 4 bit blocks.
// Each level is limited to 16 components, so we wrap to prevent overflowing
// into another depth's bits.

#ifndef SORT_ORDER_SIZE
#define SORT_ORDER_SIZE 128
#endif

#ifndef SORT_ORDER_OFFSET_SIZE
#define SORT_ORDER_OFFSET_SIZE 4
#endif

#if SORT_ORDER_OFFSET_SIZE > 32
#error Sort order offset cannot be more than 32 bits
#endif

#define BITS_PER_INT 32

namespace lull {

using RenderSortOrderOffset = int;

// A global render sort order for a given entity relative to all other entities.
// TODO: Add tests for different sizes.
// TODO: Hide implementation details such as left shift operation.
class RenderSortOrder {
 public:
  constexpr static int kNumBitsPerGroup = SORT_ORDER_OFFSET_SIZE;
  constexpr static RenderSortOrderOffset kMaxOffset = 1 << kNumBitsPerGroup;
  constexpr static int kMaxDepth = SORT_ORDER_SIZE / kNumBitsPerGroup;
  constexpr static int kRootShift = SORT_ORDER_SIZE - kNumBitsPerGroup;

  RenderSortOrder() {
    memset(&value_, 0, sizeof(value_));
  }

  RenderSortOrder(uint64_t v) {
    if (kIntSize == 1) {
      LOG(WARNING) << "Render sort order overflow.";
    }
    Init(v);
  }

  std::string ToHexString() const {
    std::string result = "0x";
    if (kIntSize == 1) {
      char buf[9];
      snprintf(buf, sizeof(buf), "%08" PRIX32, value_.u32);
      result += buf;
    } else if (kIntSize == 2) {
      char buf[17];
      snprintf(buf, sizeof(buf), "%016" PRIX64, value_.u64);
      result += buf;
    } else {
      char buf[9];
      int index = 0;
      while (index < kIntSize) {
        snprintf(buf, sizeof(buf), "%08" PRIX32, value_.u32s_[index]);
        result += buf;
        ++index;
      }
    }
    return result;
  }

  RenderSortOrder& operator=(const int& v) {
    if (kIntSize == 1 && sizeof(int) > 4) {
      LOG(WARNING) << "Render sort order overflow.";
    }
    Init(static_cast<uint64_t>(v));
    return *this;
  }

  RenderSortOrder& operator=(const uint32_t& v) {
    Init(static_cast<uint64_t>(v));
    return *this;
  }

  RenderSortOrder& operator=(const uint64_t& v) {
    Init(v);
    return *this;
  }

  RenderSortOrder& operator+=(const RenderSortOrder& rhs) {
    *this = *this + rhs;
    return *this;
  }

  RenderSortOrder& operator+(const RenderSortOrder& rhs) {
    if (kIntSize == 1) {
      value_.u32 += rhs.value_.u32;
    } else if (kIntSize == 2) {
      value_.u64 += rhs.value_.u64;
    } else {
      int index = kIntSize - 1;
      uint64_t carry_over = 0;
      while (index >= 0) {
        uint64_t sum =
            rhs.value_.u32s_[index] + value_.u32s_[index] + carry_over;
        carry_over = sum >> BITS_PER_INT;
        value_.u32s_[index] = sum & 0xFFFFFFFF;
        --index;
      }
      if (carry_over > 0) {
        LOG(WARNING) << "Render sort order addition overflow.";
        memset(&value_, 0xFF, sizeof(value_));
      }
    }
    return *this;
  }

  RenderSortOrder& operator-=(const RenderSortOrder& rhs) {
    *this = *this - rhs;
    return *this;
  }

  RenderSortOrder& operator-(const RenderSortOrder& rhs) {
    if (kIntSize == 1) {
      value_.u32 -= rhs.value_.u32;
    } else if (kIntSize == 2) {
      value_.u64 -= rhs.value_.u64;
    } else {
      int index = kIntSize - 1;
      uint64_t borrow = 0;
      while (index >= 0) {
        int64_t sub = value_.u32s_[index] - borrow - rhs.value_.u32s_[index];
        if (sub < 0) {
          borrow = 1;
          sub += static_cast<int64_t>(borrow << BITS_PER_INT);
        } else {
          borrow = 0;
        }
        value_.u32s_[index] = static_cast<uint32_t>(sub);
        --index;
      }
      if (borrow > 0) {
        LOG(WARNING) << "Render sort order subtraction overflow.";
        memset(&value_, 0, sizeof(value_));
      }
    }
    return *this;
  }

  RenderSortOrder& operator<<(const int& shift) {
    if (kIntSize == 1) {
      value_.u32 <<= shift;
    } else if (kIntSize == 2) {
      value_.u64 <<= shift;
    } else {
      int index_shift = shift / BITS_PER_INT;
      int bits_shift = shift % BITS_PER_INT;
      uint64_t u64 = 0;
      int index = 0;
      while (index < kIntSize) {
        if (value_.u32s_[index] != 0) {
          u64 = value_.u32s_[index];
          value_.u32s_[index] = 0;
          int target_index = index - index_shift;
          if (target_index >= 0) {
            u64 <<= bits_shift;
            uint32_t u32_high = static_cast<uint32_t>(u64 >> BITS_PER_INT);
            uint32_t u32_low = u64 & 0xFFFFFFFF;
            if (target_index > 0) {
              value_.u32s_[target_index - 1] |= u32_high;
            }
            value_.u32s_[target_index] = u32_low;
          }
        }
        ++index;
      }
    }
    return *this;
  }

  bool operator<(const RenderSortOrder& rhs) const {
    if (kIntSize == 1) {
      return value_.u32 < rhs.value_.u32;
    }
    if (kIntSize == 2) {
      return value_.u64 < rhs.value_.u64;
    }
    int index = FindFirstDifference(rhs);
    return index == kIntSize ? false
                             : value_.u32s_[index] < rhs.value_.u32s_[index];
  }

  bool operator<=(const RenderSortOrder& rhs) const {
    if (kIntSize == 1) {
      return value_.u32 <= rhs.value_.u32;
    }
    if (kIntSize == 2) {
      return value_.u64 <= rhs.value_.u64;
    }
    int index = FindFirstDifference(rhs);
    return index == kIntSize ? true
                             : value_.u32s_[index] < rhs.value_.u32s_[index];
  }

  bool operator>(const RenderSortOrder& rhs) const {
    if (kIntSize == 1) {
      return value_.u32 > rhs.value_.u32;
    }
    if (kIntSize == 2) {
      return value_.u64 > rhs.value_.u64;
    }
    int index = FindFirstDifference(rhs);
    return index == kIntSize ? false
                             : value_.u32s_[index] > rhs.value_.u32s_[index];
  }

  bool operator>=(const RenderSortOrder& rhs) const {
    if (kIntSize == 1) {
      return value_.u32 >= rhs.value_.u32;
    }
    if (kIntSize == 2) {
      return value_.u64 >= rhs.value_.u64;
    }
    int index = FindFirstDifference(rhs);
    return index == kIntSize ? true
                             : value_.u32s_[index] > rhs.value_.u32s_[index];
  }

  bool operator==(const RenderSortOrder& rhs) const {
    if (kIntSize == 1) {
      return value_.u32 == rhs.value_.u32;
    }
    if (kIntSize == 2) {
      return value_.u64 == rhs.value_.u64;
    }
    int index = FindFirstDifference(rhs);
    return index == kIntSize;
  }

 private:
  constexpr static int kIntSize = SORT_ORDER_SIZE / BITS_PER_INT;
  union ValueType {
    uint32_t u32s_[kIntSize];
    uint64_t u64;
    uint32_t u32;
  };
  ValueType value_;

  void Init(uint64_t v) {
    if (kIntSize == 1) {
      value_.u32 = static_cast<uint32_t>(v);
    } else if (kIntSize == 2) {
      value_.u64 = v;
    } else {
      memset(&value_, 0, sizeof(value_));
      value_.u32s_[kIntSize - 1] = static_cast<uint32_t>(v & 0xFFFFFFFF);
      value_.u32s_[kIntSize - 2] = static_cast<uint32_t>(v >> BITS_PER_INT);
    }
  }

  int FindFirstDifference(const RenderSortOrder& rhs) const {
    int index = 0;
    while (index < kIntSize) {
      if (value_.u32s_[index] != rhs.value_.u32s_[index]) {
        break;
      }
      ++index;
    }
    return index;
  }
};

};  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_SORT_ORDER_TYPE_H_
