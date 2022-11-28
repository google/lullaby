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

#ifndef REDUX_TOOLS_ANIM_PIPELINE_TOLERANCES_H_
#define REDUX_TOOLS_ANIM_PIPELINE_TOLERANCES_H_

#include "redux/engines/animation/common.h"

namespace redux::tool {

// Amounts by which output curves are allowed to deviate from input curves.
struct Tolerances {
  // Amount output translate curves can deviate, in scene's distance units.
  float translate = 0.01f;

  // Amount output quaternion curves can deviate, unitless.
  float quaternion = 0.0005f;

  // Amount output scale curves can deviate, unitless.
  float scale = 0.005f;

  // Amount derivative, converted to an angle in x/y, can deviate, in radians.
  float derivative_angle = 0.00873f;  // 0.5 degrees

  // Amount derivative, converted to an angle in x/y, can deviate, in radians,
  // This value used when determining if an animation repeats or not.
  float repeat_derivative_angle = 0.1745f;  // 10 degrees
};

// Returns the tolerance value for a given operation.
inline float ToleranceForOp(const Tolerances& tolerances, AnimChannelType op) {
  if (IsTranslationChannel(op)) {
    return tolerances.translate;
  } else if (IsScaleChannel(op)) {
    return tolerances.scale;
  } else if (IsQuaternionChannel(op)) {
    return tolerances.quaternion;
  } else {
    return 0.0f;
  }
}

}  // namespace redux::tool

#endif  // REDUX_TOOLS_ANIM_PIPELINE_TOLERANCES_H_
