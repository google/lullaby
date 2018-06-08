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

#include "lullaby/tools/pack_ktx/ktx_image.h"

#include <string.h>
#include <algorithm>
#include <fstream>
#include <map>

#include "lullaby/util/logging.h"

namespace lull {
namespace tool {

namespace {
void PrintKtxTextureInfo(const KTX_texture_info& texture_info) {
  std::cerr << "GL Type: 0x" << std::hex << texture_info.glType << "\n";
  std::cerr << "GL TypeSize: " << texture_info.glTypeSize << "\n";
  std::cerr << "GL Format: 0x" << std::hex << texture_info.glFormat << "\n";
  std::cerr << "GL Internal Format: 0x" << std::hex
            << texture_info.glInternalFormat << "\n";
  std::cerr << "GL Base Internal Format: 0x" << std::hex
            << texture_info.glBaseInternalFormat << "\n";
  std::cerr << "Pixel Width: " << texture_info.pixelWidth << "\n";
  std::cerr << "Pixel Height: " << texture_info.pixelHeight << "\n";
  std::cerr << "Pixel Depth: " << texture_info.pixelDepth << "\n";
  std::cerr << "Number of Array Elements: "
            << texture_info.numberOfArrayElements << "\n";
  std::cerr << "Number of Faces: " << texture_info.numberOfFaces << "\n";
  std::cerr << "Number of Mipmap Levels: " << texture_info.numberOfMipmapLevels
            << "\n";
}

}  // namespace

// This uses ktx library functions that allocate memory
// it is the callers responsibility to free the pointer returned in data_out
KtxImage::ErrorCode KtxImage::CreateKtxHashTable(const KeyValueData& table,
                                                 unsigned char** data_out,
                                                 unsigned int* data_len_out) {
  if ((data_len_out == nullptr) || (data_out == nullptr)) {
    return KtxImage::kBadParameters;
  }
  *data_len_out = 0;
  *data_out = nullptr;
  if (table.empty()) {
    return KtxImage::kOK;
  }

  KTX_error_code result;
  KTX_hash_table ktx_table = ktxHashTable_Create();

  for (const auto& entry : table) {
    result = ktxHashTable_AddKVPair(ktx_table, entry.first.c_str(),
                                    entry.second.size(), &entry.second[0]);
    if (KTX_SUCCESS != result) {
      ktxHashTable_Destroy(ktx_table);
      return KtxImage::kUnexpectedError;
    }
  }
  result = ktxHashTable_Serialize(ktx_table, data_len_out, data_out);
  if (KTX_SUCCESS != result) {
    ktxHashTable_Destroy(ktx_table);
    free(*data_out);
    *data_len_out = 0;
    *data_out = nullptr;
    return KtxImage::kUnexpectedError;
  }

  return KtxImage::kOK;
}

void KtxImage::FillTextureInfo(KTX_texture_info* texture_info) const {
  if (texture_info == nullptr) {
    return;
  }
  texture_info->glType = GlType();
  texture_info->glTypeSize = GlTypeSize();
  texture_info->glFormat = GlFormat();
  texture_info->glInternalFormat = GlInternalFormat();
  texture_info->glBaseInternalFormat = GlBaseInternalFormat();
  texture_info->pixelWidth = PixelWidth();
  texture_info->pixelHeight = PixelHeight();
  texture_info->pixelDepth = PixelDepth();
  texture_info->numberOfArrayElements = NumberOfArrayElements();
  texture_info->numberOfFaces = NumberOfFaces();
  uint32_t mips = NumberOfMipmapLevels();
  texture_info->numberOfMipmapLevels = ((mips == 1) && want_mips_) ? 0 : mips;
}

void KtxImage::PrintTextureInfo() const {
  KTX_texture_info texture_info;
  FillTextureInfo(&texture_info);
  PrintKtxTextureInfo(texture_info);
}

bool KtxImage::WriteFile(const std::string& filename) {
  if (!Valid()) {
    std::cerr << "KTX is invalid - cannot be written\n";
    return false;
  }
  KTX_texture_info texture_info;
  FillTextureInfo(&texture_info);

  if (image_info_.empty()) {
    std::cerr << "No image info for KTX!\n";
    return false;
  }

  // Generate formatted key/value data
  // keys KTX* are reserved
  // Only one reserved key is defined
  // KTX_ORIENTATION_KEY
  unsigned char* key_value_data = nullptr;
  unsigned int key_value_data_len = 0;

  {
    KtxImage::ErrorCode result = CreateKtxHashTable(
        key_value_data_, &key_value_data, &key_value_data_len);
    if (KtxImage::kOK != result) {
      std::cerr << "Could not create key value data for ktx because "
                << std::to_string(result) << "\n";
      free(key_value_data);
      return false;
    }
  }

  {
    std::cerr << "Writing: " << filename << "\n";

    KTX_error_code result =
        ktxWriteKTXN(filename.c_str(), &texture_info, key_value_data_len,
                     key_value_data, image_info_.size(), &image_info_[0]);
    free(key_value_data);
    if (KTX_SUCCESS != result) {
      std::cerr << "Could not write " << filename << " as output ktx because "
                << std::to_string(result) << "\n";
      return false;
    }
  }
  return true;
}

ByteArray KtxImage::ToByteArray() {
  if (!Valid()) {
    std::cerr << "KTX is invalid - cannot be written\n";
    return ByteArray();
  }
  KTX_texture_info texture_info;
  FillTextureInfo(&texture_info);

  if (image_info_.empty()) {
    std::cerr << "No image info for KTX!\n";
    return ByteArray();
  }

  // Generate formatted key/value data
  // keys KTX* are reserved
  // Only one reserved key is defined
  // KTX_ORIENTATION_KEY
  unsigned char* key_value_data = nullptr;
  unsigned int key_value_data_len = 0;

  {
    KtxImage::ErrorCode result = CreateKtxHashTable(
        key_value_data_, &key_value_data, &key_value_data_len);
    if (KtxImage::kOK != result) {
      std::cerr << "Could not create key value data for ktx because "
                << std::to_string(result) << "\n";
      free(key_value_data);
      return ByteArray();
    }
  }

  {
    unsigned char* bytes;
    GLsizei size;
    KTX_error_code result =
        ktxWriteKTXM(&bytes, &size, &texture_info, key_value_data_len,
                     key_value_data, image_info_.size(), &image_info_[0]);
    free(key_value_data);
    if (KTX_SUCCESS != result) {
      std::cerr << "Could not create ktx because " << std::to_string(result)
                << "\n";
      return ByteArray();
    }
    ByteArray ktx_byte_array(bytes, bytes + size);
    free(bytes);
    return ktx_byte_array;
  }
}

void KtxImage::AddImageInfo(const KTX_image_info& image_info) {
  image_info_.push_back(image_info);
}

void KtxImage::AddImageInfo(const std::vector<KTX_image_info>& image_info) {
  image_info_.reserve(image_info_.size() + image_info.size());
  image_info_.insert(image_info_.end(), image_info.begin(), image_info.end());
}

void KtxImage::SetKeyValueData(const char* key, const uint8_t* data,
                               const uint32_t data_len) {
  std::vector<uint8_t> value = std::vector<uint8_t>(data, data + data_len);
  key_value_data_[key] = value;
}

void KtxImage::SetKeyValueData(const KeyValueData& key_value_data) {
  key_value_data_.insert(key_value_data.begin(), key_value_data.end());
}

// Mipmap
KtxImage::ErrorCode KtxMipmapImage::Open(
    const std::vector<std::string>& filenames, OpenImage& open_mip_func,
    ImagePtr* image_out) {
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  if (filenames.size() < 2) {
    return kBadParameters;
  }
  std::vector<ImagePtr> mips;
  for (const std::string& filename : filenames) {
    ImagePtr mip;
    ErrorCode result = open_mip_func(filename, &mip);
    if (result != kOK) {
      return result;
    }
    if (mip == nullptr) {
      return kUnexpectedError;
    }
    // Push the mip before the check to make the code prettier
    mips.push_back(std::move(mip));
    if (!ValidMip(*mips[0], *mips.back(), mips.size() - 1)) {
      return kBadMip;
    }
  }
  image_out->reset(new KtxMipmapImage(&mips));
  return kOK;
}

KtxImage::ErrorCode KtxMipmapImage::OpenCubemap(
    const std::vector<std::string>& filenames, OpenImage& open_face_function,
    ImagePtr* image_out) {
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  if (filenames.size() % 6 != 0) {
    std::cerr << "Must have a multiple of 6 source files for cube map\n";
    return kBadParameters;
  }
  if (filenames.size() < 12) {
    std::cerr << "Must have a multiple at least 2 mips for mipmap\n";
    return kBadParameters;
  }
  std::vector<ImagePtr> mips;
  for (size_t i = 0; i < filenames.size(); i += 6) {
    std::vector<std::string> mip_files;
    for (size_t j = i; j < i + 6; ++j) {
      mip_files.push_back(filenames[j]);
    }
    ImagePtr mip;
    ErrorCode result =
        KtxCubemapImage::Open(mip_files, open_face_function, &mip);
    if ((result != kOK) || (mip == nullptr)) {
      std::cerr << "Could not open images as valid cube map\n";
      return result;
    }
    mips.push_back(std::move(mip));
  }
  ErrorCode result = KtxMipmapImage::Create(&mips, image_out);
  if ((result != kOK) || ((*image_out) == nullptr)) {
    std::cerr << "Could not open images as valid cube map\n";
    return result;
  }
  return kOK;
}

KtxImage::ErrorCode KtxMipmapImage::Create(std::vector<ImagePtr>* mips,
                                           ImagePtr* image_out) {
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    LOG(ERROR) << "Missing image out";
    return kBadParameters;
  }
  if ((mips == nullptr) || (mips->size() < 2)) {
    LOG(ERROR) << "Missing mips";
    return kBadParameters;
  }
  for (size_t i = 0; i < mips->size(); ++i) {
    if ((*mips)[i] == nullptr) {
      LOG(ERROR) << "Missing mip " << i;
      return kBadParameters;
    }
    if (!ValidMip(*(*mips)[0], *(*mips)[i], i)) {
      LOG(ERROR) << "Mip " << i << " does not match mip 0";
      return kBadMip;
    }
  }
  image_out->reset(new KtxMipmapImage(mips));
  return kOK;
}

bool KtxMipmapImage::ValidMip(const KtxImage& base, const KtxImage& mip,
                              const int level) {
  bool valid = mip.Valid();
  valid &= (mip.GlType() == base.GlType());
  valid &= (mip.GlTypeSize() == base.GlTypeSize());
  valid &= (mip.GlFormat() == base.GlFormat());
  valid &= (mip.GlInternalFormat() == base.GlInternalFormat());
  valid &= (mip.GlBaseInternalFormat() == base.GlBaseInternalFormat());
  uint32_t mip_width = base.PixelWidth() >> level;
  uint32_t mip_height = base.PixelHeight() >> level;
  if ((mip_width == 0) && (mip_height == 0)) {
    // Beyond minimum mip.
    return false;
  }
  mip_width = std::max(mip_width, 1u);
  mip_height = std::max(mip_height, 1u);
  valid &= (mip.PixelWidth() == mip_width);
  valid &= (mip.PixelHeight() == mip_height);
  // Cannot mip 3D textures
  valid &= (mip.PixelDepth() == 0);
  valid &= (mip.NumberOfArrayElements() == base.NumberOfArrayElements());
  valid &= (mip.NumberOfFaces() == base.NumberOfFaces());
  // Cannot merge mipped images
  valid &= (mip.NumberOfMipmapLevels() == 1);
  return valid;
}

KtxMipmapImage::KtxMipmapImage(std::vector<ImagePtr>* mips) {
  DCHECK_NE(mips, nullptr);
  if (mips != nullptr) {
    mips_ = std::move(*mips);
    DCHECK_GT(mips_.size(), 1);
    for (const auto& mip : mips_) {
      AddImageInfo(mip->GetImageInfo());
    }
  }
}

// All pass through to mips_[0] except for mipmaps levels
uint32_t KtxMipmapImage::GlType() const { return mips_[0]->GlType(); }
uint32_t KtxMipmapImage::GlTypeSize() const { return mips_[0]->GlTypeSize(); }
uint32_t KtxMipmapImage::GlFormat() const { return mips_[0]->GlFormat(); }
uint32_t KtxMipmapImage::GlInternalFormat() const {
  return mips_[0]->GlInternalFormat();
}
uint32_t KtxMipmapImage::GlBaseInternalFormat() const {
  return mips_[0]->GlBaseInternalFormat();
}
uint32_t KtxMipmapImage::PixelWidth() const { return mips_[0]->PixelWidth(); }
uint32_t KtxMipmapImage::PixelHeight() const { return mips_[0]->PixelHeight(); }
uint32_t KtxMipmapImage::PixelDepth() const { return mips_[0]->PixelDepth(); }
uint32_t KtxMipmapImage::NumberOfArrayElements() const {
  return mips_[0]->NumberOfArrayElements();
}
uint32_t KtxMipmapImage::NumberOfFaces() const {
  return mips_[0]->NumberOfFaces();
}
uint32_t KtxMipmapImage::NumberOfMipmapLevels() const { return mips_.size(); }

// Cubemap
KtxImage::ErrorCode KtxCubemapImage::Open(
    const std::vector<std::string>& filenames, OpenImage& open_face_function,
    ImagePtr* image_out) {
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  if (filenames.size() != 6) {
    return kBadParameters;
  }
  std::vector<ImagePtr> faces;
  for (const std::string& filename : filenames) {
    ImagePtr face;
    ErrorCode result = open_face_function(filename, &face);
    if (result != kOK) {
      return result;
    }
    if (face == nullptr) {
      return kUnexpectedError;
    }
    // Push the mip before the check to make the code prettier
    faces.push_back(std::move(face));
    if (!ValidFace(*faces[0], *faces.back())) {
      return kBadFace;
    }
  }
  image_out->reset(new KtxCubemapImage(&faces));
  return kOK;
}

KtxImage::ErrorCode KtxCubemapImage::Create(std::vector<ImagePtr>* faces,
                                            ImagePtr* image_out) {
  DCHECK_NE(image_out, nullptr);
  if (image_out == nullptr) {
    return kBadParameters;
  }
  if ((faces == nullptr) || (faces->size() != 6)) {
    return kBadParameters;
  }
  for (size_t i = 0; i < faces->size(); ++i) {
    if ((*faces)[i] == nullptr) {
      return kBadParameters;
    }
    if (!ValidFace(*(*faces)[0], *(*faces)[i])) {
      return kBadFace;
    }
  }
  image_out->reset(new KtxCubemapImage(faces));
  return kOK;
}

bool KtxCubemapImage::ValidFace(const KtxImage& base, const KtxImage& face) {
  bool valid = face.Valid();
  valid &= (face.GlType() == base.GlType());
  valid &= (face.GlTypeSize() == base.GlTypeSize());
  valid &= (face.GlFormat() == base.GlFormat());
  valid &= (face.GlInternalFormat() == base.GlInternalFormat());
  valid &= (face.GlBaseInternalFormat() == base.GlBaseInternalFormat());
  valid &= (face.PixelWidth() > 0) && (face.PixelWidth() == base.PixelWidth());
  valid &=
      (face.PixelHeight() > 0) && (face.PixelHeight() == base.PixelHeight());
  // Cube maps must be made from images containing a single layer
  valid &= (face.PixelDepth() == 0);
  valid &= (face.NumberOfArrayElements() == 0);
  valid &= (face.NumberOfFaces() == 1);
  valid &= (face.NumberOfMipmapLevels() == base.NumberOfMipmapLevels());
  return valid;
}

KtxCubemapImage::KtxCubemapImage(std::vector<ImagePtr>* faces) {
  DCHECK(faces != nullptr);
  if (faces != nullptr) {
    faces_ = std::move(*faces);
    DCHECK_EQ(faces_.size(), 6);
    uint32_t mips = faces_[0]->NumberOfMipmapLevels();
    for (uint32_t mip = 0; mip < mips; ++mip) {
      for (const auto& face : faces_) {
        AddImageInfo(face->GetImageInfo()[mip]);
      }
    }
  }
}

// All pass through to faces_[0] except for NumberOfFaces
uint32_t KtxCubemapImage::GlType() const { return faces_[0]->GlType(); }
uint32_t KtxCubemapImage::GlTypeSize() const { return faces_[0]->GlTypeSize(); }
uint32_t KtxCubemapImage::GlFormat() const { return faces_[0]->GlFormat(); }
uint32_t KtxCubemapImage::GlInternalFormat() const {
  return faces_[0]->GlInternalFormat();
}
uint32_t KtxCubemapImage::GlBaseInternalFormat() const {
  return faces_[0]->GlBaseInternalFormat();
}
uint32_t KtxCubemapImage::PixelWidth() const { return faces_[0]->PixelWidth(); }
uint32_t KtxCubemapImage::PixelHeight() const {
  return faces_[0]->PixelHeight();
}
uint32_t KtxCubemapImage::PixelDepth() const { return faces_[0]->PixelDepth(); }
uint32_t KtxCubemapImage::NumberOfArrayElements() const {
  return faces_[0]->NumberOfArrayElements();
}
uint32_t KtxCubemapImage::NumberOfFaces() const { return 6; }
uint32_t KtxCubemapImage::NumberOfMipmapLevels() const {
  return faces_[0]->NumberOfMipmapLevels();
}

}  // namespace tool
}  // namespace lull
