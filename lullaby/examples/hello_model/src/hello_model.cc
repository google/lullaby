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
  entity_factory->CreateSystem<lull::RenderSystem>();
  entity_factory->CreateSystem<lull::TransformSystem>();

  // Initialize the entity factory definitions.
  entity_factory->Initialize<EntityDef, ComponentDef>(
      GetEntityDef, EnumNamesComponentDefType());
  entity_factory->Create("model");
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
  render_system->Render(
      views.data(), views.size(),
      static_cast<lull::RenderPass>(lull::ConstHash("ClearDisplay")));
  render_system->Render(views.data(), views.size());
  render_system->EndRendering();
  render_system->EndFrame();
}
