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

#include "lullaby/generated/cursor_trail_def_generated.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/cursor/cursor_trail_system.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/reticle_trail_def_generated.h"

namespace lull {

const HashValue kReticleTrailDef = Hash("ReticleTrailDef");

ReticleTrailSystem::ReticleTrailSystem(Registry* registry) : System(registry) {
  RegisterDef(this, kReticleTrailDef);
  auto* entity_factory = registry_->Get<lull::EntityFactory>();
  entity_factory->CreateSystem<CursorTrailSystem>();
}

void ReticleTrailSystem::Create(Entity entity, HashValue type, const Def* def) {
  CHECK_NE(def, nullptr);

  if (type == kReticleTrailDef) {
    const auto* data = ConvertDef<ReticleTrailDef>(def);
    CursorTrailDefT cursor;

    cursor.curve_samples = data->curve_samples();
    cursor.max_trail_length = data->max_trail_length();
    cursor.average_trail_length = data->average_trail_length();
    cursor.average_speed = data->average_speed();
    cursor.quad_size = data->quad_size();
    mathfu::vec4 color;
    MathfuVec4FromFbColor(data->default_color(), &color);
    cursor.default_color = Color4ub(color);

    auto* cursor_trail_system = registry_->Get<CursorTrailSystem>();
    cursor_trail_system->CreateComponent(entity, Blueprint(&cursor));

  } else {
    DCHECK(false) << "Unsupported ComponentDef type: " << type;
  }
}

void ReticleTrailSystem::Destroy(Entity entity) {
  auto* cursor_trail_system = registry_->Get<CursorTrailSystem>();
  cursor_trail_system->Destroy(entity);
}

void ReticleTrailSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  auto* cursor_trail_system = registry_->Get<CursorTrailSystem>();
  cursor_trail_system->AdvanceFrame(delta_time);
}
}  // namespace lull
