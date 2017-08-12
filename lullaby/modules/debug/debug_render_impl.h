/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_DEBUG_DEBUG_RENDER_IMPL_H_
#define LULLABY_MODULES_DEBUG_DEBUG_RENDER_IMPL_H_

#include "lullaby/modules/debug/debug_render_draw_interface.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/simple_font.h"
#include "lullaby/util/registry.h"

namespace lull {

class DebugRenderImpl : public debug::DebugRenderDrawInterface {
 public:
  explicit DebugRenderImpl(Registry* registry);

  ~DebugRenderImpl() override;

  void Begin(const RenderSystem::View* views, size_t num_views);
  void End();

  void DrawLine(const mathfu::vec3& start_point, const mathfu::vec3& end_point,
                const Color4ub color) override;

  void DrawText3D(const mathfu::vec3& pos, const Color4ub color,
                  const char* text) override;

  void DrawText2D(const Color4ub color, const char* text) override;

  void DrawBox3D(const mathfu::mat4& world_from_object_matrix, const Aabb& box,
                 Color4ub color) override;

 private:
  Registry* registry_;
  RenderSystem* render_system_;
  const RenderSystem::View* views_;
  size_t num_views_;
  std::unique_ptr<SimpleFont> font_;
  ShaderPtr font_shader_;
  TexturePtr font_texture_;
  ShaderPtr shape_shader_;
  std::vector<VertexPC> verts_;

  DebugRenderImpl(const DebugRenderImpl&) = delete;
  DebugRenderImpl& operator=(const DebugRenderImpl&) = delete;
};
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DebugRenderImpl);

#endif  // LULLABY_MODULES_DEBUG_DEBUG_RENDER_IMPL_H_
