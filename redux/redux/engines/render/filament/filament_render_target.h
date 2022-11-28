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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_TARGET_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_TARGET_H_

#include "filament/RenderTarget.h"
#include "filament/Texture.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/render_target.h"
#include "redux/engines/render/render_target_factory.h"
#include "redux/modules/base/registry.h"

namespace redux {

// Manages a filament RenderTarget.
class FilamentRenderTarget : public RenderTarget {
 public:
  explicit FilamentRenderTarget(Registry* registry);
  FilamentRenderTarget(Registry* registry, const RenderTargetParams& params);

  // Returns the dimensions of the render target.
  vec2i GetDimensions() const;

  // Returns the format of the underlying color buffer.
  RenderTargetFormat GetRenderTargetFormat() const;

  // Returns the contents of the render buffer.
  ImageData GetFrameBufferData();

  filament::RenderTarget* GetFilamentRenderTarget() {
    return frender_target_.get();
  }

 private:
  void CreateColorAttachment(const RenderTargetParams& params);
  void CreateDepthStencilAttachment(const RenderTargetParams& params);

  Registry* registry_ = nullptr;
  filament::Engine* fengine_ = nullptr;
  FilamentResourcePtr<filament::Texture> fcolor_;
  FilamentResourcePtr<filament::Texture> fdepth_stencil_;
  FilamentResourcePtr<filament::RenderTarget> frender_target_;
  RenderTargetFormat color_format_;
  vec2i dimensions_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_TARGET_H_
