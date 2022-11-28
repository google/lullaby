/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_TOOLS_TEXTURE_PIPELINE_GENERATE_MIPMAPS_H_
#define REDUX_TOOLS_TEXTURE_PIPELINE_GENERATE_MIPMAPS_H_

#include <vector>

#include "redux/modules/graphics/image_data.h"

namespace redux::tool {

// Generates a vector of mipmap levels for the given image.  The top level
// image will also be included in the vector.
std::vector<ImageData> GenerateMipmaps(ImageData image);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_TEXTURE_PIPELINE_GENERATE_MIPMAPS_H_
