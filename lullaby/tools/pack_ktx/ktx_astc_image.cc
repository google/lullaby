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

#include "lullaby/tools/pack_ktx/ktx_astc_image.h"

#include <string.h>
#include <algorithm>
#include <fstream>

#include "lullaby/util/logging.h"

// Transitively included from "ktx.h"
// Including it twice causes GLAPI to be redefined when using '-c opt'
// #include "GL/gl.h"

namespace lull {
namespace tool {

namespace {
constexpr uint8_t kAstcFileMagic[4] = {0x13, 0xab, 0xa1, 0x5c};
// ASTC block is always 128 bits
// https://www.khronos.org/opengl/wiki/ASTC_Texture_Compression#Variable_block_sizes
constexpr uint32_t kAstcBlockByteSize = 16;
// TODO(gavindodd): validate the size?
uint32_t Get24BitUnsignedInt(const uint8_t byte[3]) {
  return byte[0] + (byte[1] << 8) + (byte[2] << 16);
}

void Set24BitUnsignedInt(const uint32_t value, uint8_t byte_out[3]) {
  byte_out[0] = value & 0xff;
  byte_out[1] = (value >> 8) & 0xff;
  byte_out[2] = (value >> 16) & 0xff;
}

struct AstcBlockSize {
  uint8_t blockdim_x;  // x, y, z size in texels
  uint8_t blockdim_y;
  uint8_t blockdim_z;
};

// Returns the block size of the given GL ASTC format code
// All block dimensions will be 0 for an unrecognised format.
AstcBlockSize AstcBlockSizeFromGlData(const uint32_t gl_internal_format) {
  AstcBlockSize block_size = {0, 0, 1};
  switch (gl_internal_format) {
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
    case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
      block_size.blockdim_x = 4;
      block_size.blockdim_y = 4;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
    case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
      block_size.blockdim_x = 5;
      block_size.blockdim_y = 4;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
      block_size.blockdim_x = 5;
      block_size.blockdim_y = 5;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
      block_size.blockdim_x = 6;
      block_size.blockdim_y = 5;
      break;
    case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
      block_size.blockdim_x = 6;
      block_size.blockdim_y = 6;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
      block_size.blockdim_x = 8;
      block_size.blockdim_y = 5;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
    case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
      block_size.blockdim_x = 8;
      block_size.blockdim_y = 6;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
    case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
      block_size.blockdim_x = 8;
      block_size.blockdim_y = 8;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
      block_size.blockdim_x = 10;
      block_size.blockdim_y = 5;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
      block_size.blockdim_x = 10;
      block_size.blockdim_y = 6;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
      block_size.blockdim_x = 10;
      block_size.blockdim_y = 8;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
    case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
      block_size.blockdim_x = 10;
      block_size.blockdim_y = 10;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
    case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
      block_size.blockdim_x = 12;
      block_size.blockdim_y = 10;
      break;
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
    case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
      block_size.blockdim_x = 12;
      block_size.blockdim_y = 12;
      break;
    default:
      // Clear z so block size is in a consistent error state
      block_size.blockdim_z = 0;
      break;
  }
  return block_size;
}

uint32_t GlAstcInternalFormat(uint8_t blockdim_x, uint8_t blockdim_y,
                              bool srgb) {
  switch (blockdim_x) {
    case 4:
      if (blockdim_y == 4) {
        return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
                    : GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
      }
      break;
    case 5:
      switch (blockdim_y) {
        case 4:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR
                      : GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
        case 5:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR
                      : GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
        default:
          break;
      }
      break;
    case 6:
      switch (blockdim_y) {
        case 5:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR
                      : GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
        case 6:
          return srgb ? GL_COMPRESSED_RGBA_ASTC_6x6_KHR
                      : GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
        default:
          break;
      }
      break;
    case 8:
      switch (blockdim_y) {
        case 5:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR
                      : GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
        case 6:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR
                      : GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
        case 8:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
                      : GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
        default:
          break;
      }
      break;
    case 10:
      switch (blockdim_y) {
        case 5:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR
                      : GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
        case 6:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR
                      : GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
        case 8:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR
                      : GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
        case 10:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR
                      : GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
        default:
          break;
      }
      break;
    case 12:
      switch (blockdim_y) {
        case 10:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR
                      : GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
        case 12:
          return srgb ? GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR
                      : GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
        default:
          break;
      }
      break;
    default:
      break;
  }
  return 0;
}

// from
// https://arm-software.github.io/opengl-es-sdk-for-android/structastc__header.html
// and astc_toplevel.cpp
bool CheckAstcMagic(const uint8_t* const magic) {
  return memcmp(magic, kAstcFileMagic, sizeof(kAstcFileMagic)) == 0;
}

KtxImage::ErrorCode ValidateAstcHeader(const KtxAstcImage::AstcHeader& header,
                                       bool srgb) {
  if (!CheckAstcMagic(header.magic)) {
    std::cerr << "No magic!\n";
    return KtxImage::kFormatError;
  }
  // https://www.khronos.org/opengl/wiki/ASTC_Texture_Compression#Unavailable_features
  if (header.blockdim_z != 1) {
    std::cerr << "3D blocks not yet supported\n";
    return KtxImage::kFormatError;
  }
  if (GlAstcInternalFormat(header.blockdim_x, header.blockdim_y, srgb) == 0) {
    std::cerr << "Invalid block size\n";
    return KtxImage::kCorruptError;
  }
  uint32_t depth = Get24BitUnsignedInt(header.zsize);
  if (depth != 1) {
    std::cerr << "Array images not yet supported\n";
    return KtxImage::kFormatError;
  }

  return KtxImage::kOK;
}

size_t GetAstcDataSize(const KtxAstcImage::AstcHeader& header) {
  // Calculate expected data size and validate
  uint32_t width = Get24BitUnsignedInt(header.xsize);
  uint32_t height = Get24BitUnsignedInt(header.ysize);
  uint32_t cell_width = (width + header.blockdim_x - 1) / header.blockdim_x;
  uint32_t cell_height = (height + header.blockdim_y - 1) / header.blockdim_y;
  return cell_width * cell_height * kAstcBlockByteSize;
}

}  // namespace

// TODO(gavindodd): add passing or inferring SRGB flag
KtxImage::ErrorCode KtxAstcImage::Open(const std::string& filename,
                                       ImagePtr* image_out) {
  // TODO(gavindodd): this should be passed in
  const bool srgb = false;
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  std::ifstream astc_file(filename, std::ios::binary);
  if (!astc_file.is_open()) {
    std::cerr << "astc::Open Could not open " << filename << "\n";
    return kFileOpenError;
  }
  AstcHeader header;
  astc_file.read(reinterpret_cast<char*>(&header), sizeof(header));
  if (!astc_file) {
    std::cerr << "Could not read file header!\n";
    astc_file.close();
    return kFileReadError;
  }
  ErrorCode result = ValidateAstcHeader(header, srgb);
  if (result != kOK) {
    astc_file.close();
    return result;
  }

  // Calculate expected data size and validate
  size_t expected_data_size = GetAstcDataSize(header);
  std::streampos data_pos = 0;
  data_pos = astc_file.tellg();
  astc_file.seekg(0, astc_file.end);
  uint64_t image_data_size = astc_file.tellg() - data_pos;
  if (image_data_size != expected_data_size) {
    std::cerr << "Image data size did not match expected: "
              << std::to_string(image_data_size)
              << " != " << std::to_string(expected_data_size) << "\n";
    astc_file.close();
    return kCorruptError;
  }
  astc_file.seekg(data_pos, astc_file.beg);
  uint8_t* image_data = new uint8_t[image_data_size];
  astc_file.read(reinterpret_cast<char*>(image_data), image_data_size);
  if (!astc_file) {
    std::cerr << "Could not read file data!\n";
    astc_file.close();
    delete[] image_data;
    return kFileReadError;
  }

  image_out->reset(new KtxAstcImage(filename, header, image_data,
                                    image_data_size, srgb, true));
  return kOK;
}

KtxImage::ErrorCode KtxAstcImage::Create(const uint8_t* data, size_t data_size,
                                         ImagePtr* image_out) {
  // TODO(gavindodd): this should be passed in
  const bool srgb = false;
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  DCHECK_NE(data, nullptr);
  if (data == nullptr) {
    return kBadParameters;
  }
  if (data_size < sizeof(AstcHeader)) {
    return kBadParameters;
  }
  const AstcHeader* header = reinterpret_cast<const AstcHeader*>(data);
  ErrorCode result = ValidateAstcHeader(*header, srgb);
  if (result != kOK) {
    return result;
  }

  // Calculate expected data size and validate
  size_t expected_data_size = GetAstcDataSize(*header);
  size_t image_data_size = data_size - sizeof(AstcHeader);
  if (image_data_size != expected_data_size) {
    std::cerr << "Image data size did not match expected: "
              << std::to_string(image_data_size)
              << " != " << std::to_string(expected_data_size) << "\n";
    return kCorruptError;
  }
  const uint8_t* image_data = data + sizeof(AstcHeader);
  image_out->reset(new KtxAstcImage("<memory>", *header, image_data,
                                    image_data_size, srgb, false));
  return kOK;
}

KtxImage::ErrorCode KtxAstcImage::WriteASTC(const KtxImage& image,
                                            uint32_t index,
                                            const std::string& filename) {
  AstcBlockSize block_size = AstcBlockSizeFromGlData(image.GlInternalFormat());
  if ((block_size.blockdim_x == 0) || (block_size.blockdim_x == 0) ||
      (block_size.blockdim_z == 0)) {
    // This is not an ASTC format image
    return kFormatError;
  }
  if (block_size.blockdim_z != 1) {
    // 3D ASTC is not supported
    std::cerr << "3D blocks not yet supported\n";
    return kFormatError;
  }
  const std::vector<KTX_image_info>& image_info = image.GetImageInfo();
  if (image_info.size() <= index) {
    std::cerr << "Image index out of range\n";
    return kBadParameters;
  }
  uint32_t mip_level = index / image.NumberOfFaces();
  uint32_t width = image.PixelWidth() >> mip_level;
  uint32_t height = image.PixelHeight() >> mip_level;
  uint32_t depth = image.PixelDepth() >> mip_level;
  width = std::max(width, 1u);
  height = std::max(height, 1u);
  depth = std::max(depth, 1u);

  if ((width > 0xffffffu) || (height > 0xffffffu) || (depth > 0xffffffu)) {
    std::cerr << "Image size too large for ASTC\n";
    return kFormatError;
  }

  AstcHeader header;
  memcpy(header.magic, kAstcFileMagic, sizeof(header.magic));
  header.blockdim_x = block_size.blockdim_x;
  header.blockdim_y = block_size.blockdim_y;
  header.blockdim_z = block_size.blockdim_z;
  Set24BitUnsignedInt(width, header.xsize);
  Set24BitUnsignedInt(height, header.ysize);
  Set24BitUnsignedInt(depth, header.zsize);

  std::ofstream astc_file(filename, std::ios::binary);
  if (!astc_file.is_open()) {
    std::cerr << "astc::Open Could not open " << filename << "\n";
    return kFileOpenError;
  }
  astc_file.write(reinterpret_cast<char*>(&header), sizeof(header));
  if (!astc_file) {
    std::cerr << "Could not write file header!\n";
    astc_file.close();
    return kFileWriteError;
  }
  astc_file.write(reinterpret_cast<char*>(image_info[index].data),
                  image_info[index].size);
  if (!astc_file) {
    std::cerr << "Could not write image data!\n";
    astc_file.close();
    return kFileWriteError;
  }

  return kOK;
}

KtxAstcImage::KtxAstcImage(const std::string& filename,
                           const AstcHeader& header, const uint8_t* data,
                           const uint64_t data_size, bool srgb,
                           bool take_ownership)
    : filename_(filename),
      header_(header),
      data_(data),
      srgb_(srgb),
      owns_data_(take_ownership) {
  KTX_image_info image_info;
  image_info.data = const_cast<GLubyte*>(data);
  image_info.size = data_size;
  AddImageInfo(image_info);
}

KtxAstcImage::~KtxAstcImage() {
  if (owns_data_) {
    delete[] data_;
  }
}

uint32_t KtxAstcImage::GlType() const {
  // GL type is always 0 for compressed textures.
  return 0;
}

uint32_t KtxAstcImage::GlTypeSize() const {
  // GL type size is always 1 for compressed textures.
  return 1;
}

uint32_t KtxAstcImage::GlFormat() const {
  // GL format is always 0 for compressed textures
  return 0;
}

uint32_t KtxAstcImage::GlInternalFormat() const {
  return GlAstcInternalFormat(header_.blockdim_x, header_.blockdim_y, srgb_);
}

uint32_t KtxAstcImage::GlBaseInternalFormat() const {
  // ASTC only supports COMPRESSED_RGBA and COMPRESSED_SRGB_ALPHA which map to
  // internal format RGBA
  return GL_RGBA;
}

uint32_t KtxAstcImage::PixelWidth() const {
  return Get24BitUnsignedInt(header_.xsize);
}

uint32_t KtxAstcImage::PixelHeight() const {
  return Get24BitUnsignedInt(header_.ysize);
}

uint32_t KtxAstcImage::PixelDepth() const {
  uint32_t depth = Get24BitUnsignedInt(header_.zsize);
  return (depth > 1) ? depth : 0;
}

uint32_t KtxAstcImage::NumberOfArrayElements() const { return 0; }
uint32_t KtxAstcImage::NumberOfFaces() const { return 1; }
uint32_t KtxAstcImage::NumberOfMipmapLevels() const { return 1; }

}  // namespace tool
}  // namespace lull
