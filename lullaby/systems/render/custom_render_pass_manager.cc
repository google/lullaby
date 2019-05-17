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

#include "lullaby/systems/render/custom_render_pass_manager.h"

#include <algorithm>
#include <cmath>

#include "fplbase/glplatform.h"
#include "lullaby/events/render_events.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/systems/render/render_system.h"
#include "mathfu/quaternion.h"
#include "mathfu/vector.h"

namespace lull {

namespace {
const mathfu::mat4 kIdentityMatrix = mathfu::mat4::Identity();
}

CustomRenderPassManager::CustomRenderPassManager(Registry* registry)
    : registry_(registry) {
  registry->RegisterDependency<DispatcherSystem>(this);
  registry->RegisterDependency<RenderSystem>(this);
  registry->RegisterDependency<TransformSystem>(this);
}

void CustomRenderPassManager::CreateCustomRenderPass(
    const CustomRenderPassSetup& setup) {
  CustomRenderPass pass;
  // Create or copy the render target.
  if (setup.render_target_id == CustomRenderPassSetup::kNewRenderTarget) {
    RenderTargetCreateParams create_params;
    create_params.dimensions = setup.resolution;
    create_params.texture_format = setup.color_format;
    create_params.depth_stencil_format = setup.depth_stencil_format;

    create_params.mag_filter = TextureFiltering_Linear;

    if (setup.build_mipmap) {
      create_params.num_mip_levels = 0;  // Generates full mipmap pyramid.
      create_params.min_filter = TextureFiltering_LinearMipmapLinear;
    } else {
      create_params.min_filter = TextureFiltering_Linear;
    }

    render_system()->CreateRenderTarget(setup.id, create_params);

    // Set the render target for the pass.
    render_system()->SetRenderTarget(setup.id, setup.id);
    pass.texture_id = setup.id;
  } else {
    // Set the render target for the pass.
    render_system()->SetRenderTarget(setup.id, setup.render_target_id);
    pass.texture_id = setup.render_target_id;
  }

  // Set the render state for the pass.
  render_system()->SetRenderState(setup.id, setup.render_state);

  // Set the clear params for the pass.
  RenderClearParams clear_params;
  clear_params.clear_options = setup.clear_flags;
  clear_params.color_value = setup.clear_color;
  render_system()->SetClearParams(setup.id, clear_params);

  // Set the sort mode for the pass.
  render_system()->SetSortMode(setup.id,
                               SortMode_AverageSpaceOriginFrontToBack);

  // Create the viewport for rendering the pass.
  pass.id = setup.id;
  pass.camera = setup.camera;
  pass.shader = setup.shader;
  pass.rigid_shader = setup.rigid_shader;
  pass.view.viewport.x = 0;
  pass.view.viewport.y = 0;
  pass.view.dimensions = setup.resolution;

  // Construct the view and projection matrices.
  pass.view.world_from_eye_matrix = mathfu::mat4::Identity();
  pass.view.clip_from_world_matrix = mathfu::mat4::Identity();

  SetPassRanges(setup.ranges, &pass);
  pass.enabled = true;
  pass.build_mipmap = setup.build_mipmap;

  const size_t id = render_passes_.size();
  render_passes_.push_back(std::move(pass));
  pass_dict_[setup.id] = id;
}

const TexturePtr CustomRenderPassManager::GetRenderTarget(
    HashValue pass_id) const {
  const CustomRenderPass* const pass = FindPass(pass_id);
  return pass ? render_system()->GetTexture(pass->texture_id) : nullptr;
}

const mathfu::mat4& CustomRenderPassManager::GetPassClipFromWorld(
    HashValue pass_id) const {
  const CustomRenderPass* const pass = FindPass(pass_id);
  return pass ? pass->view.clip_from_world_matrix : kIdentityMatrix;
}

const mathfu::mat4& CustomRenderPassManager::GetPassClipFromEye(
    HashValue pass_id) const {
  const CustomRenderPass* const pass = FindPass(pass_id);
  return pass ? pass->view.clip_from_eye_matrix : kIdentityMatrix;
}

void CustomRenderPassManager::SetPassRanges(
    HashValue pass_id, const CustomRenderPassRanges& ranges) {
  CustomRenderPass* const pass = FindPass(pass_id);
  if (pass) {
    SetPassRanges(ranges, pass);
  }
}

void CustomRenderPassManager::AddEntityToCustomPass(Entity entity,
                                                    HashValue pass_id) {
  AddEntityToCustomPass(entity, pass_id, nullptr);
}

void CustomRenderPassManager::AddEntityToCustomPass(
    Entity entity, HashValue pass_id, const ShaderPtr& shader,
    uint64_t hide_submeshes_mask) {
  const std::vector<HashValue>& render_passes =
      render_system()->GetRenderPasses(entity);
  const HashValue source_pass = render_passes.empty()
                                    ? render_system()->GetDefaultRenderPass()
                                    : render_passes[0];

  // Verify that the pass exists and get the index.
  const auto pass_iter = pass_dict_.find(pass_id);
  if (pass_iter == pass_dict_.end()) {
    LOG(DFATAL) << "Failed to find pass " << pass_id;
    return;
  }
  const CustomRenderPass& pass = render_passes_[pass_iter->second];
  auto setup = [=]() {
    auto mesh = render_system()->GetMesh({entity, source_pass});
    if (!mesh) {
      return;
    }

    render_system()->Create(entity, pass_id);
    if (shader != nullptr) {
      render_system()->SetShader(entity, pass_id, shader);
    } else {
      bool is_rigid = true;
      const size_t count = GetNumSubmeshes(mesh);
      for (size_t i = 0; i < count; ++i) {
        const VertexFormat format = GetVertexFormat(mesh, i);
        if (format.GetAttributeWithUsage(VertexAttributeUsage_BoneIndices)) {
          is_rigid = false;
          break;
        }
      }
      const ShaderPtr mesh_shader =
          pass.rigid_shader && is_rigid ? pass.rigid_shader : pass.shader;
      render_system()->SetShader(entity, pass_id, mesh_shader);
    }
    render_system()->SetMesh({entity, pass_id}, mesh);

    // Hide the appropriate submeshes.
    int hide_submesh = 0;
    uint64_t mutable_mask = hide_submeshes_mask;
    while (mutable_mask > 0) {
      if (mutable_mask & 1) {
        render_system()->Hide({entity, pass_id, hide_submesh});
      }
      ++hide_submesh;
      mutable_mask >>= 1;
    }
  };

  // Retrieve the mesh from the source pass and set it on the custom pass.
  dispatcher_system()->Connect(
      entity, this,
      [=](const MeshChangedEvent& event) {
        if (event.pass == source_pass) {
          setup();
        }
      });
  setup();
}

void CustomRenderPassManager::EnableOrDisablePass(HashValue pass_id,
                                                  bool enable) {
  const auto found = pass_dict_.find(pass_id);
  if (found == pass_dict_.end()) {
    LOG(DFATAL) << "Unknown render pass: " << pass_id;
    return;
  }
  render_passes_[found->second].enabled = enable;
}

void CustomRenderPassManager::RemoveEntityFromCustomPass(Entity entity,
                                                        HashValue pass_id) {
  // Remove the custom pass from the entity.
  render_system()->Destroy(entity, pass_id);

  // Disconnect the dispatcher.
  dispatcher_system()->Disconnect<MeshChangedEvent>(entity, this);
}

void CustomRenderPassManager::SetPassRanges(
    const CustomRenderPassRanges& ranges, CustomRenderPass* pass) {
  const float half_width = ranges.view_width_world * 0.5f;
  const float half_height = ranges.view_height_world * 0.5f;
  pass->view.clip_from_eye_matrix =
      mathfu::mat4::Ortho(-half_width, half_width, -half_height, half_height,
                          ranges.depth_min_world, ranges.depth_max_world, 1.0f);
}

void CustomRenderPassManager::UpdateCameras() {
  for (auto& custom_pass : render_passes_) {
    UpdateCamera(&custom_pass);
  }
}

void CustomRenderPassManager::UpdateCamera(CustomRenderPass* custom_pass) {
  // Construct the view and projection matrices.
  const mathfu::mat4* entity_world_matrix =
      transform_system()->GetWorldFromEntityMatrix(
          CHECK_NOTNULL(custom_pass)->camera);

  if (entity_world_matrix) {
    custom_pass->view.world_from_eye_matrix = *entity_world_matrix;
  } else {
    custom_pass->view.world_from_eye_matrix = mathfu::mat4::Identity();
  }
  custom_pass->view.clip_from_world_matrix =
      custom_pass->view.clip_from_eye_matrix *
      custom_pass->view.world_from_eye_matrix.Inverse();
}

}  // namespace lull
