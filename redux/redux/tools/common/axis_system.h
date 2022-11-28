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

#ifndef REDUX_TOOLS_COMMON_AXIS_SYSTEM_
#define REDUX_TOOLS_COMMON_AXIS_SYSTEM_

#include "redux/tools/common/axis_system_generated.h"

namespace redux::tool {

// Parses a command line argument string into an AxisSystem value.
inline AxisSystem ReadAxisSystem(std::string_view in) {
  static const char* kAxisSystemNames[] = {
      "x+y+z",  // XUp_YFront_ZLeft
      "x+y-z",  // XUp_YFront_ZRight
      "x-y+z",  // XUp_YBack_ZLeft
      "x-y-z",  // XUp_YBack_ZRight
      "x+z+y",  // XUp_ZFront_YLeft
      "x+z-y",  // XUp_ZFront_YRight
      "x-z+y",  // XUp_ZBack_YLeft
      "x-z-y",  // XUp_ZBack_YRight
      "y+x+z",  // YUp_XFront_ZLeft
      "y+x-z",  // YUp_XFront_ZRight
      "y-x+z",  // YUp_XBack_ZLeft
      "y-x-z",  // YUp_XBack_ZRight
      "y+z+x",  // YUp_ZFront_XLeft
      "y+z-x",  // YUp_ZFront_XRight
      "y-z+x",  // YUp_ZBack_XLeft
      "y-z-x",  // YUp_ZBack_XRight
      "z+x+y",  // ZUp_XFront_YLeft
      "z+x-y",  // ZUp_XFront_YRight
      "z-x+y",  // ZUp_XBack_YLeft
      "z-x-y",  // ZUp_XBack_YRight
      "z+y+x",  // ZUp_YFront_XLeft
      "z+y-x",  // ZUp_YFront_XRight
      "z-y+x",  // ZUp_YBack_XLeft
      "z-y-x",  // ZUp_YBack_XRight
      nullptr,
  };

  constexpr int kMinAxisSystemValue = static_cast<int>(AxisSystem::MIN);
  constexpr int kMaxAxisSystemValue = static_cast<int>(AxisSystem::MAX);
  constexpr int kNumAxisSystemValues =
      kMaxAxisSystemValue - kMinAxisSystemValue + 1;
  constexpr int kNumAxisSystemNames =
      sizeof(kAxisSystemNames) / sizeof(kAxisSystemNames[0]);
  static_assert(kNumAxisSystemNames == kNumAxisSystemValues,
                "kAxisSystemNames out of sync with flatbuffer.");

  size_t i = 0;
  for (const char* const* iter = kAxisSystemNames; *iter != nullptr; ++iter) {
    if (in.compare(*iter) == 0) {
      return static_cast<AxisSystem>(i);
    }
    ++i;
  }
  return AxisSystem::Unspecified;
}

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_AXIS_SYSTEM_
