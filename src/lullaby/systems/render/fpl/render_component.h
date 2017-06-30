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

#ifndef LULLABY_SYSTEMS_RENDER_FPL_RENDER_COMPONENT_H_
#define LULLABY_SYSTEMS_RENDER_FPL_RENDER_COMPONENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lullaby/generated/render_def_generated.h"
#include "lullaby/base/component.h"
#include "lullaby/systems/render/fpl/mesh.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/hash.h"

namespace lull {
namespace detail {

// RenderComponent contains all the data for rendering an Entity using the FPL
// backend.  This is a private class, and should not be used outside of
// render/fpl.
struct RenderComponent : Component {
  explicit RenderComponent(Entity e) : Component(e) {}

  struct UniformData {
    std::string name;
    std::vector<float> values;
    fplbase::UniformHandle location = fplbase::InvalidUniformHandle();
    int dimension = 0;
    int count = 0;
  };
  using UniformMap = std::map<HashValue, UniformData>;

  mathfu::vec4 default_color = mathfu::vec4(1, 1, 1, 1);
  MeshPtr mesh = nullptr;
  std::unique_ptr<MeshData> dynamic_mesh;
  ShaderPtr shader = nullptr;
  std::map<int, TexturePtr> textures;
  UniformMap uniforms;
  RenderPass pass = RenderPass_Main;
  RenderSystem::SortOrder sort_order = 0;
  StencilMode stencil_mode = StencilMode::kDisabled;
  int stencil_value = 0;
  bool hidden = false;
  RenderSystem::Quad quad = RenderSystem::Quad();
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FPL_RENDER_COMPONENT_H_
