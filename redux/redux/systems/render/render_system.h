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

#ifndef REDUX_SYSTEMS_RENDER_RENDER_SYSTEM_H_
#define REDUX_SYSTEMS_RENDER_RENDER_SYSTEM_H_

#include "redux/engines/render/render_engine.h"
#include "redux/engines/render/render_target_factory.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/math/math.h"
#include "redux/systems/render/render_def_generated.h"

namespace redux {

template <typename T>
MaterialPropertyType DetermineMaterialPropertyType();

// Draws Entities using the RenderEngine.
//
// Rendering is a complex system that involves many different concepts. Details
// about these concepts can be found in the RenderEngine documentation, but
// here's a quick overview.
//
// Mesh: Describes the geometry or shape of an Entity. A mesh may be divided
// into parts (e.g. the mesh of a car may have a "body" part and separate
// "wheel" parts).
//
// Texture: Textures can be complicated, but its simplest to think of them as
// an image (i.e. a 2D collection of RGBA pixels). The most common use-case for
// Textures is to apply them as a Material (see below) onto the Mesh of an
// Entity to give it a visually interesting surface.
//
// Material: Describes the "surface" of the geometry of an Entity. Simple use
// cases are where you want the Mesh to be a single color, or if you want to
// cover the surface of a Mesh with a Texture image. Materials are controlled
// by either settings Textures or Material Properties on an Entity.
//
// Render Target: A special texture that stores the pixels of the image that
// is being rendered. Think of this as the canvas on which you are drawing/
// rendering the final image.
//
// Light: Decsribes a source of light, such as a sun or a light bulb. In most
// cases, there must be at least one light in order to "see" anything.
//
// Shader: The program that runs on the GPU that computes the color of each
// pixel on the Render Target. At its simplest, you can think of a shader
// program that "projects" the 3D geometry of an Entity onto a 2D surface and
// then calculates the color of each pixel on that surface. The color of the
// final pixel is a combination of the Textures applied to the Mesh and the
// Lights surrounding the Mesh.
//
// Shading Model: A collection of individual Shader progams that basically
// use the same general algorithm in its calculations. For example, a "flat"
// shader may ignore all lighting in a scene, or a "pbr" shader applies
// "Physics-Based Rendering" algorithms to determine how the Lights and
// Materials interact in order to produce a color.
//
// Shading Feature: A flag that helps pick a specific variation of a Shading
// Model algorithm. For example, if your object is semi-transparent, you may
// need to enable the "Alpha" shading feature (since calculating transparencies
// is a rather expensive operation).
//
// Scene: The collection of Entities (i.e. Lights and Meshes+Materials) that
// will be rendered together. Multiple Scenes can be rendered onto the same
// Render Target in arbitrary order and different Scenes can be rendered onto
// different Render Targets. Scenes and RenderTargets can be managed directly
// with the RenderEngine.
class RenderSystem : public System {
 public:
  explicit RenderSystem(Registry* registry);

  void OnRegistryInitialize();

  // Adds the Entity to the given scene.
  void AddToScene(Entity entity, HashValue scene);

  // Removes the Entity from the given scene.
  void RemoveFromScene(Entity entity, HashValue scene);

  // Returns whether all the Entity's GPU resources are loaded for rendering.
  bool IsReadyToRender(Entity entity) const;

  // Entity visibility functions. Applies to the entire Entity across all
  // scenes.
  void Hide(Entity entity);
  void Show(Entity entity);
  bool IsHidden(Entity entity) const;

  // Entity part/submesh visibility functions. Applies across all scenes.
  void Hide(Entity entity, HashValue part);
  void Show(Entity entity, HashValue part);
  bool IsHidden(Entity entity, HashValue part) const;

  // Shading model. Applies to either the entire Entity or a specific part.
  void SetShadingModel(Entity entity, std::string_view model);
  void SetShadingModel(Entity entity, HashValue part, std::string_view model);

  // Enables specific features of the shading model.
  void EnableShadingFeature(Entity entity, HashValue feature);
  void DisableShadingFeature(Entity entity, HashValue feature);

  // Meshes can be assigned only at the Entity level. A mesh may be composed
  // of submeshes which can be individually targetted by some functions.
  void SetMesh(Entity entity, const MeshPtr& mesh);
  void SetMesh(Entity entity, MeshData mesh);
  MeshPtr GetMesh(Entity entity) const;

  // Textures can be set only at the Entity level. To provide different textures
  // across different parts/views/layers, use a custom TextureUsage instead to
  // more accurately describe the purpose.
  void SetTexture(Entity entity, TextureUsage usage, const TexturePtr& texture);
  TexturePtr GetTexture(Entity entity, TextureUsage usage) const;

