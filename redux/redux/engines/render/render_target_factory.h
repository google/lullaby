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

#ifndef REDUX_ENGINES_RENDER_RENDER_TARGET_FACTORY_H_
#define REDUX_ENGINES_RENDER_RENDER_TARGET_FACTORY_H_

#include "redux/engines/render/render_target.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"

namespace redux {

enum class RenderTargetFormat {
  None,
  Red8,
  Rgb8,
  Rgba8,
};

enum class RenderTargetDepthStencilFormat {
  None,
  Depth16,
  Depth24,
  Depth32f,
  Depth24Stencil8,
  Depth32fStencil8,
  Stencil8,
};

struct RenderTargetParams {
  // The width and height of the render target.
  vec2i dimensions = {0, 0};

  // The format for the render target.
  RenderTargetFormat texture_format = RenderTargetFormat::None;

  // The depth stencil format for an accompanying depth stencil buffer.
  // |DepthStencilFormat_None| means no depth stencil buffer will be
  // generated.
  RenderTargetDepthStencilFormat depth_stencil_format =
      RenderTargetDepthStencilFormat::None;

  // |num_mip_levels| controls the number of mips the texture will be created
  // with. A value of 0 will lead to an automatic generation of mips.
  unsigned int num_mip_levels = 1;

  // The texture minifying function is used whenever the pixel being textured
  // maps to an area greater than one texture element. There are six defined
  // minifying functions. Two of them use the nearest one or nearest four
  // texture elements to compute the texture value. The other four use
  // mipmaps.
  TextureFilter min_filter = TextureFilter::Nearest;

  // The texture magnification function is used when the pixel being textured
  // maps to an area less than or equal to one texture element.
  TextureFilter mag_filter = TextureFilter::Nearest;

  // Wrap parameter for texture coordinate s.
  TextureWrap wrap_s = TextureWrap::ClampToEdge;

  // Wrap parameter for texture coordinate t.
  TextureWrap wrap_t = TextureWrap::ClampToEdge;
};

// Creates and manages RenderTarget objects.
class RenderTargetFactory {
 public:
  explicit RenderTargetFactory(Registry* registry);

  RenderTargetFactory(const RenderTargetFactory&) = delete;
  RenderTargetFactory& operator=(const RenderTargetFactory&) = delete;

  // Returns the RenderTarget in the cache associated with |name|, else nullptr.
  RenderTargetPtr GetRenderTarget(HashValue name) const;

  // Releases the cached RenderTarget associated with |name|.
  void ReleaseRenderTarget(HashValue name);

  // Creates a RenderTarget using the specified data.
  RenderTargetPtr CreateRenderTarget(HashValue name,
                                     const RenderTargetParams& params);

 private:
  Registry* registry_ = nullptr;
  ResourceManager<RenderTarget> render_targets_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::RenderTargetFactory);

#endif  // REDUX_ENGINES_RENDER_RENDER_TARGET_FACTORY_H_
