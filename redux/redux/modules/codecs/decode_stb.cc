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

#include "redux/modules/codecs/decode_stb.h"

#include <cstdlib>

#include "stb_image.h"

namespace redux {

static void MultiplyRgbByAlpha(uint8_t* data, const vec2i& size) {
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

ImageData DecodeStb(const DataContainer& data, const DecodeStbOptions& opts) {
  int width = 0;
  int height = 0;
  int32_t channels = 0;
  uint8_t* bytes = stbi_load_from_memory(
      reinterpret_cast<const stbi_uc*>(data.GetBytes()),
      static_cast<int>(data.GetNumBytes()), &width, &height, &channels, 0);
  CHECK(bytes != nullptr) << "Unable to decode image.";

  const vec2i size(width, height);
  ImageFormat format = ImageFormat::Invalid;
  if (channels == 4) {
    if (opts.premultiply_alpha) {
      MultiplyRgbByAlpha(bytes, size);
    }
    format = ImageFormat::Rgba8888;
  } else if (channels == 3) {
    format = ImageFormat::Rgb888;
  } else if (channels == 2) {
    format = ImageFormat::LuminanceAlpha88;
  } else if (channels == 1) {
    format = ImageFormat::Luminance8;
  } else {
    LOG(FATAL) << "Unsupported number of channels: " << channels;
  }

  const std::size_t num_bytes = width * height * channels;
  DataContainer decoded_data(
      reinterpret_cast<std::byte*>(bytes), num_bytes,
      [](const std::byte* mem) { std::free(const_cast<std::byte*>(mem)); });
  return ImageData(format, size, std::move(decoded_data));
}

}  // namespace redux
