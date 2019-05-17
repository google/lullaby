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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_COMPONENT_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_COMPONENT_H_

#include <vector>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/systems/render/next/material.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/systems/render/render_system.h"

namespace lull {

// RenderComponent contains all the data for rendering an Entity.
struct RenderComponent : Component {
  explicit RenderComponent(Entity e) : Component(e) {}

  // The mesh (ie. vertex and index buffers) associated with this component.
  MeshPtr mesh = nullptr;

  // The materials associated with the surfaces of the mesh.  The index of the
  // material corresponds to the submesh index in the mesh.
  std::vector<std::shared_ptr<Material>> materials;

  RenderSortOrder sort_order = 0;
  mathfu::vec4 default_color = {1, 1, 1, 1};

  // Material properties that are set across the component, or before materials
  // have been added, are collected in a default material.
  Material default_material;

  // Callback invoked after every SetUniform().
  RenderSystem::UniformChangedCallback uniform_changed_callback;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_COMPONENT_H_
