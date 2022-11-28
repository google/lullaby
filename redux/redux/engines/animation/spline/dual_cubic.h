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

#ifndef REDUX_ENGINES_ANIMATION_SPLINE_DUAL_CUBIC_H_
#define REDUX_ENGINES_ANIMATION_SPLINE_DUAL_CUBIC_H_

#include "redux/engines/animation/spline/cubic_curve.h"

namespace redux {

// Finds a point (x, y) and its derivative in between init.start_x and
// init.end_x such that the two cubic functions look smoother than the one cubic
// function created by init.
//
// Please see docs/dual_cubic.pdf for the math.
void CalculateDualCubicMidNode(const CubicInit& init, float* x, float* y,
                               float* derivative);

}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_SPLINE_DUAL_CUBIC_H_
