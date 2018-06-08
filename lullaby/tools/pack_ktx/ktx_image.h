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

#ifndef LULLABY_TOOLS_PACK_KTX_KTX_IMAGE_H_
#define LULLABY_TOOLS_PACK_KTX_KTX_IMAGE_H_
#include <iostream>
#include <map>
#include <memory>
#include <vector>

// KTX texture format file packer
//
// KTX is a container format for storing OpenGL texture data
// See
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
// for details on the KTX file format
#include "ktx.h"
// TODO(gavindodd): include this for ByteArray
// it drags in a bunch of cruft I don't want to deal with right now
// #include "lullaby/util/common_types.h"

namespace lull {
// Copied from common_types.h
using ByteArray = std::vector<uint8_t>;
namespace tool {

// Base class for all ktx handlers.
// Cannot create a valid image
class KtxImage {
 public:
  enum ErrorCode {
    kOK = 0,
    kUnexpectedError,  // Something strange happened
    kBadParameters,    // Invalid parameters passed
    kFileOpenError,    // Could not open the file
    kFileReadError,    // Could not read the file
    kFileWriteError,   // Could not write the file
    kFormatError,      // Source image is not a supported format
    kCorruptError,     // Source image has invalid data
    kBadMip,           // one of the mips did not match the others
    kBadFace,          // one of the faces did not match the others
  };

  typedef std::unique_ptr<KtxImage> ImagePtr;
  typedef std::map<std::string, std::vector<uint8_t>> KeyValueData;
  typedef ErrorCode OpenImage(const std::string& filename, ImagePtr* image_out);
  typedef ErrorCode CreateImage(const uint8_t* data, size_t data_size,
                                ImagePtr* image_out);

  static ErrorCode CreateKtxHashTable(const KeyValueData& table,
                                      unsigned char** data_out,
                                      unsigned int* data_len_out);

  KtxImage() : want_mips_(false) {}
  virtual ~KtxImage() {}
  virtual bool Valid() const { return false; }
  bool WriteFile(const std::string& filename);
  ByteArray ToByteArray();
  const std::vector<KTX_image_info>& GetImageInfo() const {
    return image_info_;
  }
  void PrintTextureInfo() const;

  void SetMipMapped() { want_mips_ = true; }

  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.3
  virtual uint32_t GlType() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.4
  virtual uint32_t GlTypeSize() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.5
  virtual uint32_t GlFormat() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.6
  virtual uint32_t GlInternalFormat() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.7
  virtual uint32_t GlBaseInternalFormat() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.8
  virtual uint32_t PixelWidth() const { return 0; }
  virtual uint32_t PixelHeight() const { return 0; }
  virtual uint32_t PixelDepth() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.9
  virtual uint32_t NumberOfArrayElements() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.10
  virtual uint32_t NumberOfFaces() const { return 0; }
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.11
  virtual uint32_t NumberOfMipmapLevels() const { return 0; }

 protected:
  void AddImageInfo(const KTX_image_info& image_info);
  void AddImageInfo(const std::vector<KTX_image_info>& image_info);
  void SetKeyValueData(const char* key, const uint8_t* data,
                       const uint32_t data_len);
  void SetKeyValueData(const KeyValueData& key_value_data);

  // TODO(gavindodd): who owns the uint8_t* in the KTX_image_info
  std::vector<KTX_image_info> image_info_;

 private:
  void FillTextureInfo(KTX_texture_info* texture_info) const;
  KeyValueData key_value_data_;
  // if there are no mips write 0 to numberOfMipmapLevels so they are generated
  // at load time if possible
  // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.11
  bool want_mips_;

  KtxImage(const KtxImage&) = delete;
  void operator=(const KtxImage&) = delete;
};

// Class to store a mipmap KTX image
// A mipmap image is a list of KtxImage that all have the same attributes
// except the width and height of each image in the list is half of the
// previous image
class KtxMipmapImage : public KtxImage {
 public:
  static ErrorCode Open(const std::vector<std::string>& filenames,
                        OpenImage& open_mip_func, ImagePtr* image_out);
  static ErrorCode OpenCubemap(const std::vector<std::string>& filenames,
                               OpenImage& open_face_function,
                               ImagePtr* image_out);
  static ErrorCode Create(std::vector<ImagePtr>* mips, ImagePtr* image_out);

  ~KtxMipmapImage() override {}

  bool Valid() const override { return mips_.size() > 1; }

  uint32_t GlType() const override;
  uint32_t GlTypeSize() const override;
  uint32_t GlFormat() const override;
  uint32_t GlInternalFormat() const override;
  uint32_t GlBaseInternalFormat() const override;
  uint32_t PixelWidth() const override;
  uint32_t PixelHeight() const override;
  uint32_t PixelDepth() const override;
  uint32_t NumberOfArrayElements() const override;
  uint32_t NumberOfFaces() const override;
  uint32_t NumberOfMipmapLevels() const override;

 protected:
  static bool ValidMip(const KtxImage& base, const KtxImage& mip,
                       const int level);
  explicit KtxMipmapImage(std::vector<ImagePtr>* mips);
  std::vector<ImagePtr> mips_;
};

// Class to store a cube map KTX image
// A cube map is a list of 6 single layer images that all have the same
// attributes including matching width and height
class KtxCubemapImage : public KtxImage {
 public:
  static ErrorCode Open(const std::vector<std::string>& filenames,
                        OpenImage& open_face_function, ImagePtr* image_out);
  static ErrorCode Create(std::vector<ImagePtr>* faces, ImagePtr* image_out);

  ~KtxCubemapImage() override {}

  bool Valid() const override {
    return faces_.size() == 6;
  }

  uint32_t GlType() const override;
  uint32_t GlTypeSize() const override;
  uint32_t GlFormat() const override;
  uint32_t GlInternalFormat() const override;
  uint32_t GlBaseInternalFormat() const override;
  uint32_t PixelWidth() const override;
  uint32_t PixelHeight() const override;
  uint32_t PixelDepth() const override;
  uint32_t NumberOfArrayElements() const override;
  uint32_t NumberOfFaces() const override;
  uint32_t NumberOfMipmapLevels() const override;

 private:
  static bool ValidFace(const KtxImage& base, const KtxImage& face);
  explicit KtxCubemapImage(std::vector<ImagePtr>* faces);
  std::vector<ImagePtr> faces_;
};

}  // namespace tool
}  // namespace lull
#endif  // LULLABY_TOOLS_PACK_KTX_KTX_IMAGE_H_
