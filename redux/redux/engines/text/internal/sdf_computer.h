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

#ifndef REDUX_ENGINES_TEXT_INTERNAL_SDF_COMPUTER_H_
#define REDUX_ENGINES_TEXT_INTERNAL_SDF_COMPUTER_H_

#include <memory>

#include "redux/modules/graphics/image_data.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Computes an image containing signed distances for font rendering.
class SdfComputer {
 public:
  SdfComputer();
  ~SdfComputer();

  // Computes the signed distance field image for the given input grayscale
  // bitmap. Each returned pixel's value (signed distance) is the distance from
  // the center of that pixel to the nearest boundary/edge, signed so that
  // pixels inside the boundary are negative and those outside the boundary are
  // positive.
  ImageData Compute(const std::byte* bytes, const vec2i& size, int padding);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_TEXT_INTERNAL_SDF_COMPUTER_H_
