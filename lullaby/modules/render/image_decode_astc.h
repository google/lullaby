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

#ifndef LULLABY_MODULES_RENDER_IMAGE_DECODE_ASTC_H_
#define LULLABY_MODULES_RENDER_IMAGE_DECODE_ASTC_H_

#include "lullaby/modules/render/image_data.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

ImageData DecodeAstc(const mathfu::vec2i& size, const mathfu::vec2i& block,
                     int faces, const uint8_t* data, size_t len);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_IMAGE_DECODE_ASTC_H_
