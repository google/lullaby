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

#include "lullaby/systems/reticle/reticle_trail_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/reticle/reticle_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"
#include "mathfu/constants.h"

namespace lull {

const HashValue kReticleTrailDef = Hash("ReticleTrailDef");
const int kNumVerticesPerTrailQuad = 4;
const int kNumIndicesPerTrailQuad = 6;
const float kMaxDeltaTime = 0.05f;

ReticleTrailSystem::ReticleTrail::ReticleTrail(Entity e)
    : Component(e),
      default_color(mathfu::kZeros4f),
      max_trail_length(0),
      average_trail_length(0),
      trail_length(0),
      curve_samples(0),
      quad_size(0.f),
      average_speed(0.f) {}

ReticleTrailSystem::ReticleTrailSystem(Registry* registry) : System(registry) {
  RegisterDef(this, kReticleTrailDef);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void ReticleTrailSystem::Create(Entity entity, HashValue type, const Def* def) {
  CHECK_NE(def, nullptr);

  if (type == kReticleTrailDef) {
    const auto* data = ConvertDef<ReticleTrailDef>(def);
    CreateReticleTrail(entity, data);
  } else {
    DCHECK(false) << "Unsupported ComponentDef type: " << type;
  }
}

void ReticleTrailSystem::CreateReticleTrail(Entity entity,
                                            const ReticleTrailDef* data) {
  reticle_trail_.reset(new ReticleTrail(entity));
  reticle_trail_->curve_samples = data->curve_samples();
  reticle_trail_->max_trail_length = data->max_trail_length();
  reticle_trail_->average_trail_length = data->average_trail_length();
  reticle_trail_->average_speed = data->average_speed();
  reticle_trail_->quad_size = data->quad_size();
  MathfuVec4FromFbColor(data->default_color(), &reticle_trail_->default_color);

  reticle_trail_->position_history.resize(4, mathfu::kZeros3f);
  reticle_trail_->trail_positions.resize(reticle_trail_->average_trail_length,
                                         mathfu::kZeros3f);
}

void ReticleTrailSystem::Destroy(Entity entity) {
  if (reticle_trail_ && reticle_trail_->GetEntity() == entity) {
    reticle_trail_.reset();
  }
}

void ReticleTrailSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  if (!reticle_trail_) {
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  Sqt sqt = *(transform_system->GetSqt(reticle_trail_->GetEntity()));

  // Save the positions of the current frame and last three frames in order to
  // create our cubic interpolated curve.
  if (reticle_trail_->position_history.size() ==
      static_cast<uint16_t>(reticle_trail_->curve_samples)) {
    reticle_trail_->position_history.pop_front();
  }
  reticle_trail_->position_history.push_back(sqt.translation);

  // If the last frame was more than kMaxDeltaTime ago, don't draw a trail
  if (SecondsFromDuration(delta_time) > kMaxDeltaTime) {
    reticle_trail_->trail_length = 1;
  } else {
    // Attenuate the number of reticles drawn based on the speed of the current
    // reticle
    const float length = (reticle_trail_->position_history[3] -
                          reticle_trail_->position_history[2])
                             .Length();
    const float length_over_speed = length / reticle_trail_->average_speed;
    const float curr_speed_length =
        length_over_speed *
        static_cast<float>(reticle_trail_->average_trail_length);

    reticle_trail_->trail_length = std::min(
        1 + static_cast<int>(floor(curr_speed_length)),
        reticle_trail_->max_trail_length);
  }

  reticle_trail_->trail_positions.resize(reticle_trail_->trail_length,
                                         sqt.translation);

  UpdateTrailMesh(sqt);
}

void ReticleTrailSystem::UpdateTrailMesh(Sqt sqt) {
  auto* input = registry_->Get<InputManager>();
  auto* render_system = registry_->Get<RenderSystem>();
  auto* reticle_system = registry_->Get<ReticleSystem>();

  mathfu::vec4 reticle_color = reticle_trail_->default_color;
  render_system->GetUniform(reticle_trail_->GetEntity(), "color", 4,
                            &reticle_color[0]);

  mathfu::vec3 camera_position = mathfu::kZeros3f;
  if (input->HasPositionDof(InputManager::kHmd)) {
    camera_position = input->GetDofPosition(InputManager::kHmd);
  }

  const float no_hit_distance = reticle_system->GetNoHitDistance();

  auto trail_fn = [this, render_system, reticle_color, camera_position,
                   no_hit_distance, sqt](MeshData* mesh) {
    uint16_t index_base = 0;

    for (int i = 0; i < reticle_trail_->trail_length; i++) {
      // Draw a trail of reticles from the last frame to current frame position,
      // using a cubic spline to interpolate.
      reticle_trail_->trail_positions[i] = EvaluateCubicSpline(
          (static_cast<float>(i + 1)) /
              static_cast<float>(reticle_trail_->trail_length),
          reticle_trail_->position_history[0],
          reticle_trail_->position_history[1],
          reticle_trail_->position_history[2], sqt.translation);

      // Scale the quad_size to match the same scaling as the reticle would
      // have at that distance in ReticleSystem::SetReticleTransform().
      const mathfu::vec3 reticle_to_camera =
          camera_position - reticle_trail_->trail_positions[i];
      const float scale = reticle_to_camera.Length() / no_hit_distance;

      VertexPTC v;

      // The reticle entity is scaled in ReticleSystem::SetReticleTransform(),
      // so we must account for that when setting the trail_position and the
      // quad_size by dividing by sqt.scale.
      reticle_trail_->trail_positions[i] =
          sqt.rotation.Inverse() *
          (reticle_trail_->trail_positions[i] - sqt.translation) /
          sqt.scale;

      float x = reticle_trail_->trail_positions[i][0];
      float y = reticle_trail_->trail_positions[i][1];
      float z = reticle_trail_->trail_positions[i][2];
      float w = reticle_trail_->quad_size * scale / sqt.scale.x;
      float h = reticle_trail_->quad_size * scale / sqt.scale.y;

      mathfu::vec4 color = reticle_color;

      // Set the opacity of each reticle in the trail such that when the reticle
      // is still and they are all drawn on top of each other, they appear to be
      // a full opacity single reticle.
      color[3] *= 1.00f / static_cast<float>(reticle_trail_->trail_length);
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
      reticle_trail_->GetEntity(), MeshData::PrimitiveType::kTriangles,
      VertexPTC::kFormat,
      kNumVerticesPerTrailQuad * reticle_trail_->trail_length,
      kNumIndicesPerTrailQuad * reticle_trail_->trail_length, trail_fn);
}

}  // namespace lull
