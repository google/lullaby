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

#ifndef LULLABY_TOOLS_PACK_KTX_KTX_ASTC_IMAGE_H_
#define LULLABY_TOOLS_PACK_KTX_KTX_ASTC_IMAGE_H_

#include "lullaby/tools/pack_ktx/ktx_image.h"

namespace lull {
namespace tool {

class KtxAstcImage : public KtxImage {
 public:
  struct AstcHeader {
    uint8_t magic[4];    //
    uint8_t blockdim_x;  // x, y, z size in texels
    uint8_t blockdim_y;
    uint8_t blockdim_z;
    uint8_t xsize[3];  // x-size = (xsize[0]<<16) + (xsize[1]<<8) + xsize[2]
    uint8_t ysize[3];  // x-size, y-size and z-size are given in texels
    uint8_t zsize[3];  // block count is inferred
  };

  static ErrorCode Open(const std::string& filename, ImagePtr* image_out);
  static ErrorCode Create(const uint8_t* data, size_t data_size,
                          ImagePtr* image_out);
  static ErrorCode WriteASTC(const KtxImage& image, uint32_t index,
                             const std::string& filename);

  ~KtxAstcImage() override;
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
  KtxAstcImage(const std::string& filename, const AstcHeader& header,
               const uint8_t* data, const uint64_t data_size, bool srgb,
               bool take_ownership);
  std::string filename_;
  AstcHeader header_;
  // TODO(gavindodd): manage this better.
  const uint8_t* data_;
  bool srgb_;
  bool owns_data_;
};

}  // namespace tool
}  // namespace lull
#endif  // LULLABY_TOOLS_PACK_KTX_KTX_ASTC_IMAGE_H_
