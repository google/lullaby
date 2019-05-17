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

#include "lullaby/tools/texture_pipeline/encode_astc.h"


namespace lull {
namespace tool {

struct AstcHeader {
  uint8_t magic[4];    // 0x5CA1AB13
  uint8_t blockdim_x;  // x, y, z size in texels
  uint8_t blockdim_y;
  uint8_t blockdim_z;
  uint8_t xsize[3];  // x-size = (xsize[2]<<16) + (xsize[1]<<8) + xsize[0]
  uint8_t ysize[3];
  uint8_t zsize[3];
};

const unsigned int kAstcMagicNumber = 0x5CA1AB13;

ByteArray EncodeAstc(const ImageData& src) {
  AstcEncodeOptions default_options;

  return EncodeAstc(src, default_options);
}

astc_encoder::CompressionSpeed ConvertLullAstcSpeed(AstcEncodeSpeed speed) {
  switch (speed) {
    case AstcEncodeSpeed::kVeryFast:
      return astc_encoder::CompressionSpeed::kVeryFast;
    case AstcEncodeSpeed::kFast:
      return astc_encoder::CompressionSpeed::kFast;
    case AstcEncodeSpeed::kMedium:
      return astc_encoder::CompressionSpeed::kMedium;
    case AstcEncodeSpeed::kThorough:
      return astc_encoder::CompressionSpeed::kThorough;
    case AstcEncodeSpeed::kExhaustive:
      return astc_encoder::CompressionSpeed::kExhaustive;
  }
}

ByteArray EncodeAstc(const ImageData& src, const AstcEncodeOptions& options) {
  std::vector<uint8_t> astc_img;


  return astc_img;
}

}  // namespace tool
}  // namespace lull
