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

#ifndef REDUX_MODULES_MATH_CONSTANTS_H_
#define REDUX_MODULES_MATH_CONSTANTS_H_

#include <cmath>

namespace redux {

// A collection of useful mathematical constants.

static constexpr float kPi = M_PI;
static constexpr float kTwoPi = 2.f * kPi;
static constexpr float kThreePi = 3.f * kPi;
static constexpr float kHalfPi = M_PI_2;

static constexpr float kDegreesToRadians = kPi / 180.f;
static constexpr float kRadiansToDegrees = 180.f / kPi;

static constexpr float kDefaultEpsilon = 1.0e-5f;
static constexpr float kDefaultEpsilonSqr = 1.0e-10f;

// Use a smaller threshold for determinants to support matrices with 1/1000
// scales.
static constexpr float kDeterminantThreshold = 1.0e-9f;  // (1/1000)^3

// Whether or not the math library should use SIMD types by default.
static constexpr bool kEnableSimdByDefault = false;

}  // namespace redux

#endif  // REDUX_MODULES_MATH_CONSTANTS_H_
