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

#ifndef LULLABY_TOOLS_TEXTURE_PIPELINE_ENCODE_KTX_H_
#define LULLABY_TOOLS_TEXTURE_PIPELINE_ENCODE_KTX_H_

#include <vector>

#include "lullaby/modules/render/image_data.h"
#include "lullaby/util/common_types.h"
#include "lullaby/tools/texture_pipeline/encode_texture.h"

namespace lull {
namespace tool {

ByteArray EncodeKtx(const ImageData& src);
ByteArray EncodeKtx(const std::vector<ImageData>& srcs,
                    const EncodeInfo& encode_info);


}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_TEXTURE_PIPELINE_ENCODE_KTX_H_
