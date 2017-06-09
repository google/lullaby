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

#ifndef LULLABY_UTIL_ARC_H_
#define LULLABY_UTIL_ARC_H_

#include "lullaby/util/math.h"

namespace lull {

struct Arc {
  // The angle (in radians) in which the start of the arc should be poised at.
  // 0 = [0,1]. PI = [0,-1], PI/2 = [1,0].
  float start_angle = 0.0f;

  // Size of the arc measured in radians. PI = half circle, 2 PI = full circle.
  float angle_size = static_cast<float>(M_PI * 2.0f);

  // Inner radius of the arc.
  float inner_radius = 0.9f;

  // Outer radius of the arc.
  float outer_radius = 1.0f;

  // Number of samples used for drawing the arc.
  int num_samples = 32;
};

}  // namespace lull

#endif  // LULLABY_UTIL_ARC_H_
