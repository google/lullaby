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

#ifndef LULLABY_SYSTEMS_TEXT_DETAIL_UTIL_H_
#define LULLABY_SYSTEMS_TEXT_DETAIL_UTIL_H_

#include "mathfu/glsl_mappings.h"

namespace lull {

inline mathfu::vec4 CalcSdfParams(float edge_softness, float sdf_dist_offset,
                                  float sdf_dist_scale) {
  // Softness defines how wide the gradient at the edge of glyphs are drawn.
  // Default value chosen by UX.
  const float default_softness = 32.0f / 255.0f;

  const float threshold = .5f;
  const float softness =
      (edge_softness >= 0) ? edge_softness : default_softness;

  const float sdf_min = mathfu::Clamp(threshold - .5f * softness, 0.0f, 1.0f);
  const float sdf_max =
      mathfu::Clamp(threshold + .5f * softness, sdf_min, 1.0f) + .001f;

  return mathfu::vec4(sdf_dist_offset, sdf_dist_scale, sdf_min, sdf_max);
}

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_DETAIL_UTIL_H_
