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

#include "lullaby/contrib/track_hmd/track_hmd_system.h"

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/track_hmd_def_generated.h"

namespace lull {
namespace {
static const HashValue kTrackHmdDef = ConstHash("TrackHmdDef");
}

TrackHmdSystem::TrackHmdSystem(Registry* registry)
    : System(registry), trackers_(4) {
  RegisterDef<TrackHmdDefT>(this);
  RegisterDependency<TransformSystem>(this);
}

void TrackHmdSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kTrackHmdDef) {
    Tracker* tracker = trackers_.Emplace(entity);
    auto data = ConvertDef<TrackHmdDef>(def);
    tracker->only_track_yaw = data->only_track_yaw();
    tracker->local_transform = data->local_transform();
    tracker->mirror = data->mirror();
    MathfuVec3FromFbVec3(data->euler_scale(), &tracker->euler_scale);
    tracker->convergence_rot_rate = data->convergence_rot_rate();
    tracker->convergence_trans_rate = data->convergence_trans_rate();
    tracker->convergence_max_rot_rad =
        data->convergence_max_rot_deg() * kDegreesToRadians;
    tracker->convergence_max_trans = data->convergence_max_trans();
    tracker->hmd_extra_transform_fn = {};
  } else {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting TrackHmdDef!";
  }
}

void TrackHmdSystem::Destroy(Entity e) { trackers_.Destroy(e); }

void TrackHmdSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  auto input_manager = registry_->Get<InputManager>();
  const mathfu::mat4 world_from_head =
      input_manager->GetDofWorldFromObjectMatrix(InputManager::kHmd);
  trackers_.ForEach([&](Tracker& tracker) {
    if (!tracker.resumed) {
      return;
    }
    UpdateTracker(delta_time, world_from_head, tracker);
  });
}

void TrackHmdSystem::SetExtraHmdTransformFn(
    Entity entity,
    const std::function<mathfu::mat4()>& hmd_extra_transform_fn) {
  Tracker* tracker = trackers_.Get(entity);
  if (!tracker) {
    LOG(DFATAL) << "SetExtraHmdTransformFn: Tracker is not found";
    return;
  }
  tracker->hmd_extra_transform_fn = hmd_extra_transform_fn;
}

void TrackHmdSystem::UpdateTracker(const Clock::duration& delta_time,
                                   const mathfu::mat4& world_from_head,
                                   const Tracker& tracker) const {
  auto transform_system = registry_->Get<TransformSystem>();
  const Entity entity = tracker.GetEntity();
  Sqt old_sqt = *transform_system->GetSqt(entity);
  const mathfu::mat4 world_from_head_with_extra =
      tracker.hmd_extra_transform_fn
          ? tracker.hmd_extra_transform_fn() * world_from_head
          : world_from_head;
  const mathfu::mat4* p_world_from_head = &world_from_head_with_extra;
  mathfu::mat4 mirrored_world_from_head;
  if (tracker.mirror || tracker.euler_scale != mathfu::kOnes3f) {
    Sqt sqt = CalculateSqtFromMatrix(world_from_head_with_extra);
    mathfu::vec3 euler = sqt.rotation.ToEulerAngles();
    if (tracker.mirror) {
      euler.y = -euler.y;
      euler.z = -euler.z;
    }
    euler *= tracker.euler_scale;
    sqt.rotation = mathfu::quat::FromEulerAngles(euler);
    mirrored_world_from_head = CalculateTransformMatrix(sqt);
    p_world_from_head = &mirrored_world_from_head;
  }
  if (tracker.local_transform) {
    transform_system->SetSqt(entity,
                             CalculateSqtFromMatrix(*p_world_from_head));
  } else {
    transform_system->SetWorldFromEntityMatrix(entity, *p_world_from_head);
  }

  if (!tracker.only_track_yaw && tracker.convergence_trans_rate == 0.f &&
      tracker.convergence_trans_rate == 0.f) {
    return;
  }

  Sqt sqt = *transform_system->GetSqt(entity);

  if (tracker.only_track_yaw) {
    sqt = GetHeading(sqt);
  }

  const float delta_time_sec =
      std::chrono::duration_cast<std::chrono::duration<float>>(
          delta_time).count();

  if (tracker.convergence_trans_rate > 0) {
    const float trans_factor = tracker.convergence_trans_rate * delta_time_sec;
    const auto pos =
        mathfu::Lerp(old_sqt.translation, sqt.translation, trans_factor);
    if (tracker.convergence_max_trans > 0) {
      sqt.translation = ProjectPositionToVicinity(pos, sqt.translation,
                                             tracker.convergence_max_trans);
    } else {
      sqt.translation = pos;
    }
  }

  if (tracker.convergence_rot_rate > 0) {
    const float rot_factor = tracker.convergence_rot_rate * delta_time_sec;
    const auto rot =
        mathfu::quat::Slerp(old_sqt.rotation, sqt.rotation, rot_factor);
    if (tracker.convergence_max_rot_rad > 0) {
      sqt.rotation = ProjectRotationToVicinity(rot, sqt.rotation,
                                          tracker.convergence_max_rot_rad);
    } else {
      sqt.rotation = rot;
    }
  }

  transform_system->SetSqt(entity, sqt);
}

void TrackHmdSystem::PauseTracker(Entity entity) {
  Tracker* tracker = trackers_.Get(entity);
  if (tracker != nullptr && tracker->resumed) {
    tracker->resumed = false;
  }
}

void TrackHmdSystem::ResumeTracker(Entity entity) {
  Tracker* tracker = trackers_.Get(entity);
  if (tracker != nullptr && !tracker->resumed) {
    tracker->resumed = true;
  }
}

}  // namespace lull
