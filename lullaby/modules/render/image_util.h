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

#ifndef LULLABY_MODULES_RENDER_IMAGE_UTIL_H_
#define LULLABY_MODULES_RENDER_IMAGE_UTIL_H_

#include <stdint.h>

#include "mathfu/glsl_mappings.h"

namespace lull {

// Converts |size|.x() x |size|.y() RGB pixels at |rgb_ptr| into 4 byte RGBA
// pixels at |out_rgba_ptr|.  The alpha values are set to 255.
void ConvertRgb888ToRgba8888(const uint8_t* rgb_ptr, const mathfu::vec2i& size,
                             uint8_t* out_rgba_ptr);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_IMAGE_UTIL_H_
