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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_RENDERABLE_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_RENDERABLE_H_

#include "lullaby/systems/render/filament/renderer.h"
#include "lullaby/systems/render/filament/mesh.h"
#include "lullaby/systems/render/filament/shader.h"
#include "lullaby/systems/render/filament/texture.h"
#include "lullaby/systems/render/filament/material_data.h"

namespace lull {

class Renderable;
using RenderablePtr = std::shared_ptr<Renderable>;

// Bridges the functionality between the System-layer code (eg. Shaders, Meshes,
// etc.) and the Filament-layer code (eg. Entities, MaterialInstances, etc.)
//
class Renderable {
 public:
  Renderable(filament::Engine* engine);
  ~Renderable();

  Renderable(const Renderable&) = delete;
  Renderable& operator=(const Renderable&) = delete;

  // Sets the shader to be used to render
  void SetShader(ShaderPtr shader);

  // Sets the geometry to be rendered by filament which is a submesh specified
  // by the |index| of the provided |mesh|.
  void SetGeometry(MeshPtr mesh, size_t index);

  // Marks the renderable as visible.
  void Show();

  // Marks the renderable as hidden.
  void Hide();

  // Returns true if the renderable is hidden, false otherwise.
  bool IsHidden() const;

  // Returns true if the renderable has all the necessary components so that
  // it can be rendered by filament, false otherwise.
  bool IsReadyToRender() const;

  // Returns the shading model of the Shader to be used to render.
  const std::string& GetShadingModel() const;

  // Sets a user-requested feature.
  void RequestFeature(HashValue feature);

  // Clears a user-requested feature.
  void ClearFeature(HashValue feature);

  // Returns true if a user-requested feature is set, false otherwise.
  bool IsFeatureRequested(HashValue feature) const;

  // Associates a texture with a given usage.
  void SetTexture(TextureUsageInfo usage, const TexturePtr& texture);

  // Returns the texture associated with the specified usage.
  TexturePtr GetTexture(TextureUsageInfo usage) const;

  // Returns the color associated with the renderable.
  mathfu::vec4 GetColor() const;

  // Sets the color associated with the renderable.
  void SetColor(const mathfu::vec4& color);

  // Stores the data as a uniform.
  template <typename T>
  void SetUniform(HashValue name, ShaderDataType type, Span<T> data);

  // Stores the data as a uniform.
  void SetUniform(HashValue name, ShaderDataType type, Span<uint8_t> data);

  // Reads the data associated with the given uniform into the provided buffer.
  bool ReadUniformData(HashValue name, size_t length, uint8_t* data_out) const;

  // Extracts feature flags from the renderable.
  void ReadFeatureFlags(std::set<HashValue>* flags) const;

  // Extracts environment flags from the renderable.
  void ReadEnvironmentFlags(std::set<HashValue>* flags) const;

  // Attempts to create and add a filament Entity representing this renderable
  // to the specified scene.  If the Entity has already been created, this will
  // only update the transform of the Entity.  If the renderable is no longer
  // valid, then it will be removed from the scene.
  bool PrepareForRendering(filament::Scene* scene,
                           const mathfu::mat4* world_from_entity_matrix);

  // Copies System-layer data from the source Renderable.
  void CopyFrom(const Renderable& rhs);

 private:
  void InitFilamentState();
  void DestroyFilamentState();
  void UpdateTransform(const mathfu::mat4& transform);
  void UpdateMaterialInstance();
  void AddToScene(filament::Scene* scene);
  void RemoveFromScene();
  void BindUniform(HashValue name, const detail::UniformData& uniform);
  void BindTexture(TextureUsageInfo usage, TexturePtr texture);

  filament::Scene* scene_ = nullptr;
  filament::Engine* engine_ = nullptr;
  filament::MaterialInstance* material_instance_ = nullptr;
  Optional<utils::Entity> filament_entity_;
  size_t index_ = 0;
  MeshPtr mesh_;
  ShaderPtr shader_;
  MaterialData data_;
};

template <typename T>
void Renderable::SetUniform(HashValue name, ShaderDataType type, Span<T> data) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const size_t size =
      data.size() * detail::UniformData::ShaderDataTypeToBytesSize(type);
  SetUniform(name, type, {bytes, size});
}

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_RENDERABLE_H_
