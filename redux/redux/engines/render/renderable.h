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

#ifndef REDUX_ENGINES_RENDER_RENDERABLE_H_
#define REDUX_ENGINES_RENDER_RENDERABLE_H_

#include <memory>
#include <optional>

#include "redux/engines/render/mesh.h"
#include "redux/engines/render/shader.h"
#include "redux/engines/render/texture.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/math/matrix.h"

namespace redux {

// Represents an "object" that will be drawn in a scene.
//
// Renderables consist of two main concepts: the shape and the surface. The
// shape of a renderable is defined by a Mesh and its surface is defined by a
// Shader. A Shader will use information from the Mesh as well as any Textures
// or Properties that are set on the renderable to "color-in" the surface of
// renderable as described by its Mesh shape.
//
// A renderable may have multiple parts as defined by the Mesh object. Each
// part can then be individually assigned a Shader and given its own set of
// properties.
//
// Renderables can belong to multiple scenes. See RenderScene and RenderLayer
// for more information.
class Renderable {
 public:
  virtual ~Renderable() = default;

  Renderable(const Renderable&) = delete;
  Renderable& operator=(const Renderable&) = delete;

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

 protected:
  Renderable() = default;
};

using RenderablePtr = std::shared_ptr<Renderable>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_RENDERABLE_H_
