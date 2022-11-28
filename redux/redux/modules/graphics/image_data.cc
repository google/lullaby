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

#include "redux/modules/graphics/image_data.h"

#include "redux/modules/base/logging.h"
#include "redux/modules/graphics/image_utils.h"

namespace redux {

static constexpr std::size_t kBitsPerByte = 8;

ImageData::ImageData(ImageFormat format, const vec2i& size, DataContainer data,
                     std::size_t stride)
    : format_(format),
      size_(size),
      data_(std::move(data)),
      stride_(stride == 0 ? CalculateMinStride(format, size) : stride) {
  CHECK_GE(stride_, CalculateMinStride(format, size));
}

int ImageData::GetRowAlignment() const {
  if ((stride_ % 8) == 0) {
    return 8;
  }
  if ((stride_ % 4) == 0) {
    return 4;
  }
  if ((stride_ % 2) == 0) {
    return 2;
  }
  return 1;
}

int ImageData::GetStrideInPixels() const {
  const std::size_t bits_per_pixel = GetBitsPerPixel(format_);
  if (bits_per_pixel == 0) {
    return 0;
  }
  const std::size_t stride_in_bits = stride_ * kBitsPerByte;
  return static_cast<int>(stride_in_bits / bits_per_pixel);
}

ImageData ImageData::Clone() const {
  return ImageData(format_, size_, data_.Clone(), stride_);
}

ImageData ImageData::Rebind(std::shared_ptr<ImageData> image) {
  DataContainer data = DataContainer::WrapDataInSharedPtr(
      image->GetData(), image->GetNumBytes(), image);
  return ImageData(image->format_, image->size_, std::move(data));
}

}  // namespace redux
