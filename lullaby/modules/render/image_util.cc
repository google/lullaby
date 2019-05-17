/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/util/color.h"
#include "lullaby/util/logging.h"

namespace lull {

void MultiplyRgbByAlpha(uint8_t* data, const mathfu::vec2i& size) {
  const int num_pixels = size.x * size.y;
  for (int i = 0; i < num_pixels; ++i, data += 4) {
    const auto alpha = static_cast<uint16_t>(data[3]);
    data[0] =
        static_cast<uint8_t>((static_cast<uint16_t>(data[0]) * alpha) / 255);
    data[1] =
        static_cast<uint8_t>((static_cast<uint16_t>(data[1]) * alpha) / 255);
    data[2] =
        static_cast<uint8_t>((static_cast<uint16_t>(data[2]) * alpha) / 255);
  }
}

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

ImageData CreateWhiteImage() {
  constexpr int kTextureSize = 2;
  static const Color4ub data[kTextureSize * kTextureSize];
  const mathfu::vec2i size(kTextureSize, kTextureSize);
  ImageData image(ImageData::kRgba8888, size,
                  DataContainer::WrapDataAsReadOnly(data, sizeof(data)));
  return image;
}

ImageData CreateInvalidImage() {
  constexpr int kTextureSize = 64;
  const Color4ub kUglyGreen(0, 255, 0, 255);
  const Color4ub kUglyPink(255, 0, 128, 255);

  static Color4ub data[kTextureSize * kTextureSize];
  static bool initialized = [&]() {
    Color4ub* ptr = data;
    for (int y = 0; y < kTextureSize; ++y) {
      for (int x = 0; x < kTextureSize; ++x) {
        *ptr = ((x / 8 + y / 8) % 2 == 0) ? kUglyGreen : kUglyPink;
        ++ptr;
      }
    }
    return true;
  }();
  (void)initialized;

  const mathfu::vec2i size(kTextureSize, kTextureSize);
  return ImageData(ImageData::kRgba8888, size,
                   DataContainer::WrapDataAsReadOnly(data, sizeof(data)));
}
}  // namespace lull
