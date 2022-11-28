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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_TEXTURE_INFO_H_
#define REDUX_TOOLS_MODEL_PIPELINE_TEXTURE_INFO_H_

#include <string>
#include <vector>

#include "redux/modules/base/data_container.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/math/vector.h"

namespace redux::tool {

// Information about a single Texture.
struct TextureInfo {
  TextureInfo() = default;

  // The name for this texture to be used when exported. Even though the
  // material contains a map of a texture name to this TextureInfo, that is the
  // "internal name" of the texture as defined by the source asset.
  std::string export_name;

  // The texture data.
  std::shared_ptr<DataContainer> data;

  vec2i size = {0, 0};
  ImageFormat format = ImageFormat::Invalid;
  TextureUsage usage;
  TextureWrap wrap_s = TextureWrap::Repeat;
  TextureWrap wrap_t = TextureWrap::Repeat;
  bool premultiply_alpha = false;
  bool generate_mipmaps = false;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_TEXTURE_INFO_H_
