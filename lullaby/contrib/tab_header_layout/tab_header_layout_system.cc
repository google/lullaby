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

#include "lullaby/contrib/tab_header_layout/tab_header_layout_system.h"

#include "lullaby/events/input_events.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/modules/animation_channels/render_channels.h"
#include "lullaby/modules/animation_channels/transform_channels.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/contrib/deform/deform_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/contrib/layout/layout_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"

namespace lull {

namespace {

constexpr int kBackgroundQuadVerts = 4;
constexpr int kBackgroundQuadCornerVerts = 15;
constexpr float kBehindIndicatorHorizontalEpsilon = 0.002;
constexpr float kBehindIndicatorVerticalEpsilon = 0.005;

}  // namespace

TabHeaderLayoutSystem::TabHeaderLayoutSystem(Registry* registry)
    : System(registry), tab_layouts_(1) {
  RegisterDef<TabHeaderLayoutDefT>(this);
}

void TabHeaderLayoutSystem::CreateComponent(Entity e,
                                            const Blueprint& blueprint) {
  if (!blueprint.Is<TabHeaderLayoutDefT>()) {
    LOG(DFATAL) << "Invalid blueprint type. Expected TabHeaderLayoutDefT";
    return;
  }

  auto* tab_layout = tab_layouts_.Emplace(e);
  if (!tab_layout) {
    LOG(DFATAL) << "An entity can have only one TabHeaderLayoutDef.";
    return;
  }

  blueprint.Read(&tab_layout->def);
  if (tab_layout->def.tab_blueprint.empty()) {
    tab_layouts_.Destroy(e);
    LOG(DFATAL) << "tab_blueprint must be specified";
    return;
  }

  tab_layout->anim_time =
      std::chrono::milliseconds(tab_layout->def.anim_time_ms);
}

void TabHeaderLayoutSystem::Destroy(Entity e) { tab_layouts_.Destroy(e); }

void TabHeaderLayoutSystem::PostCreateComponent(Entity e,
                                                const Blueprint& blueprint) {
  TabLayout* tab_layout = tab_layouts_.Get(e);
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create the tab_indicator as a top-level entity so it won't impact the
  // layout of the tabs.
  tab_layout->tab_indicator =
      transform_system->CreateChild(e, tab_layout->def.tab_indicator_blueprint);
  tab_layout->tab_container =
      transform_system->CreateChild(e, tab_layout->def.tab_container);

  ConfigureTabs(tab_layout->def.tabs, tab_layout);

  // If the layout updates, we need to update the tab indicator to be positioned
  // correctly. In particular, the layout will update when the text loads.
  dispatcher_system->Connect(tab_layout->tab_container, this,
                             [this, e](const LayoutChangedEvent& event) {
                               TabLayout* tab_layout = tab_layouts_.Get(e);
                               if (!tab_layout->tab_entities.empty()) {
                                 SelectTab(e, tab_layout->selected_tab_index);
                                 SetBackgroundQuad(*tab_layout);
                               }
                             });

  // Register for changes to the tabs.
  dispatcher_system->Connect(e, this, [this](const ConfigureTabsEvent& event) {
    TabLayout* tab_layout = tab_layouts_.Get(event.tab_layout);
    RemoveAllTabs(tab_layout);
    ConfigureTabs(event.tabs, tab_layout);
  });

  // Register for tab selections from Java.
  dispatcher_system->Connect(e, this, [this, e](const ChangeTabEvent& event) {
    SelectTab(e, event.tab_index);
  });
}

void TabHeaderLayoutSystem::SelectTab(Entity layout, int tab_index) {
  auto* animation_system = registry_->Get<AnimationSystem>();
  TabLayout* tab_layout = tab_layouts_.Get(layout);

  CHECK_GE(tab_index, 0);
  CHECK_LT(tab_index, static_cast<int>(tab_layout->tab_entities.size()));

  if (tab_layout->selected_tab_index != tab_index) {
    // Reset the previously selected tab.
    Entity previous_tab =
        tab_layout->tab_entities[tab_layout->selected_tab_index];
    animation_system->SetTarget(previous_tab, UniformChannel::kColorChannelName,
                                &tab_layout->def.deselected_tab_color[0], 4,
                                tab_layout->anim_time);
  }

  const Entity selected_tab = tab_layout->tab_entities[tab_index];

  // Update the color of the selected tab.
  animation_system->SetTarget(selected_tab, UniformChannel::kColorChannelName,
                              &tab_layout->def.selected_tab_color[0], 4,
                              tab_layout->anim_time);

  auto* transform_system = registry_->Get<TransformSystem>();
  const Aabb* aabb = transform_system->GetAabb(selected_tab);
  const Entity hit_target = tab_layout->hit_targets[tab_index];
  const Aabb* hit_target_aabb = transform_system->GetAabb(hit_target);
  const Sqt* tab_sqt = transform_system->GetSqt(hit_target);

  Sqt updated_indicator_sqt;
  switch (tab_layout->def.indicator_location) {
    case IndicatorLocation_Underline:
      updated_indicator_sqt =
          GetUnderlineIndicatorPosition(*tab_sqt, *aabb, *tab_layout);
      break;
    case IndicatorLocation_Behind:
      updated_indicator_sqt =
          GetBehindIndicatorPosition(*tab_sqt, *hit_target_aabb, *tab_layout);
      break;
  }

  // TODO: Use correct easing for these animations.
  animation_system->SetTarget(
      tab_layout->tab_indicator, PositionChannel::kChannelName,
      &updated_indicator_sqt.translation[0], 3, tab_layout->anim_time);
  animation_system->SetTarget(
      tab_layout->tab_indicator, ScaleChannel::kChannelName,
      &updated_indicator_sqt.scale[0], 3, tab_layout->anim_time);

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Send(layout,
                          TabIndicatorMovedEvent({updated_indicator_sqt}));

  if (tab_layout->selected_tab_index != tab_index) {
    tab_layout->selected_tab_index = tab_index;
    dispatcher_system->Send(layout, TabChangedEvent(layout, tab_index));
  }
}

void TabHeaderLayoutSystem::AddHitPadding(const TabLayout& tab_layout,
                                          int tab_index, Entity tab_hit_target,
                                          Entity tab_text) {
  auto* transform_system = registry_->Get<TransformSystem>();
  const Aabb* aabb = transform_system->GetAabb(tab_text);
  Aabb updated_aabb(*aabb);
  updated_aabb.max += mathfu::vec3(tab_layout.def.hit_padding, 0.0f) / 2.0f;
  updated_aabb.min -= mathfu::vec3(tab_layout.def.hit_padding, 0.0f) / 2.0f;

  // For the first and last tab, we have to make the indicator wide enough to
  // cover the padding.
  if (tab_index == 0) {
    updated_aabb.min.x -= tab_layout.def.background_padding;
  }
  if (tab_index == tab_layout.tab_entities.size() - 1) {
    updated_aabb.max.x += tab_layout.def.background_padding;
  }

  transform_system->SetAabb(tab_hit_target, updated_aabb);
}

void TabHeaderLayoutSystem::SetBackgroundQuad(const TabLayout& tab_layout) {
  if (!tab_layout.def.add_background_quad) {
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  auto* render_system = registry_->Get<RenderSystem>();
  const Aabb* aabb = transform_system->GetAabb(tab_layout.tab_container);
  RenderSystem::Quad quad;
  quad.size = (aabb->max - aabb->min).xy();
  quad.verts = mathfu::vec2i(kBackgroundQuadVerts, kBackgroundQuadVerts);
  quad.corner_radius = quad.size.y / 2.0f;
  quad.corner_verts = kBackgroundQuadCornerVerts;
  quad.has_uv = true;
  render_system->SetQuad(tab_layout.GetEntity(), quad);
}

Sqt TabHeaderLayoutSystem::GetUnderlineIndicatorPosition(
    const Sqt& tab_sqt, const Aabb& tab_aabb,
    const TabLayout& tab_layout) const {
  Sqt result;
  result.translation.x = tab_sqt.translation.x;
  result.translation.y = tab_aabb.min.y + tab_sqt.translation.y -
                         tab_layout.def.tab_indicator_leading;
  result.scale.x = tab_aabb.max.x - tab_aabb.min.x;
  result.scale.y = tab_layout.def.tab_indicator_height;
  return result;
}

Sqt TabHeaderLayoutSystem::GetBehindIndicatorPosition(
    const Sqt& tab_sqt, const Aabb& hit_target_aabb,
    const TabLayout& tab_layout) const {
  Sqt result;
  // Position the indicator where the tab is, but handle the case that the
  // hit target aabb is not-centered by shifting by its average.
  result.translation.x = tab_sqt.translation.x +
                         0.5f * (hit_target_aabb.max.x + hit_target_aabb.min.x);
  result.scale.x = hit_target_aabb.max.x - hit_target_aabb.min.x;

  result.translation.y = tab_sqt.translation.y +
                         0.5f * (hit_target_aabb.max.y + hit_target_aabb.min.y);
  result.scale.y = hit_target_aabb.max.y - hit_target_aabb.min.y;
  // We need to make the indicator just a tiny bit bigger so you can't see the
  // background quad on it's edge.
  result.scale.x += kBehindIndicatorHorizontalEpsilon;
  result.scale.y += kBehindIndicatorVerticalEpsilon;

  return result;
}

void TabHeaderLayoutSystem::ConfigureTabs(const std::vector<std::string>& tabs,
                                          TabLayout* tab_layout) {
  const Entity e = tab_layout->GetEntity();
  int tab_index = 0;
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* text_system = registry_->Get<TextSystem>();
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  tab_layout->selected_tab_index = 0;
  for (const auto& tab_name : tabs) {
    Entity hit_target = transform_system->CreateChild(
        tab_layout->tab_container, tab_layout->def.hit_target_blueprint);
    tab_layout->hit_targets.push_back(hit_target);

    Entity tab_text = transform_system->CreateChild(
        hit_target, tab_layout->def.tab_blueprint);
    text_system->SetText(tab_text, tab_name);
    tab_layout->tab_entities.push_back(tab_text);
    dispatcher_system->Connect(
        hit_target, kClickEventHash, this,
        [this, e, tab_index](const EventWrapper& event) {
          SelectTab(e, tab_index);
        });
    dispatcher_system->Connect(tab_text, this,
                               [this, hit_target, tab_text, tab_index,
                                tab_layout](const AabbChangedEvent& event) {
                                 AddHitPadding(*tab_layout, tab_index,
                                               hit_target, tab_text);
                               });
    tab_index++;
  }

  if (!tab_layout->tab_entities.empty()) {
    // Reset the tab selection.
    SelectTab(e, 0);
  }
}

void TabHeaderLayoutSystem::RemoveAllTabs(TabLayout* tab_layout) {
  auto* entity_factory = registry_->Get<EntityFactory>();
  for (const auto& e : tab_layout->tab_entities) {
    entity_factory->Destroy(e);
  }
  tab_layout->tab_entities.clear();
  for (const auto& e : tab_layout->hit_targets) {
    entity_factory->Destroy(e);
  }
  tab_layout->hit_targets.clear();
}

}  // namespace lull
