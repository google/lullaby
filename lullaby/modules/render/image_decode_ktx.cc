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

#include "lullaby/modules/render/image_decode_ktx.h"

#include "GL/gl.h"
#include "GL/glext.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/util/logging.h"

namespace lull {

static const char kKtxMagicId[] = "\xABKTX 11\xBB\r\n\x1A\n";

static bool IsEtc(GLenum format) {
  switch (format) {
#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_4_3)
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      return true;
#endif  // GL_ES_VERSION_3_0 || GL_VERSION_4_3
    default:
      return false;
  }
}

static mathfu::vec2i GetBlockSize(GLenum format) {
  switch (format) {
#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_4_3)
    // ETC1 and ETC2 use 4x4 blocks.
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      return mathfu::vec2i(4, 4);
#endif  // GL_ES_VERSION_3_0 || GL_VERSION_4_3
#if defined(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR)
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
      return mathfu::vec2i(4, 4);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
      return mathfu::vec2i(5, 4);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
      return mathfu::vec2i(5, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
      return mathfu::vec2i(6, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
      return mathfu::vec2i(6, 6);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
      return mathfu::vec2i(8, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
      return mathfu::vec2i(8, 6);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
      return mathfu::vec2i(8, 8);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
      return mathfu::vec2i(10, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
      return mathfu::vec2i(10, 6);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
      return mathfu::vec2i(10, 8);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
      return mathfu::vec2i(10, 10);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
      return mathfu::vec2i(12, 10);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
      return mathfu::vec2i(12, 12);
#endif  // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
    case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
      return mathfu::vec2i(4, 4);
    case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
      return mathfu::vec2i(5, 4);
    case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
      return mathfu::vec2i(5, 5);
    case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
      return mathfu::vec2i(6, 5);
    case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
      return mathfu::vec2i(6, 6);
    case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
      return mathfu::vec2i(8, 5);
    case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
      return mathfu::vec2i(8, 6);
    case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
      return mathfu::vec2i(8, 8);
    case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
      return mathfu::vec2i(10, 5);
    case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
      return mathfu::vec2i(10, 6);
    case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
      return mathfu::vec2i(10, 8);
    case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
      return mathfu::vec2i(10, 10);
    case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
      return mathfu::vec2i(12, 10);
    case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
      return mathfu::vec2i(12, 12);
    default:
      // Uncompressed textures effectively have 1x1 blocks.
      return mathfu::vec2i(1, 1);
  }
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


int ProcessKtx(const KtxHeader* header, const KtxProcessor processor) {
  int num_images = 0;
  if (header == nullptr || processor == nullptr) {
    return num_images;
  }

  const bool is_etc = IsEtc(header->internal_format);
  const mathfu::vec2i block_size = GetBlockSize(header->internal_format);

  // Get the pointer to the ktx image data payload.
  const uint8_t* data = reinterpret_cast<const uint8_t*>(header) +
                        sizeof(KtxHeader) + header->keyvalue_data;

  int mip_width = header->width;
  int mip_height = header->height;
  for (uint32_t mip_level = 0; mip_level < header->mip_levels; ++mip_level) {
    // Guard against extra mip levels in the ktx when using ETC compression.
    if (is_etc && (mip_width < block_size.x || mip_height < block_size.y)) {
      LOG(ERROR) << "KTX file has too many mips.";
      break;
    }

    // For cube maps imageSize is the number of bytes in each face of the
    // texture for the current LOD level, not including bytes in cubePadding
    // or mipPadding.
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.16
    const int32_t data_size = *(reinterpret_cast<const int32_t*>(data));
    data += sizeof(data_size);

    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.17
    // We do not need to worry about cubePadding as we only support etc and astc
    // which are both aligned to 8 or 16 bytes (block size)
    DCHECK_EQ((data_size % 4), 0);

    // Keep loading mip data even if one of our calculated dimensions goes
    // to 0, but maintain a min size of 1.  This is needed to get non-square
    // mip chains to work using ETC2 (eg a 256x512 needs 10 mips defined).
    const int num_faces = header->faces;
    const int width = std::max(mip_width, 1);
    const int height = std::max(mip_height, 1);
    const mathfu::vec2i dimensions(width, height);

    processor(data, data_size, num_faces, dimensions, mip_level, block_size);
    ++num_images;

    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.18
    // We do not need to worry about mipPadding as we only support etc and astc
    // which are both aligned to 8 or 16 bytes (block size)
    uint32_t mip_size = data_size * num_faces;
    DCHECK_EQ((mip_size % 4), 0);
    data += mip_size;
    mip_width /= 2;
    mip_height /= 2;
  }

  return num_images;
}

}  // namespace lull
