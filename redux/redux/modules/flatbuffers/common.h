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

#ifndef REDUX_MODULES_FLATBUFFERS_COMMON_H_
#define REDUX_MODULES_FLATBUFFERS_COMMON_H_

#include <string>
#include <string_view>

#include "redux/modules/base/hash.h"
#include "redux/modules/flatbuffers/common_generated.h"

namespace redux {

// Converts a flatbuffer HashVal into a HashValue.
inline HashValue ReadFbs(const fbs::HashVal& value) {
  return HashValue(value.value());
}

// Converts a flatbuffer String into a std::string.
inline std::string ReadFbs(const flatbuffers::String& value) {
  return value.str();
}

// Converts a flatbuffer HashString into a HashValue.
inline HashValue ReadFbs(const fbs::HashString& value) {
  return HashValue(value.hash());
}

// Creates a flatbuffer HashVal from a string.
inline fbs::HashVal CreateHashVal(std::string_view str) {
  return fbs::HashVal(Hash(str).get());
}

// Creates a flatbuffer HashString from a string.
inline fbs::HashStringT CreateHashStringT(std::string_view str) {
  fbs::HashStringT res;
  res.hash = Hash(str).get();
  res.name = std::string(str);
  return res;
}

// Simple pass-through overloads of the above functions for some fundamental
// types, for a uniform API for use in generic code.
inline bool ReadFbs(bool value) { return value; }
inline int ReadFbs(int value) { return value; }
inline float ReadFbs(float value) { return value; }

// Converts the above "direct" conversion functions (which return the results)
// into a more generic "indirect" conversion function (which takes the result
// as an out param), for use in generic code.
//
// With flatbuffers, fields can sometimes be null when they are not present in a
// table. As such, this function will be a no-op when `in` is null. The `out`
// param must be non-null.
template <typename T, typename U>
inline void TryReadFbs(const T* in, U* out) {
  CHECK(out != nullptr) << "Out param is required.";
  if (in != nullptr) {
    *out = ReadFbs(*in);
  }
}

}  // namespace redux

#endif  // REDUX_MODULES_FLATBUFFERS_COMMON_H_
