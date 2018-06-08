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

#ifndef LULLABY_TOOLS_PACK_KTX_KTX_DIRECT_IMAGE_H_
#define LULLABY_TOOLS_PACK_KTX_KTX_DIRECT_IMAGE_H_

#include <vector>

#include "lullaby/util/logging.h"
#include "lullaby/tools/pack_ktx/ktx_image.h"

namespace lull {
namespace tool {

template <typename T>
std::vector<uint8_t> ToUint8Vector(const T& t) {
  const uint8_t* start = reinterpret_cast<const uint8_t*>(&t);
  const uint8_t* end = start + sizeof(t);
  return std::vector<uint8_t>(start, end);
}

template <typename T>
T FromUint8Vector(std::vector<uint8_t> data) {
  CHECK(data.size() == sizeof(T));
  return *reinterpret_cast<T*>(&data[0]);
}

class KtxDirectImage : public KtxImage {
 public:
  // derived from third_party/ktx/lib/ktxint.h
  struct KtxHeader {
    uint8_t identifier[12];
    uint32_t endianness;
    KTX_texture_info texture_info;
    uint32_t bytes_of_key_value_data;
  };

  static KeyValueData ReadKtxHashTable(const uint8_t* data, uint32_t data_len,
                                       bool reverse_endian);
  static ErrorCode Open(const std::string& filename,
                        std::unique_ptr<KtxImage>* image_out);
  static ErrorCode Create(const uint8_t* data, size_t data_size,
                          ImagePtr* image_out);
  ErrorCode DropMips(int drop_mips);

  ~KtxDirectImage() override;
  bool Valid() const override { return true; }

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
  typedef std::vector<std::vector<uint8_t>> ImageData;

  KtxDirectImage(const std::string& filename, const KtxHeader& header,
                 const KeyValueData& key_value_data, ImageData* image_data);
  std::string filename_;
  KtxHeader header_;
  ImageData image_data_;
};

}  // namespace tool
}  // namespace lull
#endif  // LULLABY_TOOLS_PACK_KTX_KTX_DIRECT_IMAGE_H_
