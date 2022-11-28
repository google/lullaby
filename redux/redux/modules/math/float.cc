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

#include "redux/modules/math/float.h"

#include <limits>

namespace redux {

// If this assert fails, you cannot use float.h with your compiler.
// See: https://en.wikipedia.org/wiki/Single-precision_floating-point_format
static_assert(std::numeric_limits<float>::is_iec559,
              "This code assumes float is the IEEE 32-bit type.");

// externs in float.h.
const float kMinInvertablePowerOf2 = ExponentFromInt(kMinInvertableExponent);
const float kMaxInvertablePowerOf2 = ExponentFromInt(kMaxInvertableExponent);

}  // namespace redux
