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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDERABLE_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDERABLE_H_

#include <cstddef>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"
#include "filament/Box.h"
#include "filament/Engine.h"
#include "utils/Entity.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_shader.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/mesh.h"
#include "redux/engines/render/renderable.h"
#include "redux/engines/render/shader.h"
#include "redux/engines/render/texture.h"
#include "redux/modules/base/data_buffer.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/math/matrix.h"

namespace redux {

// Manages a filament "renderable" Entity and MaterialInstance which is used for
// rendering. A Renderable requires a Mesh and a Shader to render.
class FilamentRenderable : public Renderable {
 public:
  explicit FilamentRenderable(Registry* registry);
  ~FilamentRenderable() override;

  // Prepares the renderable for rendering. The transform is used to place the
  // renderable in all scenes to which it belongs.
  void PrepareToRender(const mat4& transform);

  // Enables the renderable.
  void Show();

  // Disables the renderable from being rendered.
  void Hide();

  // Returns true if the renderable is hidden.
  bool IsHidden() const;

  // Sets the mesh (i.e. shape) of the renderable. Note that a single renderable
  // represents just a single part of a larger mesh. This allows each part to
  // be configured independently for rendering.
  void SetMesh(MeshPtr mesh, size_t part_index);

  // Enables a vertex attributes. All attributes are enabled by default.
  void EnableVertexAttribute(VertexUsage usage);

  // Disables a specific vertex attribute which may affect how the renderable is
  // drawn. For example, disabling a color vertex attribute will prevent the
  // renderable's mesh color from being used when rendering.
  void DisableVertexAttribute(VertexUsage usage);

  // Returns whether or not the given vertex attribute is enabled.
  bool IsVertexAttributeEnabled(VertexUsage usage) const;

  // Sets the shader that will be used to render the surface of the renderable.
  void SetShader(ShaderPtr shader);

  // Assigns a Texture for a given usage on the renderable.
  void SetTexture(TextureUsage usage, const TexturePtr& texture);

  // Returns the Texture that was set for a given usage on the renderable.
  TexturePtr GetTexture(TextureUsage usage) const;

  // Assigns a specific value to a material property with the given `name` which
  // can be used by the shader when drawing the renderable. The shader will
  // interpret the property `data` based on the material property `type`.
  void SetProperty(HashValue name, MaterialPropertyType type,
                   absl::Span<const std::byte> data);

  // Copies properties from the other renderable.
  void InheritProperties(const Renderable& other);

  // Adds the renderable to a filament scene.
  void AddToScene(FilamentRenderScene* scene) const;

  // Removes the renderable from a filament scene.
  void RemoveFromScene(FilamentRenderScene* scene) const;

 private:
  struct MaterialProperty {
    MaterialPropertyType type = MaterialPropertyType::Invalid;
    DataBuffer data;
    TexturePtr texture;
    int generation = 0;
  };

  void CreateFilamentEntity();
  void DestroyFilamentEntity();
  void RebuildConditions();
  void ReacquireInstance();
  bool ApplyProperties();
  const MaterialProperty* GetMaterialProperty(HashValue name) const;

  filament::Engine* fengine_ = nullptr;

  MeshPtr mesh_;
  int part_index_ = 0;
  ShaderPtr shader_;
  FilamentShader::VariantId variant_id_ = FilamentShader::kInvalidVariant;

  absl::btree_set<HashValue> conditions_;
  absl::btree_set<HashValue> features_;
  absl::flat_hash_set<VertexUsage> disabled_vertices_;
  absl::flat_hash_map<HashValue, MaterialProperty> properties_;
  absl::flat_hash_map<HashValue, int> property_generations_;

  utils::Entity fentity_;
  FilamentResourcePtr<filament::MaterialInstance> finstance_;

  mutable absl::flat_hash_set<filament::Scene*> scenes_;
  filament::Box aabb_;
  bool is_skinned_ = false;
  bool visible_ = true;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDERABLE_H_
