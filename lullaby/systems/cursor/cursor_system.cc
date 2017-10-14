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

#include "lullaby/systems/cursor/cursor_system.h"

#include "lullaby/events/input_events.h"
#include "lullaby/modules/animation_channels/render_channels.h"
#include "lullaby/modules/animation_channels/transform_channels.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/flatbuffers/common_fb_conversions.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/bits.h"
#include "lullaby/util/trace.h"
#include "mathfu/constants.h"

#include "mathfu/io.h"

namespace lull {

const HashValue kRingDiamaterChannelName = Hash("ring-diameter");
const HashValue kCursorDef = Hash("CursorDef");

CursorSystem::Cursor::Cursor(Entity entity)
    : Component(entity),
      no_hit_distance(kDefaultNoHitDistance),
      ring_active_diameter(0.f),
      ring_inactive_diameter(0.f),
      hit_color(mathfu::kZeros4f),
      no_hit_color(mathfu::kZeros4f) {}

CursorSystem::CursorSystem(Registry* registry) : System(registry), cursors_(8) {
  RegisterDef(this, kCursorDef);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void CursorSystem::Initialize() {
  // Only attempt to setup the channel if it will succeed. This lets this
  // system's tests function without the AnimationSystem.
  if (registry_->Get<AnimationSystem>() && registry_->Get<RenderSystem>()) {
    UniformChannel::Setup(registry_, 2, kRingDiamaterChannelName,
                          "ring_diameter", 1);
  } else {
    LOG(ERROR) << "Failed to set up the ring_diameter channel due to missing "
                  "Animation or Render system.";
  }
}

void CursorSystem::Create(Entity entity, HashValue type, const Def* def) {
  CHECK_NE(def, nullptr);

  if (type == kCursorDef) {
    const auto* data = ConvertDef<CursorDef>(def);
    CreateCursor(entity, data);
  } else {
    DCHECK(false) << "Unsupported ComponentDef type: " << type;
  }
}

void CursorSystem::CreateCursor(Entity entity, const CursorDef* data) {
  Cursor* cursor = cursors_.Emplace(entity);
  InputManager::DeviceType device = TranslateInputDeviceType(data->device());
  cursor->device = device;

  if (data->ring_active_diameter() != 0) {
    cursor->ring_active_diameter = data->ring_active_diameter();
  }
  if (data->ring_inactive_diameter() != 0) {
    cursor->ring_inactive_diameter = data->ring_inactive_diameter();
  }
  if (data->no_hit_distance() != 0) {
    cursor->no_hit_distance = data->no_hit_distance();
  }

  MathfuVec4FromFbColor(data->hit_color(), &cursor->hit_color);
  MathfuVec4FromFbColor(data->no_hit_color(), &cursor->no_hit_color);

  const float inner_hole = data->inner_hole();
  const float inner_ring_end = data->inner_ring_end();
  const float inner_ring_thickness = data->inner_ring_thickness();
  const float mid_ring_end = data->mid_ring_end();
  const float mid_ring_opacity = data->mid_ring_opacity();

  // Set some initial uniform values.
  auto* render_system = registry_->Get<RenderSystem>();
  if (render_system) {
    mathfu::vec4 cursor_color = cursor->no_hit_color;
    render_system->SetUniform(entity, "color", &cursor_color[0], 4);
    render_system->SetUniform(entity, "ring_diameter",
                              &cursor->ring_inactive_diameter, 1);
    render_system->SetUniform(entity, "inner_hole", &inner_hole, 1);
    render_system->SetUniform(entity, "inner_ring_end", &inner_ring_end, 1);
    render_system->SetUniform(entity, "inner_ring_thickness",
                              &inner_ring_thickness, 1);
    render_system->SetUniform(entity, "mid_ring_end", &mid_ring_end, 1);
    render_system->SetUniform(entity, "mid_ring_opacity", &mid_ring_opacity, 1);
  }
}

void CursorSystem::Destroy(Entity entity) { cursors_.Destroy(entity); }

// TODO(b/63343249) enable this when InputProcessor's focus detection is
// available.
// void CursorSystem::AdvanceFrame(const Clock::duration& delta_time) {
//   LULLABY_CPU_TRACE_CALL();
//   for (auto& pair : cursors_) {
//     Cursor* cursor = pair.second;
//     const Entity entity = cursor.GetEntity();
//     const bool showing = input_manager->IsConnected(cursor.device);
//     const InputProcessor::FocusState& focus =
//         input_processor->GetFocusState(cursor.device);

//     UpdateCursor(cursor.GetEntity(), showing, focus.target,
//     focus.interactive,
//                  focus.location);
//   }
// }

void CursorSystem::DoNotCallUpdateCursor(Entity entity, bool showing,
                                         Entity target, bool interactive,
                                         const mathfu::vec3& location) {
  UpdateCursor(entity, showing, target, interactive, location);
}

void CursorSystem::UpdateCursor(Entity entity, bool showing, Entity target,
                                bool interactive,
                                const mathfu::vec3& location) {
  Cursor* cursor = cursors_.Get(entity);
  if (!cursor) {
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();

  if (!showing) {
    // Input device isn't connected.  Set the scale to 0 to hide the cursor.
    Sqt sqt;
    sqt.scale = mathfu::kZeros3f;
    transform_system->SetSqt(entity, sqt);
    return;
  }

  auto* input = registry_->Get<InputManager>();

  // Get camera position if there is one
  mathfu::vec3 camera_position = mathfu::kZeros3f;
  if (input->HasPositionDof(InputManager::kHmd)) {
    camera_position = input->GetDofPosition(InputManager::kHmd);
  }

  SetCursorTransform(*cursor, location, camera_position);

  if (interactive != cursor->in_interactive_mode) {
    cursor->in_interactive_mode = interactive;
    const float ring_diameter = interactive ? cursor->ring_active_diameter
                                            : cursor->ring_inactive_diameter;
    auto* render_system = registry_->Get<RenderSystem>();
    auto* animation_system = registry_->Get<AnimationSystem>();
    if (animation_system) {
      const auto kAnimTime = std::chrono::milliseconds(250);
      animation_system->SetTarget(entity, kRingDiamaterChannelName,
                                  &ring_diameter, 1, kAnimTime);
    } else {
      render_system->SetUniform(entity, "ring_diameter", &ring_diameter, 1);
    }

    const float* color_data =
        interactive ? &cursor->hit_color[0] : &cursor->no_hit_color[0];
    render_system->SetUniform(entity, "color", color_data, 4);
  }
}

void CursorSystem::SetCursorTransform(const Cursor& cursor,
                                      const mathfu::vec3& cursor_world_pos,
                                      const mathfu::vec3& camera_world_pos) {
  auto* transform_system = registry_->Get<TransformSystem>();

  Sqt sqt;
  mathfu::vec3 cursor_to_camera = camera_world_pos - cursor_world_pos;

  // Place the cursor at the desired location:
  sqt.translation = cursor_world_pos;

  // Rotate to face the camera.  We want the cursor's +z to point directly
  // at the camera, with a preference for rotating around the y axis in
  // ambiguous cases.
  sqt.rotation = mathfu::quat::RotateFromToWithAxis(
      mathfu::kAxisZ3f, cursor_to_camera, mathfu::kAxisY3f);

  // Scale the cursor to maintain constant apparent size.
  sqt.scale *= cursor_to_camera.Length() / cursor.no_hit_distance;

  transform_system->SetWorldFromEntityMatrix(cursor.GetEntity(),
                                             CalculateTransformMatrix(sqt));
}

Entity CursorSystem::GetCursor(InputManager::DeviceType device) const {
  for (auto& cursor : cursors_) {
    if (cursor.device == device) {
      return cursor.GetEntity();
    }
  }
  return kNullEntity;
}

void CursorSystem::SetNoHitDistance(Entity entity, float distance) {
  Cursor* cursor = cursors_.Get(entity);
  if (cursor) {
    cursor->no_hit_distance = distance;
  }
}

float CursorSystem::GetNoHitDistance(Entity entity) const {
  const Cursor* cursor = cursors_.Get(entity);
  if (cursor) {
    return cursor->no_hit_distance;
  }
  return kDefaultNoHitDistance;
}

}  // namespace lull
