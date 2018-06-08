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

#include "lullaby/tools/pack_ktx/ktx_direct_image.h"

#include <string.h>
#include <cstring>
#include <fstream>
#include <map>

namespace lull {
namespace tool {

namespace {
const uint8_t kKtxFileMagic[] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                                 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
const uint32_t kKtxNativeEndian = 0x04030201;
const uint32_t kReverseEndian = 0x01020304;
const uint32_t kValuePadding = 4;
const uint32_t kMipPadding = 4;
const uint32_t kCubePadding = 4;

uint32_t SwapEndian(const uint32_t value) {
  return (value << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) |
         (value >> 24);
}

uint32_t GetPadding(const size_t current, const uint32_t alignment) {
  uint32_t mod = current % alignment;
  return (mod == 0) ? 0 : alignment - mod;
}

bool CheckKtxMagic(const uint8_t* const magic) {
  return (memcmp(magic, kKtxFileMagic, sizeof(kKtxFileMagic)) == 0);
}

KtxImage::ErrorCode ValidateKtxHeader(KtxDirectImage::KtxHeader* header,
                                      bool* reverse_endian) {
  if (!CheckKtxMagic(header->identifier)) {
    std::cerr << "No magic!\n";
    return KtxImage::kFormatError;
  }

  *reverse_endian = (header->endianness == kReverseEndian);
  if (*reverse_endian) {
    KTX_texture_info& info = header->texture_info;
    info.glType = SwapEndian(info.glType);
    info.glTypeSize = SwapEndian(info.glTypeSize);
    info.glFormat = SwapEndian(info.glFormat);
    info.glInternalFormat = SwapEndian(info.glInternalFormat);
    info.glBaseInternalFormat = SwapEndian(info.glBaseInternalFormat);
    info.pixelWidth = SwapEndian(info.pixelWidth);
    info.pixelHeight = SwapEndian(info.pixelHeight);
    info.pixelDepth = SwapEndian(info.pixelDepth);
    info.numberOfArrayElements = SwapEndian(info.numberOfArrayElements);
    info.numberOfFaces = SwapEndian(info.numberOfFaces);
    info.numberOfMipmapLevels = SwapEndian(info.numberOfMipmapLevels);
    header->bytes_of_key_value_data =
        SwapEndian(header->bytes_of_key_value_data);
    // Set the endianness
    header->endianness = kKtxNativeEndian;
  }
  // TODO(gavindodd): handle array textures
  if (header->texture_info.numberOfArrayElements > 0) {
    std::cerr << "Array textures are not supported yet\n";
    return KtxImage::kFormatError;
  }
  return KtxImage::kOK;
}

}  // namespace

KtxImage::KeyValueData KtxDirectImage::ReadKtxHashTable(const uint8_t* data,
                                                        uint32_t data_len,
                                                        bool reverse_endian) {
  // KTX hash table entry format
  // uint_32 key_value_data_size;
  // char null_terminated_string[];
  // uint8_t data[key_value_data_size - (strlen(null_terminated_string) + 1)

  KeyValueData table;

  const uint8_t* data_end = data + data_len;
  while (data < data_end) {
    uint32_t key_value_data_size = *reinterpret_cast<const uint32_t*>(data);
    if (reverse_endian) {
      key_value_data_size = SwapEndian(key_value_data_size);
    }
    data += sizeof(uint32_t);
    const uint8_t* key_value_data_end = data + key_value_data_size;
    // malformed data - source data overflow
    if (key_value_data_end > data_end) {
      table.clear();
      return table;
    }
    const char* key = reinterpret_cast<const char*>(data);
    size_t key_len = strnlen(key, key_value_data_size);
    data += key_len + 1;
    // malformed data - key value data overflow
    // TODO(gavindodd): is 0 length value valid?
    if (data > key_value_data_end) {
      table.clear();
      return table;
    }
    table[key] = std::vector<uint8_t>(data, key_value_data_end);
    // align data to 4 byte boundary
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.15
    uint32_t padding = GetPadding(
        reinterpret_cast<const uintptr_t>(key_value_data_end), kValuePadding);
    data = key_value_data_end + padding;
  }
  return table;
}

KtxImage::ErrorCode KtxDirectImage::Open(const std::string& filename,
                                         std::unique_ptr<KtxImage>* image_out) {
  // It would be nice if third_party/ktx could do most of this work but
  // it does not expose a way to load a file into memory, only directly
  // to a GL texture
  DCHECK(image_out != nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  std::ifstream ktx_file(filename, std::ios::binary);
  if (!ktx_file.is_open()) {
    const char* error = strerror(errno);
    std::cerr << "ktx::Open Could not open " << filename << "\n";
    std::cerr << error << std::endl;
    return kFileOpenError;
  }
  std::cerr << "ktx::Open reading " << filename << "\n";
  KtxHeader header;
  ktx_file.read(reinterpret_cast<char*>(&header), sizeof(header));
  if (!ktx_file) {
    std::cerr << "Could not read file header!\n";
    ktx_file.close();
    return kFileReadError;
  }
  bool reverse_endian = false;
  KtxImage::ErrorCode result = ValidateKtxHeader(&header, &reverse_endian);
  if (result != kOK) {
    ktx_file.close();
    return kFormatError;
  }

  // load the key/value data
  KeyValueData key_value_data;
  if (header.bytes_of_key_value_data > 0) {
    // Key/value data immediately follows the header.
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.12
    uint8_t* key_value_binary_data =
        new uint8_t[header.bytes_of_key_value_data];
    ktx_file.read(reinterpret_cast<char*>(key_value_binary_data),
                  header.bytes_of_key_value_data);
    if (!ktx_file) {
      std::cerr << "Could not read file key/value data!\n";
      ktx_file.close();
      delete[] key_value_binary_data;
      return kFileReadError;
    }
    key_value_data = ReadKtxHashTable(
        key_value_binary_data, header.bytes_of_key_value_data, reverse_endian);
    delete[] key_value_binary_data;
    if (key_value_data.empty()) {
      std::cerr << "Could not parse file key/value data!\n";
      ktx_file.close();
      return kFileReadError;
    }
  }

  // Image data immediately follows any key/value data or the header if there
  // are none.
  // Load the image data.
  uint32_t mip_levels = (header.texture_info.numberOfMipmapLevels == 0)
                            ? 1
                            : header.texture_info.numberOfMipmapLevels;
  ImageData data;
  for (int m = 0; m < mip_levels; ++m) {
    uint32_t image_size;
    ktx_file.read(reinterpret_cast<char*>(&image_size), sizeof(image_size));
    if (!ktx_file) {
      std::cerr << "Could not read file current image size!\n";
      ktx_file.close();
      return kFileReadError;
    }
    // TODO(gavindodd): Confirm that the size is stored endian swapped
    // this has not been tested as it is hard to source an endian swapped KTX
    if (reverse_endian) {
      image_size = SwapEndian(image_size);
    }
    for (int f = 0; f < header.texture_info.numberOfFaces; ++f) {
      std::vector<uint8_t> image_data(image_size);
      ktx_file.read(reinterpret_cast<char*>(&image_data[0]), image_size);
      if (!ktx_file) {
        std::cerr << "Could not read file current image!\n";
        ktx_file.close();
        return kCorruptError;
      }
      // TODO(gavindodd): Probably need to endian swap based on
      // header.texture_info.glTypeSize
      DCHECK_EQ(reverse_endian, false);
      data.push_back(std::move(image_data));
      // pad for face
      // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.17
      uint32_t padding = GetPadding(ktx_file.tellg(), kCubePadding);
      ktx_file.seekg(padding, std::ios::cur);
    }
    // pad for mip
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.18
    uint32_t padding = GetPadding(ktx_file.tellg(), kMipPadding);
    ktx_file.seekg(padding, std::ios::cur);
  }

  image_out->reset(new KtxDirectImage(filename, header, key_value_data, &data));
  return kOK;
}

KtxImage::ErrorCode KtxDirectImage::DropMips(int drop_mips) {
  if (image_data_.size() <= drop_mips) {
    std::cerr << "Image only has " << image_data_.size() << " mips, can't drop "
              << drop_mips << std::endl;
    return kBadParameters;
  }

  header_.texture_info.pixelWidth >>= drop_mips;
  header_.texture_info.pixelHeight >>= drop_mips;
  header_.texture_info.numberOfMipmapLevels -= drop_mips;
  // TODO(dmuir):  To make this work for texture arrays or cube maps, we would
  // remove max(faces, array_size) * drop_mips images.
  for (int i = 0; i < drop_mips; ++i) {
    image_data_.erase(image_data_.begin());
    image_info_.erase(image_info_.begin());
  }

  return kOK;
}

KtxImage::ErrorCode KtxDirectImage::Create(
    const uint8_t* data, size_t data_size,
    std::unique_ptr<KtxImage>* image_out) {
  // TODO(gavindodd): try to reduce memory copying
  // It would be nice if third_party/ktx could do most of this work but
  // it does not expose a way to load a file into memory, only directly
  // to a GL texture
  DCHECK(image_out != nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  DCHECK(data != nullptr);
  if (data == nullptr) {
    return kBadParameters;
  }
  if (data_size < sizeof(KtxHeader)) {
    return kBadParameters;
  }
  KtxHeader header;
  std::memcpy(&header, data, sizeof(header));
  bool reverse_endian = false;
  KtxImage::ErrorCode result = ValidateKtxHeader(&header, &reverse_endian);
  if (result != kOK) {
    return kFormatError;
  }
  const uint8_t* data_pos = data + sizeof(header);
  const uint8_t* data_end = data + data_size + 1;

  // Parse the key/value data.
  KeyValueData key_value_data;
  if (header.bytes_of_key_value_data > 0) {
    if (data_pos + header.bytes_of_key_value_data > data_end) {
      return kCorruptError;
    }
    // Key/value data immediately follows the header.
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.12
    const uint8_t* key_value_binary_data = data_pos;
    data_pos += header.bytes_of_key_value_data;
    key_value_data = ReadKtxHashTable(
        key_value_binary_data, header.bytes_of_key_value_data, reverse_endian);
    if (key_value_data.empty()) {
      std::cerr << "Could not parse file key/value data!\n";
      return kCorruptError;
    }
  }

  // Image data immediately follows any key/value data or the header if there
  // are none.
  // Load the image data.
  uint32_t mip_levels = (header.texture_info.numberOfMipmapLevels == 0)
                            ? 1
                            : header.texture_info.numberOfMipmapLevels;
  ImageData image_data;
  for (int m = 0; m < mip_levels; ++m) {
    if (data_pos + sizeof(uint32_t) > data_end) {
      return kCorruptError;
    }
    uint32_t image_size = *reinterpret_cast<const uint32_t*>(data_pos);
    data_pos += sizeof(uint32_t);
    // TODO(gavindodd): Confirm that the size is stored endian swapped
    // this has not been tested as it is hard to source an endian swapped KTX
    if (reverse_endian) {
      image_size = SwapEndian(image_size);
    }
    for (int f = 0; f < header.texture_info.numberOfFaces; ++f) {
      if (data_pos + image_size > data_end) {
        return kCorruptError;
      }
      // TODO(gavindodd): Probably need to endian swap based on
      // header.texture_info.glTypeSize
      DCHECK_EQ(reverse_endian, false);
      std::vector<uint8_t> single_image_data(data_pos, data_pos + image_size);
      data_pos += image_size;
      image_data.push_back(std::move(single_image_data));
      // pad for face
      // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.17
      uint32_t padding = GetPadding(data_pos - data, kCubePadding);
      data_pos += padding;
    }
    // pad for mip
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.18
    uint32_t padding = GetPadding(data_pos - data, kMipPadding);
    data_pos += padding;
  }

  image_out->reset(
      new KtxDirectImage("<memory>", header, key_value_data, &image_data));
  return kOK;
}

KtxDirectImage::KtxDirectImage(const std::string& filename,
                               const KtxHeader& header,
                               const KeyValueData& key_value_data,
                               ImageData* image_data)
    : filename_(filename),
      header_(header),
      image_data_(std::move(*image_data)) {
  SetKeyValueData(key_value_data);
  for (auto& image : image_data_) {
    KTX_image_info image_info;
    image_info.data = reinterpret_cast<GLubyte*>(&image[0]);
    image_info.size = image.size();
    AddImageInfo(image_info);
  }
}

KtxDirectImage::~KtxDirectImage() {}

uint32_t KtxDirectImage::GlType() const { return header_.texture_info.glType; }

uint32_t KtxDirectImage::GlTypeSize() const {
  return header_.texture_info.glTypeSize;
}

uint32_t KtxDirectImage::GlFormat() const {
  return header_.texture_info.glFormat;
}

uint32_t KtxDirectImage::GlInternalFormat() const {
  return header_.texture_info.glInternalFormat;
}

uint32_t KtxDirectImage::GlBaseInternalFormat() const {
  return header_.texture_info.glBaseInternalFormat;
}

uint32_t KtxDirectImage::PixelWidth() const {
  return header_.texture_info.pixelWidth;
}

uint32_t KtxDirectImage::PixelHeight() const {
  return header_.texture_info.pixelHeight;
}

uint32_t KtxDirectImage::PixelDepth() const {
  return header_.texture_info.pixelDepth;
}

uint32_t KtxDirectImage::NumberOfArrayElements() const {
  return header_.texture_info.numberOfArrayElements;
}
uint32_t KtxDirectImage::NumberOfFaces() const {
  return header_.texture_info.numberOfFaces;
}
uint32_t KtxDirectImage::NumberOfMipmapLevels() const {
  return header_.texture_info.numberOfMipmapLevels;
}

}  // namespace tool
}  // namespace lull
