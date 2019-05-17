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

#include "lullaby/tools/texture_pipeline/encode_webp.h"

#include "webp/encode.h"

namespace lull {
namespace tool {

ByteArray EncodeWebp(const ImageData& src) {
  uint8_t* webp = nullptr;
  size_t size;

  const auto image_size = src.GetSize();
  const int channel_count = src.GetChannelCount(src.GetFormat());

  if (channel_count == 3) {
    size = WebPEncodeLosslessRGB(src.GetBytes(), image_size.x, image_size.y,
                                 src.GetStride(), &webp);
  } else if (channel_count == 4) {
    size = WebPEncodeLosslessRGBA(src.GetBytes(), image_size.x, image_size.y,
                                  src.GetStride(), &webp);
  } else {
    LOG(ERROR) << "Unsupported number of image channels: " << channel_count;
  }

  if (webp) {
    ByteArray webp_data = ByteArray(webp, webp + size);
    delete webp;
    return webp_data;
  }

  return ByteArray();
}

}  // namespace tool
}  // namespace lull
