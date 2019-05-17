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

#include "lullaby/tools/texture_pipeline/mipmap_generator.h"

#include <vector>
#include "lullaby/modules/render/image_data.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace tool {

int UvAndChannelToIndex(const ImageData& image, int u, int v, int c) {
  const int channel_count = ImageData::GetChannelCount(image.GetFormat());
  return (v * image.GetSize().x + u) * channel_count + c;
}

std::vector<ImageData> GenerateMipmapLevels(ImageData image) {
  const ImageData::Format format = image.GetFormat();
  const int chan_count = ImageData::GetChannelCount(format);
  if (chan_count == 0) {
    LOG(ERROR) << "Unsupported format";
    return {};
  }
  int bits_per_channel = ImageData::GetBitsPerPixel(format) / chan_count;
  if (bits_per_channel != 8) {
    LOG(ERROR) << "Only 8 bit images are supported";
    return {};
  }

  std::vector<ImageData> images;
  images.push_back(std::move(image));
  ImageData const* src = &images.front();

  while (src->GetSize().x > 1 && src->GetSize().y > 1) {
    const mathfu::vec2i src_size = src->GetSize();
    const mathfu::vec2i dst_size = src_size / 2;
    const size_t dst_data_size = ImageData::CalculateDataSize(format, dst_size);
    DataContainer dst_data =
        DataContainer::CreateHeapDataContainer(dst_data_size);
    ImageData dst = ImageData(format, dst_size, std::move(dst_data));
    const uint8_t* src_bytes = src->GetBytes();
    uint8_t* dst_bytes = dst.GetMutableBytes();

    for (int yy = 0; yy < dst_size.y; ++yy) {
      for (int xx = 0; xx < dst_size.x; ++xx) {
        for (int cc = 0; cc < chan_count; ++cc) {
          // Simple box filter, and this does not correctly account for non-
          // power-of-two textures, but instead behaves as nearest-neighbor.  A
          // better implementation of box would use bilinear interpolation to
          // help with pixel siting in non-POT cases.
          // TODO: Improve mipmap generation quality.
          const int xx2 = xx * 2, yy2 = yy * 2;
          const int p00 = UvAndChannelToIndex(*src, xx2 + 0, yy2 + 0, cc);
          const int p01 = UvAndChannelToIndex(*src, xx2 + 1, yy2 + 0, cc);
          const int p10 = UvAndChannelToIndex(*src, xx2 + 0, yy2 + 1, cc);
          const int p11 = UvAndChannelToIndex(*src, xx2 + 1, yy2 + 1, cc);
          const int total =
              src_bytes[p00] + src_bytes[p10] + src_bytes[p01] + src_bytes[p11];
          dst_bytes[UvAndChannelToIndex(dst, xx, yy, cc)] = total / 4;
        }
      }
    }

    images.push_back(std::move(dst));
    src = &images.back();
  }

  return images;
}

}  // namespace tool
}  // namespace lull
