/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_GVR_MATHFU_GVR_CONVERSIONS_H_
#define LULLABY_MODULES_GVR_MATHFU_GVR_CONVERSIONS_H_

#include "mathfu/glsl_mappings.h"
#include "vr/gvr/capi/include/gvr_types.h"

namespace lull {

// Functions to convert between mathfu and GVR common types.

inline gvr::Mat4f GvrMatFromMathfu(const mathfu::mat4& m) {
  gvr::Mat4f output;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      output.m[r][c] = m(r, c);
    }
  }
  return output;
}

inline mathfu::mat4 MathfuMatFromGvr(const gvr::Mat4f& m) {
  mathfu::mat4 output;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      output(r, c) = m.m[r][c];
    }
  }
  return output;
}

inline gvr::Quatf GvrQuatFromMathfu(const mathfu::quat& q) {
  gvr::Quatf output;
  output.qx = q[1];
  output.qy = q[2];
  output.qz = q[3];
  output.qw = q[0];
  return output;
}

}  // namespace lull

#endif  // LULLABY_MODULES_GVR_MATHFU_GVR_CONVERSIONS_H_
