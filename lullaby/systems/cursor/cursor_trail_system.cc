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

#include "lullaby/systems/cursor/cursor_trail_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/cursor/cursor_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"
#include "mathfu/constants.h"

namespace lull {

const HashValue kCursorTrailDef = Hash("CursorTrailDef");
const int kNumVerticesPerTrailQuad = 4;
const int kNumIndicesPerTrailQuad = 6;
const float kMaxDeltaTime = 0.05f;

CursorTrailSystem::CursorTrail::CursorTrail(Entity entity)
    : Component(entity),
      default_color(mathfu::kZeros4f),
      max_trail_length(0),
      average_trail_length(0),
      trail_length(0),
      curve_samples(0),
      quad_size(0.f),
      average_speed(0.f) {}

CursorTrailSystem::CursorTrailSystem(Registry* registry)
    : System(registry), cursor_trails_(8) {
  RegisterDef(this, kCursorTrailDef);
  RegisterDependency<CursorSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void CursorTrailSystem::Create(Entity entity, HashValue type, const Def* def) {
  CHECK_NE(def, nullptr);

  if (type == kCursorTrailDef) {
    const auto* data = ConvertDef<CursorTrailDef>(def);
    CreateCursorTrail(entity, data);
  } else {
    DCHECK(false) << "Unsupported ComponentDef type: " << type;
  }
}

void CursorTrailSystem::CreateCursorTrail(Entity entity,
                                          const CursorTrailDef* data) {
  auto cursor_trail = cursor_trails_.Emplace(entity);
  cursor_trail->curve_samples = data->curve_samples();
  cursor_trail->max_trail_length = data->max_trail_length();
  cursor_trail->average_trail_length = data->average_trail_length();
  cursor_trail->average_speed = data->average_speed();
  cursor_trail->quad_size = data->quad_size();
  MathfuVec4FromFbColor(data->default_color(), &cursor_trail->default_color);

  cursor_trail->position_history.resize(4, mathfu::kZeros3f);
  cursor_trail->trail_positions.resize(cursor_trail->average_trail_length,
                                       mathfu::kZeros3f);
}

void CursorTrailSystem::Destroy(Entity entity) {
  cursor_trails_.Destroy(entity);
}

void CursorTrailSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  auto* transform_system = registry_->Get<TransformSystem>();
  cursor_trails_.ForEach([this, transform_system,
                          delta_time](CursorTrail& cursor_trail) {
    Sqt sqt = *(transform_system->GetSqt(cursor_trail.GetEntity()));

    // Save the positions of the current frame and last three frames in order to
    // create our cubic interpolated curve.
    if (cursor_trail.position_history.size() ==
        static_cast<uint16_t>(cursor_trail.curve_samples)) {
      cursor_trail.position_history.pop_front();
    }
    cursor_trail.position_history.push_back(sqt.translation);

    // If the last frame was more than kMaxDeltaTime ago, don't draw a trail
    if (SecondsFromDuration(delta_time) > kMaxDeltaTime) {
      cursor_trail.trail_length = 1;
    } else {
      // Attenuate the number of cursors drawn based on the speed of the current
      // cursor
      const float length =
          (cursor_trail.position_history[3] - cursor_trail.position_history[2])
              .Length();
      const float length_over_speed = length / cursor_trail.average_speed;
      const float curr_speed_length =
          length_over_speed *
          static_cast<float>(cursor_trail.average_trail_length);

      cursor_trail.trail_length =
          std::min(1 + static_cast<int>(floor(curr_speed_length)),
                   cursor_trail.max_trail_length);
    }

    cursor_trail.trail_positions.resize(cursor_trail.trail_length,
                                        sqt.translation);

    UpdateTrailMesh(&cursor_trail, sqt);
  });
}

void CursorTrailSystem::UpdateTrailMesh(CursorTrail* cursor_trail, Sqt sqt) {
  auto* input = registry_->Get<InputManager>();
  auto* render_system = registry_->Get<RenderSystem>();
  auto* cursor_system = registry_->Get<CursorSystem>();

  mathfu::vec4 cursor_color = cursor_trail->default_color;
  render_system->GetUniform(cursor_trail->GetEntity(), "color", 4,
                            &cursor_color[0]);

  mathfu::vec3 camera_position = mathfu::kZeros3f;
  if (input->HasPositionDof(InputManager::kHmd)) {
    camera_position = input->GetDofPosition(InputManager::kHmd);
  }

  const float no_hit_distance =
      cursor_system->GetNoHitDistance(cursor_trail->GetEntity());

  auto trail_fn = [this, cursor_trail, render_system, cursor_color,
                   camera_position, no_hit_distance, sqt](MeshData* mesh) {
    uint16_t index_base = 0;

    for (int i = 0; i < cursor_trail->trail_length; i++) {
      // Draw a trail of cursors from the last frame to current frame position,
      // using a cubic spline to interpolate.
      cursor_trail->trail_positions[i] = EvaluateCubicSpline(
          (static_cast<float>(i + 1)) /
              static_cast<float>(cursor_trail->trail_length),
          cursor_trail->position_history[0], cursor_trail->position_history[1],
          cursor_trail->position_history[2], sqt.translation);

      // Scale the quad_size to match the same scaling as the cursor would
      // have at that distance in ReticleSystem::SetReticleTransform().
      const mathfu::vec3 cursor_to_camera =
          camera_position - cursor_trail->trail_positions[i];
      const float scale = cursor_to_camera.Length() / no_hit_distance;

      VertexPTC v;

      // The cursor entity is scaled in ReticleSystem::SetReticleTransform(),
      // so we must account for that when setting the trail_position and the
      // quad_size by dividing by sqt.scale.
      cursor_trail->trail_positions[i] =
          sqt.rotation.Inverse() *
          (cursor_trail->trail_positions[i] - sqt.translation) / sqt.scale;

      float x = cursor_trail->trail_positions[i][0];
      float y = cursor_trail->trail_positions[i][1];
      float z = cursor_trail->trail_positions[i][2];
      float w = cursor_trail->quad_size * scale / sqt.scale.x;
      float h = cursor_trail->quad_size * scale / sqt.scale.y;

      mathfu::vec4 color = cursor_color;

      // Set the opacity of each cursor in the trail such that when the cursor
      // is still and they are all drawn on top of each other, they appear to be
      // a full opacity single cursor.
      color[3] *= 1.00f / static_cast<float>(cursor_trail->trail_length);
      v.color = Color4ub(color);
      v.u0 = 0.0f;
      v.v0 = 0.0f;
      mathfu::vec3 vert = mathfu::vec3(-.5f * w, -.5f * h, 0.0f);
      v.x = x + vert[0];
      v.y = y + vert[1];
      v.z = z + vert[2];
      mesh->AddVertex(v);

      v.u0 = 1.0f;
      vert = mathfu::vec3(.5f * w, -.5f * h, 0.0f);
      v.x = x + vert[0];
      v.y = y + vert[1];
      v.z = z + vert[2];
      mesh->AddVertex(v);

      v.v0 = 1.0f;
      vert = mathfu::vec3(.5f * w, .5f * h, 0.0f);
      v.x = x + vert[0];
      v.y = y + vert[1];
      v.z = z + vert[2];
      mesh->AddVertex(v);

      v.u0 = 0.0f;
      vert = mathfu::vec3(-.5f * w, .5f * h, 0.0f);
      v.x = x + vert[0];
      v.y = y + vert[1];
      v.z = z + vert[2];
      mesh->AddVertex(v);

      mesh->AddIndex(static_cast<MeshData::Index>(index_base + 0));
      mesh->AddIndex(static_cast<MeshData::Index>(index_base + 1));
      mesh->AddIndex(static_cast<MeshData::Index>(index_base + 2));
      mesh->AddIndex(static_cast<MeshData::Index>(index_base + 2));
      mesh->AddIndex(static_cast<MeshData::Index>(index_base + 3));
      mesh->AddIndex(static_cast<MeshData::Index>(index_base + 0));

      index_base = static_cast<MeshData::Index>(index_base + 4);
    }
  };

  render_system->UpdateDynamicMesh(
      cursor_trail->GetEntity(), MeshData::PrimitiveType::kTriangles,
      VertexPTC::kFormat, kNumVerticesPerTrailQuad * cursor_trail->trail_length,
      kNumIndicesPerTrailQuad * cursor_trail->trail_length, trail_fn);
}

}  // namespace lull
