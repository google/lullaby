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

#include "lullaby/modules/render/image_decode.h"

#include <stdlib.h>

#if !LULLABY_DISABLE_STB_LOADERS
#include "stb/stb_image.h"
#endif

#if !LULLABY_DISABLE_WEBP_LOADER
#ifdef LULLABY_GYP
#include "webp/decode.h"
#else
#include "webp/decode.h"
#endif
#endif

#include "lullaby/modules/render/image_util.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {

static const char kRiffMagicId[] = "RIFF";
static const char kWebpMagicId[] = "WEBP";
static const char kKtxMagicId[] = "\xABKTX 11\xBB\r\n\x1A\n";
static const char kPkmMagicId[] = "PKM ";
static const char kPkmVersion[] = "10";
static const uint8_t kAstcMagicId[] = {0x13, 0xab, 0xa1, 0x5c};

ImageData BuildImageData(
    const uint8_t* bytes, size_t num_bytes, ImageData::Format format,
    const mathfu::vec2i& size,
    std::function<void(const uint8_t*)> deleter = nullptr) {
  DataContainer data;
  if (deleter) {
    DataContainer::DataPtr ptr(const_cast<uint8_t*>(bytes), std::move(deleter));
    data = DataContainer(std::move(ptr), num_bytes, num_bytes,
                         DataContainer::kRead);
  } else {
    data = DataContainer::WrapDataAsReadOnly(bytes, num_bytes);
  }
  return ImageData(format, size, std::move(data));
}

mathfu::vec2i GetKtxImageDimensions(const KtxHeader* header) {
  DCHECK(header != nullptr);
  return mathfu::vec2i(header->width, header->height);
}

int GetPkmSize(const uint8_t (&size)[2]) {
  // Pkm dimension is stored in big endian format.
  return (size[0] << 8) | (size[1]);
}

mathfu::vec2i GetPkmImageDimensions(const PkmHeader* header) {
  DCHECK(header != nullptr);
  const int xsize = GetPkmSize(header->width);
  const int ysize = GetPkmSize(header->height);
  return mathfu::vec2i(xsize, ysize);
}

int GetAstcSize(const uint8_t (&size)[3]) {
  return (size[0]) | (size[1] << 8) | (size[2] << 16);
}

mathfu::vec2i GetAstcImageDimensions(const AstcHeader* header) {
  DCHECK(header != nullptr);
  const int xsize = GetAstcSize(header->xsize);
  const int ysize = GetAstcSize(header->ysize);
  return mathfu::vec2i(xsize, ysize);
}

ImageData DecodeStbi(const uint8_t* src, size_t len, DecodeImageFlags flags) {
#if LULLABY_DISABLE_STB_LOADERS
  // libstb isn't known to be safe, so we've disabled it to eliminate an
  // attack vector.
  LOG(DFATAL) << "Unable to decode image.";
  return ImageData();
#else
  int width = 0;
  int height = 0;
  int32_t channels = 0;
  uint8_t* bytes = stbi_load_from_memory(static_cast<const stbi_uc*>(src),
                                         static_cast<int>(len), &width, &height,
                                         &channels, 0);
  if (bytes == nullptr) {
    LOG(DFATAL) << "Unable to decode image.";
    return ImageData();
  }

  const mathfu::vec2i size(width, height);
  ImageData::Format format = ImageData::kInvalid;
  if (channels == 4) {
    if (flags & kDecodeImage_PremultiplyAlpha) {
      MultiplyRgbByAlpha(bytes, size);
    }
    format = ImageData::kRgba8888;
  } else if (channels == 3) {
    format = ImageData::kRgb888;
  } else if (channels == 2) {
    format = ImageData::kLuminanceAlpha;
  } else if (channels == 1) {
    format = ImageData::kLuminance;
  } else {
    LOG(DFATAL) << "Unsupported number of channels: " << channels;
  }

  const size_t num_bytes = width * height * channels;
  return BuildImageData(bytes, num_bytes, format, size, [](const uint8_t* ptr) {
    free(const_cast<uint8_t*>(ptr));
  });
#endif
}

ImageData DecodeWebp(const uint8_t* data, size_t len, DecodeImageFlags flags) {
#if LULLABY_DISABLE_WEBP_LOADER
  // libwebp isn't known to be safe, so we've disabled it to eliminate an
  // attack vector.
  LOG(DFATAL) << "Unable to decode image.";
  return ImageData();
#else
  WebPDecoderConfig config;
  memset(&config, 0, sizeof(WebPDecoderConfig));
  auto status = WebPGetFeatures(data, len, &config.input);
  if (status != VP8_STATUS_OK) {
    LOG(DFATAL) << "Source image data not an WebP file.";
    return ImageData();
  }
  if (config.input.has_alpha) {
    if (flags & kDecodeImage_PremultiplyAlpha) {
      config.output.colorspace = MODE_rgbA;
    } else {
      config.output.colorspace = MODE_RGBA;
    }
  }
  status = WebPDecode(data, len, &config);
  if (status != VP8_STATUS_OK) {
    LOG(DFATAL) << "Unable to decode WebP data.";
    return ImageData();
  }

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
      LOG(DFATAL) << "Unknown webp colorspace: " << config.output.colorspace;
      return ImageData();
  }

  const mathfu::vec2i size(config.output.width, config.output.height);
  const uint8_t* bytes = config.output.private_memory;
  const size_t num_bytes = size.x * size.y * bytes_per_pixel;
  const ImageData::Format format =
      config.input.has_alpha ? ImageData::kRgba8888 : ImageData::kRgb888;
  return BuildImageData(bytes, num_bytes, format, size, [](const uint8_t* ptr) {
    free(const_cast<uint8_t*>(ptr));
  });
#endif
}

}  // namespace

const WebpHeader* GetWebpHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const WebpHeader*>(data);
  if (len < sizeof(WebpHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kRiffMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  if (memcmp(header->webp, kWebpMagicId, sizeof(header->webp))) {
    return nullptr;
  }
  return header;
}

const KtxHeader* GetKtxHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const KtxHeader*>(data);
  if (len < sizeof(KtxHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kKtxMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  return header;
}

const PkmHeader* GetPkmHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const PkmHeader*>(data);
  if (len < sizeof(PkmHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kPkmMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  if (memcmp(header->version, kPkmVersion, sizeof(header->version))) {
    return nullptr;
  }
  return header;
}

const AstcHeader* GetAstcHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const AstcHeader*>(data);
  if (len < sizeof(AstcHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kAstcMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  return header;
}

ImageData DecodeImage(const uint8_t* data, size_t len, DecodeImageFlags flags) {
  if (auto header = GetAstcHeader(data, len)) {
    const mathfu::vec2i size = GetAstcImageDimensions(header);
    return BuildImageData(data, len, ImageData::kAstc, size);
  } else if (auto header = GetPkmHeader(data, len)) {
    const mathfu::vec2i size = GetPkmImageDimensions(header);
    return BuildImageData(data, len, ImageData::kPkm, size);
  } else if (auto header = GetKtxHeader(data, len)) {
    const mathfu::vec2i size = GetKtxImageDimensions(header);
    return BuildImageData(data, len, ImageData::kKtx, size);
  } else if (GetWebpHeader(data, len)) {
    return DecodeWebp(data, len, flags);
  } else {
    return DecodeStbi(data, len, flags);
  }
}

}  // namespace lull
