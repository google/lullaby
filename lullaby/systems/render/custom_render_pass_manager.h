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

#ifndef LULLABY_SYSTEMS_RENDER_CUSTOM_RENDER_PASS_MANAGER_H_
#define LULLABY_SYSTEMS_RENDER_CUSTOM_RENDER_PASS_MANAGER_H_

#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

// Viewport range settings.
struct CustomRenderPassRanges {
  // Width of the viewport in world space.
  float view_width_world = 1.0f;

  // Height of the viewport in world space.
  float view_height_world = 1.0f;

  // Minimum depth (z) of the view volume, in world units.
  float depth_min_world = 1.0f;

  // Maximum depth (z) of the view volume, in world units.
  float depth_max_world = 2.0f;
};

// Contains setup data needed to create new custom render pass.
struct CustomRenderPassSetup {
  // Creates new render target rather than writing to existing one.
  static constexpr HashValue kNewRenderTarget = 0;
  // Illegal value for a custom render pass identifier.
  static constexpr HashValue kInvalidPassId = 0;

  // Pass ID, e.g.: ConstHash("ShadowMap").
  HashValue id = kInvalidPassId;

  // Entity to use as the camera (pos + dir).
  Entity camera = kNullEntity;  // Identity.

  // Render Target to use (kNewRenderTarget to create new one).
  HashValue render_target_id = kNewRenderTarget;

  // Whether to provide mipmaps in output render target.
  bool build_mipmap = false;

  // Viewport range settings.
  CustomRenderPassRanges ranges;

  // General render target info.
  TextureFormat color_format = TextureFormat_RGBA8;
  DepthStencilFormat depth_stencil_format = DepthStencilFormat_None;
  Bits clear_flags = 0;  // No clearing by default.
  mathfu::vec4 clear_color = mathfu::kZeros4f;
  mathfu::vec2i resolution = mathfu::kZeros2i;

  // RenderState (depth, blend, etc.) for this pass.
  fplbase::RenderState render_state;

  // Shader for this pass.
  ShaderPtr shader = nullptr;

  // Shader used for rigid (unskinned) meshes.  If null, defaults to 'shader'.
  ShaderPtr rigid_shader = nullptr;
};

// Manages creation, configuration, and rendering of custom render passes, and
// provides access to the results as textures.
class CustomRenderPassManager {
 public:
  explicit CustomRenderPassManager(Registry* registry);
  // To be called per frame to render all enabled custom render passes.
  template <typename PassBeginFn, typename PassEndFn>
  void RenderAllPasses(PassBeginFn pass_begin, PassEndFn pass_end);
  void RenderAllPasses() {
    const auto nop_function = [](HashValue pass_id) {};
    RenderAllPasses(nop_function, nop_function);
  }
  // Adds a new custom render pass to the list of custom render passes to be
  // processed by RenderAllPasses().
  void CreateCustomRenderPass(const CustomRenderPassSetup& setup);
  // Causes given entity to be rendered in the given custom render pass.
  void AddEntityToCustomPass(Entity entity, HashValue pass_id);
  // Causes given entity to be rendered in the given custom render pass with the
  // given shader. If the shader is null, this behaves the same as the above
  // overload. Use hide_submeshes_mask to specify a bitwise mask of submesh
  // indices (below 64) to hide e.g. to hide submesh 0, pass 1.
  void AddEntityToCustomPass(Entity entity, HashValue pass_id,
                             const ShaderPtr& shader,
                             uint64_t hide_submeshes_mask = 0);
  // Causes given entity to be omitted from the given custom render pass.
  void RemoveEntityFromCustomPass(Entity entity, HashValue pass_id);
  // Returns result of render pass as a texture.
  const TexturePtr GetRenderTarget(HashValue pass_id) const;
  // Returns the clip-from-world matrix for the given custom render pass, or
  // identity if pass not found.
  const mathfu::mat4& GetPassClipFromWorld(HashValue pass_id) const;
  // Returns the clip-from-eye matrix for the given custom render pass, or
  // identity if pass not found.
  const mathfu::mat4& GetPassClipFromEye(HashValue pass_id) const;
  // Set viewport pass ranges.
  void SetPassRanges(HashValue pass_id, const CustomRenderPassRanges& ranges);
  // Enables the given render pass. By default, a pass is automatically enabled
  // on on creation.
  void EnablePass(HashValue pass_id) { EnableOrDisablePass(pass_id, true); }
  // Disables the given render pass. Disabled passes are skipped during
  // RenderAllPasses().
  void DisablePass(HashValue pass_id) { EnableOrDisablePass(pass_id, false); }
  // Enable or disable a pass.
  void EnableOrDisablePass(HashValue pass_id, bool enable);

 private:
  DispatcherSystem* dispatcher_system() const {
    return CHECK_NOTNULL(registry_->Get<DispatcherSystem>());
  }

  RenderSystem* render_system() const {
    return CHECK_NOTNULL(registry_->Get<RenderSystem>());
  }

  TransformSystem* transform_system() const {
    return CHECK_NOTNULL(registry_->Get<TransformSystem>());
  }

  // Used internally to represent a custom render pass that has been set up.
  struct CustomRenderPass {
    HashValue id = CustomRenderPassSetup::kInvalidPassId;
    HashValue texture_id = CustomRenderPassSetup::kInvalidPassId;
    Entity camera = kNullEntity;
    RenderView view;
    ShaderPtr shader = nullptr;
    ShaderPtr rigid_shader = nullptr;
    bool build_mipmap = false;
    bool enabled = false;
  };

  static void SetPassRanges(const CustomRenderPassRanges& ranges,
                            CustomRenderPass* pass);
  void UpdateCamera(CustomRenderPass* custom_pass);
  void UpdateCameras();

  CustomRenderPass* FindPass(HashValue pass_id) {
    const auto found = pass_dict_.find(pass_id);
    return found == pass_dict_.end() ? nullptr : &render_passes_[found->second];
  }
  const CustomRenderPass* FindPass(HashValue pass_id) const {
    return const_cast<CustomRenderPassManager*>(this)->FindPass(pass_id);
  }

  Registry* registry_ = nullptr;
  std::vector<CustomRenderPass> render_passes_;
  std::map<HashValue, size_t> pass_dict_;
};

template <typename PassBeginFn, typename PassEndFn>
void CustomRenderPassManager::RenderAllPasses(PassBeginFn pass_begin,
                                              PassEndFn pass_end) {
  UpdateCameras();
  render_system()->BeginRendering();
  const size_t kNumViews = 1;
  for (auto& custom_pass : render_passes_) {
    if (custom_pass.enabled) {
      pass_begin(custom_pass.id);
      render_system()->Render(&custom_pass.view, kNumViews,
                              static_cast<RenderPass>(custom_pass.id));
      pass_end(custom_pass.id);
    }
  }
  render_system()->EndRendering();
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::CustomRenderPassManager);


#endif  // LULLABY_SYSTEMS_RENDER_CUSTOM_RENDER_PASS_MANAGER_H_
