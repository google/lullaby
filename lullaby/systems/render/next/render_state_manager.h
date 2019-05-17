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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_STATE_MANAGER_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_STATE_MANAGER_H_

#include "lullaby/generated/render_state_def_generated.h"

namespace lull {

/// Manages the underlying GL state using lull::RenderStateT and related types.
///
/// This class uses a RenderStateT instance to reflect any changes made to the
/// underlying GL hardware state.  The internally cached RenderState may have
/// NullOpt values which indicates that the actual hardware state is unknown.
class RenderStateManager {
 public:
  RenderStateManager() {}

  RenderStateManager(const RenderStateManager&) = delete;
  RenderStateManager& operator=(const RenderStateManager&) = delete;

  /// Resets the internal cached RenderState.  This function should be called
  /// when the underlying GL state has been modified outside of this class.
  void Reset();

  /// Returns the internally tracked RenderState that is a "reflection" of the
  /// underlying GL hardware state.
  const RenderStateT& GetRenderState() const;

  /// Validates whether the underlying GL hardware state actually matches the
  /// internally tracked state.
  bool Validate() const;

  /// Updates both the GL hardware state and the internally cached RenderState
  /// based on the provided |state|.
  void SetRenderState(const RenderStateT& state);

  /// Sets the alpha test state.
  void SetAlphaTestState(const AlphaTestStateT& state);

  /// Sets the blend state.
  void SetBlendState(const BlendStateT& state);

  /// Sets the color state.
  void SetColorState(const ColorStateT& state);

  /// Sets the cull state.
  void SetCullState(const CullStateT& state);

  /// Sets the depth state.
  void SetDepthState(const DepthStateT& state);

  /// Sets the point state.
  void SetPointState(const PointStateT& state);

  /// Sets the scissor state.
  void SetScissorState(const ScissorStateT& state);

  /// Sets the stencil state.
  void SetStencilState(const StencilStateT& state);

  /// Sets the viewport state.
  void SetViewport(const mathfu::recti& rect);

 private:
  RenderStateT state_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_STATE_MANAGER_H_
