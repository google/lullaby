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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_SAMPLER_H_
#define REDUX_TOOLS_SCENE_PIPELINE_SAMPLER_H_

#include "redux/tools/scene_pipeline/image.h"
#include "redux/tools/scene_pipeline/types.h"

namespace redux::tool {

// Represents a texture and its sampler.
struct Sampler {
  // The type of filtering to use when sampling the texture.
  enum class Filter {
    kUnspecified,
    kNearest,
    kLinear,
    kNearestMipmapNearest,
    kLinearMipmapNearest,
    kNearestMipmapLinear,
    kLinearMipmapLinear,
  };

  // The type of wrapping to use when sampling the texture.
  enum class WrapMode {
    kUnspecified,
    kClampToEdge,
    kMirroredRepeat,
    kRepeat,
  };

  static constexpr const float4 kRedMask{1.0f, 0.0f, 0.0f, 0.0f};
  static constexpr const float4 kGreenMask{0.0f, 1.0f, 0.0f, 0.0f};
  static constexpr const float4 kBlueMask{0.0f, 0.0f, 1.0f, 0.0f};
  static constexpr const float4 kAlphaMask{0.0f, 0.0f, 0.0f, 1.0f};
  static constexpr const float4 kRgbMask{1.0f, 1.0f, 1.0f, 0.0f};
  static constexpr const float4 kRgbaMask{1.0f, 1.0f, 1.0f, 1.0f};

  // The image data for the texture.
  ImageIndex image_index;

  // The texture coordinate to use for this texture.
  int texcoord = 0;

  // The filtering and wrapping modes to use when sampling the texture.
  Filter min_filter = Filter::kUnspecified;
  Filter mag_filter = Filter::kUnspecified;
  WrapMode wrap_s = WrapMode::kUnspecified;
  WrapMode wrap_t = WrapMode::kUnspecified;

  // The channel(s) in which the texture data is stored. For example, a
  // channel mask of (0, 0, 1, 0) means that the texture data is stored in the
  // green channel.
  float4 channel_mask = kRgbaMask;

  // The scale and bias to apply to the texture when sampling. e.g.
  //    vec4 sampled_value = texture(tex, uv) * scale + biad;
  float4 scale{1, 1, 1, 1};
  float4 bias{0, 0, 0, 0};
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_SAMPLER_H_
