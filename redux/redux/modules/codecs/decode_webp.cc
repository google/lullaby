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

#include "redux/modules/codecs/decode_webp.h"

#include <cstdlib>

#include "webp/decode.h"
#include "webp/demux.h"
#include "webp/mux_types.h"

namespace redux {

ImageData DecodeWebp(const DataContainer& data, const DecodeWebpOptions& opts) {
  const auto* ptr = reinterpret_cast<const uint8_t*>(data.GetBytes());
  const size_t len = data.GetNumBytes();

  WebPDecoderConfig config;
  memset(&config, 0, sizeof(WebPDecoderConfig));
  auto status = WebPGetFeatures(ptr, len, &config.input);
  CHECK(status == VP8_STATUS_OK) << "Source image data not an WebP file.";

  if (config.input.has_alpha) {
    if (opts.premultiply_alpha) {
      config.output.colorspace = MODE_rgbA;
    } else {
      config.output.colorspace = MODE_RGBA;
    }
  }
  status = WebPDecode(ptr, len, &config);
  CHECK(status == VP8_STATUS_OK) << "Unable to decode WebP data.";

  int bytes_per_pixel = 0;
  switch (config.output.colorspace) {
    case MODE_RGB:
    case MODE_BGR:
      bytes_per_pixel = 3;
      break;
    case MODE_RGBA:
    case MODE_BGRA:
    case MODE_ARGB:
    case MODE_rgbA:
    case MODE_bgrA:
    case MODE_Argb:
      bytes_per_pixel = 4;
      break;
    case MODE_RGBA_4444:
    case MODE_rgbA_4444:
    case MODE_RGB_565:
      bytes_per_pixel = 2;
      break;
    default:
      LOG(FATAL) << "Unknown webp colorspace: " << config.output.colorspace;
  }

  const vec2i size(config.output.width, config.output.height);
  std::byte* bytes = reinterpret_cast<std::byte*>(config.output.private_memory);
  const size_t num_bytes = size.x * size.y * bytes_per_pixel;
  const ImageFormat format =
      config.input.has_alpha ? ImageFormat::Rgba8888 : ImageFormat::Rgb888;

  DataContainer decoded_data(bytes, num_bytes, [](const std::byte* mem) {
    std::free(const_cast<std::byte*>(mem));
  });
  return ImageData(format, size, std::move(decoded_data));
}

}  // namespace redux
