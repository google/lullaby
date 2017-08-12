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

#ifndef LULLABY_SYSTEMS_RETICLE_RETICLE_TRAIL_SYSTEM_H_
#define LULLABY_SYSTEMS_RETICLE_RETICLE_TRAIL_SYSTEM_H_

#include <deque>

#include "lullaby/generated/reticle_trail_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// The ReticleTrailSystem renders the reticle trail
class ReticleTrailSystem : public System {
 public:
  explicit ReticleTrailSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

 private:
  struct ReticleTrail : Component {
    explicit ReticleTrail(Entity entity);

    std::vector<mathfu::vec3> trail_positions;
    std::deque<mathfu::vec3> position_history;
    mathfu::vec4 default_color;
    int max_trail_length = 0;
    int average_trail_length = 0;
    int trail_length = 0;
    int curve_samples = 0;
    float quad_size = 0.f;
    float average_speed = 0.f;
  };

  void CreateReticleTrail(Entity entity, const ReticleTrailDef* data);

  void UpdateTrailMesh(Sqt sqt);

  std::unique_ptr<ReticleTrail> reticle_trail_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReticleTrailSystem);

#endif  // LULLABY_SYSTEMS_RETICLE_RETICLE_TRAIL_SYSTEM_H_
