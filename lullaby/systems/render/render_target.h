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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_TARGET_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_TARGET_H_

#include "lullaby/util/math.h"
#include "lullaby/generated/render_target_def_generated.h"
#include "lullaby/generated/texture_def_generated.h"

namespace lull {

struct RenderTargetCreateParams {
  /// The width and height of the render target.
  mathfu::vec2i dimensions;
  /// The format for the render target.
  TextureFormat texture_format;
  /// The depth stencil format for an accompanying depth stencil buffer.
  /// |DepthStencilFormat_None| means no depth stencil buffer will be generated.
  DepthStencilFormat depth_stencil_format;
  /// |num_mip_levels| controls the number of mips the texture will be created
  /// with. A value of 0 will lead to an automatic generation of mips.
  unsigned int num_mip_levels = 1;
  /// The texture minifying function is used whenever the pixel being textured
  /// maps to an area greater than one texture element. There are six defined
  /// minifying functions. Two of them use the nearest one or nearest four
  /// texture elements to compute the texture value. The other four use mipmaps.
  TextureFiltering min_filter = TextureFiltering_Nearest;
  /// The texture magnification function is used when the pixel being textured
  /// maps to an area less than or equal to one texture element.
  TextureFiltering mag_filter = TextureFiltering_Nearest;
  /// Wrap parameter for texture coordinate s.
  TextureWrap wrap_s = TextureWrap_ClampToEdge;
  /// Wrap parameter for texture coordinate t.
  TextureWrap wrap_t = TextureWrap_ClampToEdge;
  /// If true, allow existing render-target to be replaced.  Otherwise, name
  /// reuse is treated as an error.
  bool replace_existing = false;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_TARGET_H_
