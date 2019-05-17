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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_NEXT_RENDERER_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_NEXT_RENDERER_H_

#include <memory>
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/systems/render/next/material.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/systems/render/next/render_state.h"
#include "lullaby/systems/render/next/render_state_manager.h"
#include "lullaby/systems/render/next/render_target.h"
#include "lullaby/systems/render/next/shader.h"
#include "lullaby/systems/render/render_types.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Provides information about the underlying graphics hardware state and
// capabilities.
class NextRenderer {
 public:
  explicit NextRenderer(Optional<int> gl_major_version_override);
  ~NextRenderer();

  NextRenderer(const NextRenderer&) = delete;
  NextRenderer& operator=(const NextRenderer&) = delete;

  /// Prepares the specified render target (or the default target in none
  /// provided) for rendering.
  void Begin(RenderTarget* render_target = nullptr);

  /// Clears the current render target based on the specified parameters.
  void Clear(const RenderClearParams& clear_params);

  /// Sets the appropriate internal state (render state, shader and uniforms,
  /// etc.) based on the material.
  void ApplyMaterial(const std::shared_ptr<Material>& material);

  /// Renders the submesh with the given transform.
  void Draw(const std::shared_ptr<Mesh>& mesh,
            const mathfu::mat4& world_from_object, int submesh_index = -1);

  void DrawMeshData(const MeshData& mesh_data) {
    mesh_helper_->DrawMeshData(mesh_data);
  }

  /// Cleans up any internal state that was used for rendering.
  void End();

  /// Determines if multiview should be enabled as a feature.
  void EnableMultiview();
  void DisableMultiview();
  bool IsMultiviewEnabled() const;

  /// Returns a set of HashValues describing the environment flags that are
  /// available to the shading model.
  const std::set<HashValue>& GetEnvironmentFlags() const;

  /// Returns the RenderStateManager that can be used for changing the
  /// underlying GL render state.
  RenderStateManager& GetRenderStateManager();
  const RenderStateManager& GetRenderStateManager() const;

  /// Resets all GPU-related state such as bound vertex arrays and samplers.
  void ResetGpuState();

  /// Sets the render state to be used for drawing operations.
  void SetRenderState(const fplbase::RenderState& render_state);

  /// Resets the render state back to a default/clean state.
  void ResetRenderState();

  /// Returns true if the current context is a GLES context (ie. mobile).
  static bool IsGles();

  /// Returns true if the current context supports non-power-of-two textures.
  static bool SupportsTextureNpot();

  /// Returns true if the current context supports vertex arrays.
  static bool SupportsVertexArrays();

  /// Returns true if the current context supports samplers.
  static bool SupportsSamplers();

  /// Returns true if the current context supports ASTC compressed textures.
  static bool SupportsAstc();

  /// Returns true if the current context supports ETC2 compressed textures.
  static bool SupportsEtc2();

  /// Returns true if the current context supports uniform buffer objects.
  static bool SupportsUniformBufferObjects();

  /// Returns the maximum supported number of texture units.
  static int MaxTextureUnits();

  /// Returns the maximum supported shader version.
  static int MaxShaderVersion();

 private:
  std::set<HashValue> environment_flags_;

  std::unique_ptr<MeshHelper> mesh_helper_;
  RenderStateManager render_state_manager_;
  bool multiview_enabled_ = false;
  RenderTarget* render_target_ = nullptr;
  int max_texture_units_ = 0;
};

}  // namespace lull

#endif  /// LULLABY_SYSTEMS_RENDER_NEXT_NEXT_RENDERER_H_
