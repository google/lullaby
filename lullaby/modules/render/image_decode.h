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

#ifndef LULLABY_UTIL_IMAGE_DECODE_H_
#define LULLABY_UTIL_IMAGE_DECODE_H_

#include "lullaby/modules/render/image_data.h"

namespace lull {

/// Header structure for Webp files.
struct WebpHeader {
  uint8_t magic[4];
  uint32_t size;
  uint8_t webp[4];
};

/// Header structure for KTX files.
struct KtxHeader {
  uint8_t magic[12];
  uint32_t endian;
  uint32_t type;
  uint32_t type_size;
  uint32_t format;
  uint32_t internal_format;
  uint32_t base_internal_format;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  uint32_t array_elements;
  uint32_t faces;
  uint32_t mip_levels;
  uint32_t keyvalue_data;
};

/// Header structure for PKM files.
struct PkmHeader {
  char magic[4];          // "PKM "
  char version[2];        // "10"
  uint8_t data_type[2];   // 0 (ETC1_RGB_NO_MIPMAPS)
  uint8_t ext_width[2];   // rounded up to 4 size, big endian.
  uint8_t ext_height[2];  // rounded up to 4 size, big endian.
  uint8_t width[2];       // original, big endian.
  uint8_t height[2];      // original, big endian.
  // Data follows header, size = (ext_width / 4) * (ext_height / 4) * 8
};

/// Header structure for ASTC files.
struct AstcHeader {
  uint8_t magic[4];
  uint8_t blockdim_x;
  uint8_t blockdim_y;
  uint8_t blockdim_z;
  uint8_t xsize[3];
  uint8_t ysize[3];
  uint8_t zsize[3];
};

// Returns the KtxHeader if the buffer is a KTX image file, otherwise returns
// nullptr.
const KtxHeader* GetKtxHeader(const uint8_t* data, size_t len);

// Returns the PkmHeader if the buffer is a PKM image file, otherwise returns
// nullptr.
const PkmHeader* GetPkmHeader(const uint8_t* data, size_t len);

// Returns the AstcHeader if the buffer is a ASTC image file, otherwise returns
// nullptr.
const AstcHeader* GetAstcHeader(const uint8_t* data, size_t len);

// Returns the WebpHeader if the buffer is a WEBP image file, otherwise returns
// nullptr.
const WebpHeader* GetWebpHeader(const uint8_t* data, size_t len);

// Flags used for decoding.
enum DecodeImageFlags {
  kDecodeImage_None = 0,
  kDecodeImage_PremultiplyAlpha = 0x01 << 1,
};

// Decodes image data that is stored in either jpg, png, webp, tga, ktx, pkm,
// or astc format.
ImageData DecodeImage(const uint8_t* data, size_t len, DecodeImageFlags flags);

}  // namespace lull

#endif  // LULLABY_UTIL_IMAGE_DECODE_H_
