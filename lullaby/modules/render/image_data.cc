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

#include "lullaby/modules/render/image_data.h"

#include "lullaby/util/logging.h"

namespace lull {

static constexpr size_t kBitsPerByte = 8;

ImageData::ImageData(Format format, const mathfu::vec2i& size,
                     DataContainer data, size_t stride)
    : format_(format),
      size_(size),
      data_(std::move(data)),
      stride_(stride == 0 ? CalculateMinStride(format, size) : stride) {
  DCHECK_GE(stride_, CalculateMinStride(format, size));
  const size_t data_size = data_.GetSize();
  const size_t total_size = CalculateDataSize(format, size);
  if (data_size < total_size) {
    data_.Advance(total_size - data_size);
  }
}

ImageData ImageData::CreateHeapCopy() const {
  return ImageData(format_, size_, data_.CreateHeapCopy());
}

size_t ImageData::GetBitsPerPixel(Format format) {
  switch (format) {
    // 8 bpp:
    case kAlpha:
    case kLuminance:
      return 8;

    // 16 bpp:
    case kLuminanceAlpha:
    case kRg88:
    case kRgb565:
    case kRgba4444:
    case kRgba5551:
      return 16;

    // 24 bpp:
    case kRgb888:
      return 24;

    // 32 bpp:
    case kRgba8888:
      return 32;

    // Container formats:
    case kAstc:
    case kPkm:
    case kKtx:
      return 0;

    default:
      LOG(WARNING) << "Invalid image format " << format;
      return 0;
  }
}

size_t ImageData::GetChannelCount(Format format) {
  switch (format) {
    // 1 channel:
    case kAlpha:
    case kLuminance:
      return 1;

    // 2 channels:
    case kLuminanceAlpha:
      return 2;

    // 3 channels:
    case kRgb565:
    case kRgb888:
      return 3;

    // 4 channels:
    case kRgba4444:
    case kRgba5551:
    case kRgba8888:
      return 4;

    // Container formats:
    case kAstc:
    case kPkm:
    case kKtx:
      return 0;

    default:
      LOG(WARNING) << "Invalid image format " << format;
      return 0;
  }
}

size_t ImageData::CalculateDataSize(Format format, const mathfu::vec2i& size) {
  return size.y * CalculateMinStride(format, size);
}

size_t ImageData::CalculateMinStride(Format format, const mathfu::vec2i& size) {
  const size_t bits_per_row = size.x * GetBitsPerPixel(format);
  return (bits_per_row + kBitsPerByte - 1) / kBitsPerByte;
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
  const size_t bits_per_pixel = GetBitsPerPixel(format_);
  if (bits_per_pixel == 0) {
    return 0;
  }
  const size_t stride_in_bits = stride_ * kBitsPerByte;
  return static_cast<int>(stride_in_bits / bits_per_pixel);
}

}  // namespace lull
