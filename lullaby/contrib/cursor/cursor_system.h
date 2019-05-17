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

#ifndef LULLABY_SYSTEMS_CURSOR_CURSOR_SYSTEM_H_
#define LULLABY_SYSTEMS_CURSOR_CURSOR_SYSTEM_H_

#include <deque>

#include "lullaby/generated/cursor_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_focus.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// The CursorSystem updates the on-screen position and render state of a dot at
/// the end of an input ray, based on where that ray collides with an entity.
/// NOTE: this system is currently a sub-system of reticle_system, pending
/// completion of refactoring reticle_system's target storage and event
/// sending into InputProcessor.
class CursorSystem : public System {
 public:
  static constexpr float kDefaultNoHitDistance = 2.0f;

  explicit CursorSystem(Registry* registry);

  void Initialize() override;

  void Create(Entity entity, HashValue type, const Def* def) override;

  void PostCreateInit(Entity entity, DefType type, const Def* def) override;

  void Destroy(Entity entity) override;

  /// Update the cursor rendering.  This should be called after
  /// StandardInputPipeline::AdvanceFrame() or InputProccessor::UpdateDevice().
  void AdvanceFrame(const Clock::duration& delta_time);

  // DO NOT CALL: This function should only be called by reticle_system, and
  // will be removed when reticle_system starts using InputProcessor.
  void DoNotCallUpdateCursor(Entity entity, bool showing, Entity target,
                             bool interactive, const mathfu::vec3& location);

  /// Sets the distance for the cursor when there is no collision.
  void SetNoHitDistance(Entity entity, float distance);

  /// Gets the distance for the cursor when there is no collision.
  float GetNoHitDistance(Entity entity) const;

  /// Gets the cursor entity that matches |device|.  If multiple match, will
  /// return the first it finds.
  Entity GetCursor(InputManager::DeviceType device) const;

  /// Changes what device the cursor is driven by.
  void SetDevice(Entity entity, InputManager::DeviceType device);

  /// Calculates where the cursor should be, (ignoring any collisions or other
  /// target providers).
  mathfu::vec3 CalculateCursorPosition(InputManager::DeviceType device,
                                       const Ray& collision_ray) const;
  /// Restores the default cursor rendering properties which is defined in the
  /// blueprint.
  void RestoreDefaultCursor(Entity entity);

 private:
  struct CursorUniforms {
    float inner_hole = 0.0f;
    float inner_ring_end = 0.0f;
    float inner_ring_thickness = 0.0f;
    float mid_ring_end = 0.0f;
    float mid_ring_opacity = 0.0f;
  };

  struct Cursor : Component {
    explicit Cursor(Entity entity);
    float no_hit_distance = 0.f;
    float ring_active_diameter = 0.f;
    float ring_inactive_diameter = 0.f;
    mathfu::vec4 hit_color;
    mathfu::vec4 no_hit_color;
    InputManager::DeviceType device;
    bool in_interactive_mode = false;
    CursorUniforms uniforms;
    ShaderPtr default_shader;
  };

  void CreateCursor(Entity entity, const CursorDef* data);

  void UpdateCursor(Entity entity, bool showing, Entity target,
                    bool interactive, const mathfu::vec3& location);

  // Place the cursor at the desired location, rotate it to face the camera,
  // and scale it to maintain constant visual size.
  void SetCursorTransform(const Cursor& cursor,
                          const mathfu::vec3& cursor_world_pos,
                          const mathfu::vec3& camera_world_pos);

  // Set the initial uniform values defined from the blueprint.
  void SetCursorUniforms(Entity entity);

  ComponentPool<Cursor> cursors_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::CursorSystem);

#endif  // LULLABY_SYSTEMS_CURSOR_CURSOR_SYSTEM_H_
