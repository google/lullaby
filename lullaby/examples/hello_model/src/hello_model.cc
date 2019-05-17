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

#include "lullaby/examples/hello_model/src/hello_model.h"

#include "lullaby/examples/hello_model/entity_generated.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/model_asset/model_asset_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "mathfu/constants.h"
#include "mathfu/quaternion.h"

LULLABY_EXAMPLE_APP(HelloModel);

void HelloModel::OnInitialize() {
  auto* entity_factory = registry_->Get<lull::EntityFactory>();

  // Create Lullaby systems being used.
  entity_factory->CreateSystem<lull::ModelAssetSystem>();
  entity_factory->CreateSystem<lull::TransformSystem>();
  auto* render_system = entity_factory->CreateSystem<lull::RenderSystem>();

  // Initialize the entity factory definitions.
  entity_factory->Initialize<EntityDef, ComponentDef>(
      GetEntityDef, EnumNamesComponentDefType());
  entity_factory->Create("model");

  // Set the pass to clear the display.
  lull::RenderClearParams clear_params;
  clear_params.clear_options = lull::RenderClearParams::kColor |
                               lull::RenderClearParams::kDepth |
                               lull::RenderClearParams::kStencil;
  clear_params.color_value = mathfu::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  render_system->SetClearParams(lull::ConstHash("Opaque"), clear_params);
}

void HelloModel::OnAdvanceFrame(lull::Clock::duration delta_time) {
  auto* render_system = registry_->Get<lull::RenderSystem>();
  render_system->ProcessTasks();
  render_system->SubmitRenderData();
}

void HelloModel::OnRender(lull::Span<lull::RenderView> views) {
  auto* render_system = registry_->Get<lull::RenderSystem>();
  render_system->BeginFrame();
  render_system->BeginRendering();
  render_system->Render(views.data(), views.size(), lull::ConstHash("Opaque"));
  render_system->EndRendering();
  render_system->EndFrame();
}
