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

#include "filament/Engine.h"
#include "utils/Entity.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_shader.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/renderable.h"
#include "redux/modules/base/data_buffer.h"
#include "redux/modules/base/registry.h"

namespace redux {

// Manages multiple filament Entities and MaterialInstances that are used for
// rendering. A Renderable requires a Mesh and a Shader to render.
//
// The Mesh defines the number of parts of the Renderable, each of which is
// assigned a filament Entity.  Each part is also assigned a filament
// MaterialInstance, depending on the Shader variant associated with the part.
//
// Once setup, parts can be individually controlled (e.g. hidden, assigned
// material properties, etc.). In cases where a specific part is not specified,
// the change applies to the whole Renderable.
class FilamentRenderable : public Renderable {
 public:
  explicit FilamentRenderable(Registry* registry);
  ~FilamentRenderable() override;

  // Prepares the renderable for rendering. The transform is used to place the
  // renderable in all scenes to which it belongs.
  void PrepareToRender(const mat4& transform);

  // Enables the renderable (or a part of the renderable) to be rendered.
  void Show(std::optional<HashValue> part = std::nullopt);

  // Disables the renderable (or a part of the renderable) to be rendered.
  void Hide(std::optional<HashValue> part = std::nullopt);

  // Returns true if the renderable (or a part of the renderable) is hidden.
  bool IsHidden(std::optional<HashValue> part = std::nullopt) const;

  // Sets the mesh (i.e. shape) of the renderable.
  void SetMesh(MeshPtr mesh);

  // Returns the mesh for the renderable.
  MeshPtr GetMesh() const;

  // Enables a vertex attributes. All attributes are enabled by default.
  void EnableVertexAttribute(VertexUsage usage);

  // Disables a specific vertex attribute which may affect how the renderable is
  // drawn. For example, disabling a color vertex attribute will prevent the
  // renderable's mesh color from being used when rendering.
  void DisableVertexAttribute(VertexUsage usage);

  // Returns true if the vertex attribute is enabled.
  bool IsVertexAttributeEnabled(VertexUsage usage) const;

  // Sets the shader that will be used to render the surface of the renderable
  // for a specific part.
  void SetShader(ShaderPtr shader,
                 std::optional<HashValue> part = std::nullopt);

  // Assigns a Texture for a given usage on the renderable. Textures are applied
  // to the entirety of the renderable and not to individual parts.
  void SetTexture(TextureUsage usage, const TexturePtr& texture);

  // Returns the Texture that was set for a given usage on the renderable.
  TexturePtr GetTexture(TextureUsage usage) const;

  // Assigns a specific value to a material property with the given `name` which
  // can be used by the shader when drawing the renderable. The shader will
  // interpret the property `data` based on the material property `type`.
  // Properties can be assigned to individual `part`s or the entire renderable.
  void SetProperty(HashValue name, MaterialPropertyType type,
                   absl::Span<const std::byte> data);
  void SetProperty(HashValue name, HashValue part, MaterialPropertyType type,
                   absl::Span<const std::byte> data);

  // Adds the renderable to a filament scene.
  void AddToScene(FilamentRenderScene* scene) const;

  // Removes the renderable from a filament scene.
  void RemoveFromScene(FilamentRenderScene* scene) const;

 private:
  struct PartInstance {
    utils::Entity fentity;
    FilamentShader::VariantId variant_id = FilamentShader::kInvalidVariant;
    FilamentResourcePtr<filament::MaterialInstance> finstance;
    absl::flat_hash_map<HashValue, int> property_generations;
    ShaderPtr shader;
  };

  struct MaterialProperty {
    MaterialPropertyType type = MaterialPropertyType::Invalid;
    DataBuffer data;
    TexturePtr texture;
    int generation = 0;
  };

  struct Material {
    ShaderPtr shader;
    bool visible = true;
    absl::btree_set<HashValue> features;
    absl::flat_hash_map<HashValue, MaterialProperty> properties;
  };

  void CreateParts();
  void DestroyParts();
  void RebuildConditions();
  void ReacquireInstance(HashValue part);
  bool ApplyProperties(HashValue name, PartInstance& part);
  const MaterialProperty* GetMaterialProperty(HashValue part,
                                              HashValue name) const;

  filament::Engine* fengine_ = nullptr;

  MeshPtr mesh_;
  absl::btree_set<HashValue> conditions_;
  absl::flat_hash_set<VertexUsage> disabled_vertices_;
  absl::flat_hash_map<HashValue, Material> materials_;
  absl::flat_hash_map<HashValue, PartInstance> parts_;
  mutable absl::flat_hash_set<filament::Scene*> scenes_;
  bool is_skinned_ = false;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDERABLE_H_