  // Sets a binding pose for skeletal animations to allow animations to be
  // encoded more efficiently. This pose will be multiplied with the bone
  // transforms to generate the final pose.
  void SetInverseBindPose(Entity entity, absl::Span<const mat4> pose);

  // Provides a mapping from skeletal bone indices to mesh bone weights. This is
  // useful when not all bones in an animation effect a mesh vertex as it allows
  // the matrices to be passed to the shader to be smaller.
  void SetBoneShaderIndices(Entity entity, absl::Span<const uint16_t> indices);

  // Sets arbitrary data on the material of the entity. The name and type of
  // data for the materials is defined by the ShadingModel assigned to the
  // entity.
  void SetMaterialProperty(Entity entity, HashValue name,
                           MaterialPropertyType type,
                           absl::Span<const std::byte> data);

  // Similar to the more "generic" SetMaterialProperty function, this one will
  // automatically determine the MaterialPropertyType based on the type T.
  template <typename T>
  void SetMaterialProperty(Entity entity, HashValue name, const T& value) {
    const auto type = DetermineMaterialPropertyType<T>();
    const size_t size = MaterialPropertyTypeByteSize(type);
    const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
    SetMaterialProperty(entity, name, type, {bytes, size});
  }

  // Similar to the more "generic" SetMaterialProperty function, this one will
  // automatically determine the MaterialPropertyType based on the type T.
  template <typename T>
  void SetMaterialProperty(Entity entity, HashValue name,
                           absl::Span<const T> value) {
    const auto type = DetermineMaterialPropertyType<T>();
    const size_t size = MaterialPropertyTypeByteSize(type) * value.size();
    const std::byte* bytes = reinterpret_cast<const std::byte*>(value.data());
    SetMaterialProperty(entity, name, type, {bytes, size});
  }

  // Sets the Entity's rendering data from the RenderDef.
  void SetFromRenderDef(Entity entity, const RenderDef& def);

  // Synchronizes the light with data from other Systems (e.g. transforms).
  // Note: this function is automatically bound to run before rendering if the
  // choreographer is available.
  void PrepareToRender();

 private:
  struct RenderableComponent {
    // The default inverse bind pose of each bone in the rig. These matrices
    // transform vertices into the space of each bone so that skinning can be
    // applied. They are mulitplied with the individual bone pose transforms to
    // generate the final flattened pose that is sent to skinning shaders.
    std::vector<mat4> inverse_bind_pose;

    // Maps a bone to a given uniform index in the skinning shader. Since not
    // all bones are required for skinning and may just be necessary for
    // computing their descendants' transforms, we only upload bones used for
    // skinning to the shader. Each value in this vector is an index into
    // |pose|, |inverse_bind_pose|, and |root_indices| to get the matrices
    // necessary for the final |shader_pose|.
    std::vector<uint16_t> shader_indices;

    RenderablePtr renderable;
  };

  void OnDestroy(Entity entity) override;

  Renderable& GetRenderable(Entity entity,
                            std::optional<HashValue> scene_name = std::nullopt);
  Renderable* TryGetRenderable(Entity entity);
  const Renderable* TryGetRenderable(Entity entity) const;

  void RemoveRenderableComponent(RenderableComponent* c);

  RenderEngine* engine_ = nullptr;
  absl::flat_hash_map<Entity, RenderableComponent> renderables_;
};

template <typename T>
MaterialPropertyType DetermineMaterialPropertyType() {
  if constexpr (std::is_same_v<T, bool>) {
    return MaterialPropertyType::Boolean;
  } else if constexpr (std::is_same_v<T, int>) {
    return MaterialPropertyType::Int1;
  } else if constexpr (std::is_same_v<T, vec2i>) {
    return MaterialPropertyType::Int2;
  } else if constexpr (std::is_same_v<T, vec3i>) {
    return MaterialPropertyType::Int3;
  } else if constexpr (std::is_same_v<T, vec4i>) {
    return MaterialPropertyType::Int4;
  } else if constexpr (std::is_same_v<T, vec4i>) {
    return MaterialPropertyType::Int4;
  } else if constexpr (std::is_same_v<T, float>) {
    return MaterialPropertyType::Float1;
  } else if constexpr (std::is_same_v<T, vec2>) {
    return MaterialPropertyType::Float2;
  } else if constexpr (std::is_same_v<T, vec3>) {
    return MaterialPropertyType::Float3;
  } else if constexpr (std::is_same_v<T, vec4>) {
    return MaterialPropertyType::Float4;
  } else if constexpr (std::is_same_v<T, mat4>) {
    return MaterialPropertyType::Float4x4;
  } else if constexpr (std::is_same_v<T, Color4f>) {
    return MaterialPropertyType::Float4;
  } else {
    CHECK(false);
    return MaterialPropertyType::Invalid;
  }
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::RenderSystem);

#endif  // REDUX_SYSTEMS_RENDER_RENDER_SYSTEM_H_
