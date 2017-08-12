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

#include "lullaby/modules/render/image_util.h"

#include "lullaby/util/logging.h"

namespace lull {

void ConvertRgb888ToRgba8888(const uint8_t* rgb_ptr, const mathfu::vec2i& size,
                             uint8_t* out_rgba_ptr) {
  if (!rgb_ptr || !out_rgba_ptr) {
    LOG(DFATAL) << "Failed to convert RGB to RGBA.";
    return;
  }

  const int num_pixels = size.x * size.y;
  const uint8_t* src_ptr = rgb_ptr;
  uint8_t* dst_ptr = out_rgba_ptr;
  for (int i = 0; i < num_pixels; ++i) {
    *dst_ptr++ = *src_ptr++;
    *dst_ptr++ = *src_ptr++;
    *dst_ptr++ = *src_ptr++;
    *dst_ptr++ = 255;
  }
}

}  // namespace lull
