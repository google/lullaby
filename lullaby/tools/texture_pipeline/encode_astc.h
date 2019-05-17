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

#ifndef LULLABY_TOOLS_TEXTURE_PIPELINE_ENCODE_ASTC_H_
#define LULLABY_TOOLS_TEXTURE_PIPELINE_ENCODE_ASTC_H_

#include "lullaby/modules/render/image_data.h"
#include "lullaby/util/common_types.h"

namespace lull {
namespace tool {

enum AstcEncodeSpeed {
  kVeryFast,
  kFast,
  kMedium,
  kThorough,
  kExhaustive,
};

struct AstcEncodeOptions {
  int block_width = 6;
  int block_height = 6;
  AstcEncodeSpeed encode_speed = kMedium;
};

ByteArray EncodeAstc(const ImageData& src);
ByteArray EncodeAstc(const ImageData& src, const AstcEncodeOptions& options);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_TEXTURE_PIPELINE_ENCODE_ASTC_H_
