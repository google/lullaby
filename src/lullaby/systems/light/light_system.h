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

#ifndef LULLABY_SYSTEMS_LIGHT_LIGHT_SYSTEM_H_
#define LULLABY_SYSTEMS_LIGHT_LIGHT_SYSTEM_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "lullaby/generated/light_def_generated.h"
#include "lullaby/base/component.h"
#include "lullaby/base/system.h"
#include "lullaby/systems/light/lights.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"

namespace lull {

/// Manages lights and lightable objects.
///
/// This system requires a RenderSystem and TransformSystem to be present.
///
/// Usage:
/// 1. Define objects which should receive light by adding the Lightable
///    component onto them. Make sure they also have Transform and Render
///    components.
///
/// 2. Ensure their render def is using a light enabled shader (you can
///    construct such shaders by using the light.glslh helper. See
///    light_texture.glslv and light_texture.glslf as an example).
///
/// 3. Call LightSystem::AdvanceFrame() in your update loop.
class LightSystem : public System {
 public:
  explicit LightSystem(Registry* registry);

  /// Creates a light or lightable component from a def.
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  /// Remove all light and lightable components associated with an entity.
  void Destroy(Entity entity) override;

  /// Tick LightSystem's logic.
  void AdvanceFrame();

  /// Creates an ambient light.
  ///
  /// @param entity The entity to attach the ambient light component to.
  /// @param group The light group to place the component in.
  /// @param data The ambient light definition for this light.
  void Create(Entity entity, HashValue group, const AmbientLight& data);
  /// Creates a directional light.
  ///
  /// @param entity The entity to attach the directional light component to.
  /// @param group The light group to place the component in.
  /// @param data The directional light definition for this light.
  void Create(Entity entity, HashValue group, const DirectionalLight& data);
  /// Defines a lightable.
  ///
  /// @param entity The entity to attach the lightable component to.
  /// @param group The light group to place the component in.
  /// @param data The lightable definition for this object.
  void Create(Entity entity, HashValue group, const Lightable& data);
  /// Creates a point light.
  ///
  /// @param entity The entity to attach the point light component to.
  /// @param group The light group to place the component in.
  /// @param data The point light definition for this light.
  void Create(Entity entity, HashValue group, const PointLight& data);

  LightSystem(const LightSystem&) = delete;
  LightSystem& operator=(const LightSystem&) = delete;

 private:
  /// Stores arrays of floating point values that will be used to populate
  /// uniform arrays.
  struct UniformData {
    /// Clears all the uniform data stored.
    void Clear();

    /// Adds uniform data for an ambient light.
    void Add(const AmbientLight& light);
    /// Adds uniform data for a directional light.
    void Add(const DirectionalLight& light);
    /// Adds uniform data for a point light.
    void Add(const PointLight& light);
    /// Applies the uniforms to an entity's render component.
    void Apply(RenderSystem* render_system, Entity entity) const;

   private:
    struct Buffer {
      int dimension = 0;
      std::vector<float> data;
    };
    std::unordered_map<std::string, Buffer> buffers;
  };

  /// Helper structure to hold lights and lightables associated together.
  class LightGroup {
   public:
    /// Add an ambient light to the group.
    void AddLight(Entity entity, const AmbientLight& light);
    /// Add a directional light to the group.
    void AddLight(Entity entity, const DirectionalLight& light);
    /// Add a point light to the group.
    void AddLight(Entity entity, const PointLight& light);
    /// Add a lightable to the group.
    void AddLightable(Entity entity, const Lightable& lightable);

    /// Updates the transforms of a light within the group.
    void UpdateLight(TransformSystem* transform_system, Entity entity);

    /// Updates the render system data of objects within this group.
    void Update(RenderSystem* render_system);

    /// Remove an entity from the group.
    void Remove(Entity entity);

    /// Is this group empty?
    bool Empty() const;

   private:
    void UpdateLightable(RenderSystem* render_system, Entity entity);
    void UpdateLightable(RenderSystem* render_system, Entity entity,
                         const Lightable* data);
    mutable bool dirty_ = false;

    std::unordered_map<Entity, AmbientLight> ambients_;
    std::unordered_map<Entity, DirectionalLight> directionals_;
    std::unordered_map<Entity, PointLight> points_;
    std::unordered_map<Entity, Lightable> lightables_;
    std::set<Entity> dirty_lightables_;
  };

  void Create(Entity e, HashValue type, const Def* def) override {}
  void Create(Entity entity, const AmbientLightDef& data);
  void Create(Entity entity, const DirectionalLightDef& data);
  void Create(Entity entity, const LightableDef& data);
  void Create(Entity entity, const PointLightDef& data);

  /// Update the transforms of light objects associated with a set of entities.
  void UpdateLightTransforms(TransformSystem* transform_system,
                             const std::unordered_set<Entity>& entities);

  std::unordered_map<HashValue, LightGroup> groups_;
  std::unordered_map<Entity, HashValue> entity_to_group_map_;
  std::unordered_set<Entity> ambients_;
  std::unordered_set<Entity> directionals_;
  std::unordered_set<Entity> points_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LightSystem);

#endif  // LULLABY_SYSTEMS_LIGHT_LIGHT_SYSTEM_H_
