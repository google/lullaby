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

#ifndef LULLABY_CONTRIB_TRACK_HMD_TRACK_HMD_SYSTEM_H_
#define LULLABY_CONTRIB_TRACK_HMD_TRACK_HMD_SYSTEM_H_

#include "mathfu/glsl_mappings.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/clock.h"

namespace lull {

// The TrackHmdSystem updates the transform of associated entities based on the
// HMD transform.
class TrackHmdSystem : public System {
 public:
  explicit TrackHmdSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

 private:
  struct Tracker : Component {
    explicit Tracker(Entity entity) : Component(entity) {}
    bool only_track_yaw = false;
    bool local_transform = false;
    bool mirror = false;
    mathfu::vec3 euler_scale = mathfu::kOnes3f;
    float convergence_rot_rate;
    float convergence_trans_rate;
    float convergence_max_rot_rad;
    float convergence_max_trans;
  };

  void UpdateTracker(const Clock::duration& delta_time,
                     const mathfu::mat4& world_from_head,
                     const Tracker& tracker) const;

  ComponentPool<Tracker> trackers_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TrackHmdSystem);

#endif  // LULLABY_CONTRIB_TRACK_HMD_TRACK_HMD_SYSTEM_H_
