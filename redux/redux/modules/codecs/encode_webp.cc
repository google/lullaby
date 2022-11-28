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

#include "redux/modules/codecs/encode_webp.h"

#include "webp/encode.h"
#include "redux/modules/graphics/image_utils.h"

namespace redux {

DataContainer EncodeWebp(const ImageData& src) {
  const auto image_size = src.GetSize();
  const int channel_count = GetChannelCountForFormat(src.GetFormat());
  const uint8_t* data = reinterpret_cast<const uint8_t*>(src.GetData());
  const size_t stride = src.GetStride();

  uint8_t* webp = nullptr;
  size_t size = 0;
  if (channel_count == 3) {
    size =
        WebPEncodeLosslessRGB(data, image_size.x, image_size.y, stride, &webp);
  } else if (channel_count == 4) {
    size =
        WebPEncodeLosslessRGBA(data, image_size.x, image_size.y, stride, &webp);
  } else {
    LOG(FATAL) << "Unsupported number of image channels: " << channel_count;
  }

  if (webp == nullptr) {
    return DataContainer(reinterpret_cast<std::byte*>(webp), size,
                         [=](const std::byte*) { delete webp; });
  } else {
    return DataContainer();
  }
}

}  // namespace redux
