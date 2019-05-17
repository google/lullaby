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

#ifndef LULLABY_MODULES_RENDER_TEXTURE_PARAMS_H_
#define LULLABY_MODULES_RENDER_TEXTURE_PARAMS_H_

#include "lullaby/generated/texture_def_generated.h"
#include "lullaby/modules/render/image_data.h"

namespace lull {

/// Simple structure containing information that can be used to create a
/// texture.
struct TextureParams {
  TextureParams() {}

  explicit TextureParams(const TextureDef& def)
      : min_filter(def.min_filter()),
        mag_filter(def.mag_filter()),
        wrap_s(def.wrap_s()),
        wrap_t(def.wrap_t()),
        premultiply_alpha(def.premultiply_alpha()),
        generate_mipmaps(def.generate_mipmaps()),
        is_cubemap(def.target_type() == TextureTargetType_CubeMap),
        is_rgbm(def.is_rgbm()) {}

  explicit TextureParams(const TextureDefT& def)
      : min_filter(def.min_filter),
        mag_filter(def.mag_filter),
        wrap_s(def.wrap_s),
        wrap_t(def.wrap_t),
        premultiply_alpha(def.premultiply_alpha),
        generate_mipmaps(def.generate_mipmaps),
        is_cubemap(def.target_type == TextureTargetType_CubeMap),
        is_rgbm(def.is_rgbm) {}

  ImageData::Format format = ImageData::kInvalid;
  TextureFiltering min_filter = TextureFiltering_NearestMipmapLinear;
  TextureFiltering mag_filter = TextureFiltering_Linear;
  TextureWrap wrap_s = TextureWrap_Repeat;
  TextureWrap wrap_t = TextureWrap_Repeat;
  bool premultiply_alpha = true;
  bool generate_mipmaps = false;
  bool is_cubemap = false;
  bool is_rgbm = false;
};

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_TEXTURE_PARAMS_H_
