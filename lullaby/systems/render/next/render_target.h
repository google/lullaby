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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_TARGET_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_TARGET_H_

#include "lullaby/modules/render/image_data.h"
#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/systems/render/render_target.h"
#include "lullaby/generated/render_target_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Represents a render target for draw operations.
class RenderTarget {
 public:
  explicit RenderTarget(const RenderTargetCreateParams& create_params);
  ~RenderTarget();

  RenderTarget(const RenderTarget&) = delete;
  RenderTarget& operator=(const RenderTarget&) = delete;

  // Sets the render target as the current target for rendering.
  void Bind() const;
  void Unbind() const;

  // Returns the handle to the texture underlying the render target.
  TextureHnd GetTextureId() const {
    return texture_;
  }

  // Gets the framebuffer data.
  ImageData GetFrameBufferData() const;

 private:
  BufferHnd frame_buffer_;
  BufferHnd depth_buffer_;
  TextureHnd texture_;
  mathfu::vec2i dimensions_ = {0, 0};
  int num_mip_levels_ = 0;
  mutable int prev_frame_buffer_ = 0;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_TARGET_H_
