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

#include "redux/modules/graphics/image_utils.h"

#include <cstddef>
#include <cstring>

namespace redux {

static constexpr std::size_t kBitsPerByte = 8;

// Several common magic numbers are described here:
// https://en.wikipedia.org/wiki/List_of_file_signatures
static constexpr std::uint8_t kRiffMagicId[] = {0x52, 0x49, 0x46, 0x46};
static constexpr std::uint8_t kWebpMagicId[] = {0x57, 0x45, 0x42, 0x50};
static constexpr std::uint8_t kAstcMagicId[] = {0x13, 0xab, 0xa1, 0x5c};
static constexpr std::uint8_t kJpgMagicId[] = {0xff, 0xd8, 0xff, 0xe0};
static constexpr std::uint8_t kKtxMagicId[] = "\xABKTX 11\xBB\r\n\x1A\n";
static constexpr std::uint8_t kPngMagicId[] = {0x89, 0x50, 0x4E, 0x47,
                                          0x0D, 0x0A, 0x1A, 0x0A};
static constexpr size_t kWebpMagicOffset = 8;

// Returns true if the data in header (at the given offset) matches the magic
// value.
template <std::size_t N>
static bool CheckMagic(absl::Span<const std::byte> header,
                       const std::uint8_t (&magic)[N], std::size_t offset = 0) {
  static_assert(N == sizeof(magic));
  if (header.size() < sizeof(magic) + offset) {
    return false;
  }
  const std::byte* ptr = header.data() + offset;
  return std::memcmp(ptr, magic, sizeof(magic)) == 0;
}

ImageFormat IdentifyImageTypeFromHeader(absl::Span<const std::byte> header) {
  if (CheckMagic(header, kRiffMagicId) &&
      CheckMagic(header, kWebpMagicId, kWebpMagicOffset)) {
    return ImageFormat::Webp;
  } else if (CheckMagic(header, kJpgMagicId)) {
    return ImageFormat::Jpg;
  } else if (CheckMagic(header, kPngMagicId)) {
    return ImageFormat::Png;
  } else if (CheckMagic(header, kAstcMagicId)) {
    return ImageFormat::Astc;
  } else if (CheckMagic(header, kKtxMagicId)) {
    return ImageFormat::Ktx;
  } else {
    return ImageFormat::Invalid;
  }
}

size_t GetBitsPerPixel(ImageFormat format) {
  constexpr size_t kBitsPerByte = 8;
  return GetBytesPerPixel(format) * kBitsPerByte;
}

size_t GetBytesPerPixel(ImageFormat format) {
  switch (format) {
    case ImageFormat::Alpha8:
    case ImageFormat::Luminance8:
      return 1;
    case ImageFormat::LuminanceAlpha88:
    case ImageFormat::Rg88:
    case ImageFormat::Rgb565:
    case ImageFormat::Rgba4444:
    case ImageFormat::Rgba5551:
      return 2;
    case ImageFormat::Rgb888:
      return 3;
    case ImageFormat::Rgba8888:
    case ImageFormat::Rgbm8888:
      return 4;
    default:
      LOG(FATAL) << "Unknown format: " << ToString(format);
  }
}

std::size_t CalculateDataSize(ImageFormat format, const vec2i& size) {
  return size.y * CalculateMinStride(format, size);
}

std::size_t CalculateMinStride(ImageFormat format, const vec2i& size) {
  if (IsCompressedFormat(format) || IsContainerFormat(format)) {
    return 0;
  }

  const std::size_t bits_per_row = size.x * GetBitsPerPixel(format);
  return (bits_per_row + kBitsPerByte - 1) / kBitsPerByte;
}

std::size_t GetChannelCountForFormat(ImageFormat format) {
  switch (format) {
    // 1 channel:
    case ImageFormat::Alpha8:
    case ImageFormat::Luminance8:
      return 1;

    // 2 channels:
    case ImageFormat::LuminanceAlpha88:
      return 2;

    // 3 channels:
    case ImageFormat::Rgb565:
    case ImageFormat::Rgb888:
      return 3;

    // 4 channels:
    case ImageFormat::Rgba4444:
    case ImageFormat::Rgba5551:
    case ImageFormat::Rgba8888:
    case ImageFormat::Rgbm8888:
      return 4;

    // Container formats:
    case ImageFormat::Ktx:
      LOG(FATAL) << "Compressed image: " << ToString(format);

    default:
      LOG(FATAL) << "Invalid image format: " << ToString(format);
  }
}

bool IsCompressedFormat(ImageFormat format) {
  return format == ImageFormat::Astc || format == ImageFormat::Jpg ||
         format == ImageFormat::Png || format == ImageFormat::Webp;
}

bool IsContainerFormat(ImageFormat format) {
  return format == ImageFormat::Ktx;
}

}  // namespace redux
