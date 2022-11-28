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

#ifndef REDUX_MODULES_BASE_BITS_H_
#define REDUX_MODULES_BASE_BITS_H_

#include <cstdint>
#include <type_traits>

namespace redux {

// Provides storage for and manipulation of individual bits.
//
// Uses T as the underlying storage for the bits. Requires that T will be an
// unsigned integral type (e.g. uint8_t, uint16_t, uint32_t, or uint64_t).
template <typename T>
class Bits {
 public:
  static_assert(std::is_unsigned_v<T>);
  using UnderlyingType = T;

  // Bits with no bits set.
  Bits() : value_(0) {}

  // Bits with the given bits set.
  constexpr explicit Bits(T bits) : value_(bits) {}

  // A Bits instance with no bits set.
  static constexpr Bits None() { return Bits(0); }

  // A Bits instance with all bits set.
  static constexpr Bits All() { return Bits(-1); }

  // A Bits instance with just the N-th bit set.
  template <unsigned int N>
  static constexpr Bits Nth() {
    return Bits(T{1} << N);
  }

  // Clears all the bits.
  constexpr void Clear() { value_ = 0; }

  // Flips all the bits (i.e. 0s becomes 1, and 1s become 0s.)
  constexpr void Flip() { value_ = ~value_; }

  // Returns whether no bits are currently set.
  constexpr bool Empty() const { return value_ == 0; }

  // Returns whether any bits are set.
  constexpr bool Any() const { return value_ != 0; }

  // Returns whether all bits are set.
  constexpr bool Full() const { return value_ == T(-1); }

  // Sets the given bits.
  constexpr void Set(Bits other) { Set(other.value_); }
  constexpr void Set(T bits) { value_ = (value_ | bits); }

  // Clears the given bits.
  constexpr void Clear(Bits other) { Clear(other.value_); }
  constexpr void Clear(T bits) { value_ = (value_ & ~bits); }

  // Unions the currently set bits with the given bits.
  constexpr void Intersect(Bits other) { Intersect(other.value_); }
  constexpr void Intersect(T bits) { value_ = (value_ & bits); }

  // Returns whether any of the specified bits are set.
  constexpr bool Any(Bits other) const { return Any(other.value_); }
  constexpr bool Any(T bits) const { return (value_ & bits) != 0; }

  // Returns true if none of the specified bits are set.
  constexpr bool None(Bits other) const { return None(other.value_); }
  constexpr bool None(T bits) const { return !Any(bits); }

  // Returns true if exactly the specified bits are set.
  constexpr bool Exactly(Bits other) const { return Exactly(other.value_); }
  constexpr bool Exactly(T bits) const { return value_ == bits; }

  // Compares the bits exactly.
  constexpr bool operator==(Bits other) const { return Exactly(other); }
  constexpr bool operator!=(Bits other) const { return !Exactly(other); }

  // Returns the underlying integral representation of the bits.
  constexpr T Value() const { return value_; }

 private:
  T value_;
};

using Bits8 = Bits<std::uint8_t>;
using Bits16 = Bits<std::uint16_t>;
using Bits32 = Bits<std::uint32_t>;
using Bits64 = Bits<std::uint64_t>;

// Useful bit manipulation functions for use with APIs that use integral values
// directly.

template <typename T>
[[nodiscard]] inline T SetBits(T in, T bits) {
  return in | bits;
}

template <typename T>
[[nodiscard]] inline T ClearBits(T in, T bits) {
  return (in & ~bits);
}

template <typename T>
[[nodiscard]] inline bool CheckBits(T in, T bits) {
  return (in & bits) != 0;
}

}  // namespace redux

#endif  // REDUX_MODULES_BASE_BITS_H_
