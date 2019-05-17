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

#ifndef LULLABY_CONTRIB_TAB_HEADER_LAYOUT_TAB_HEADER_LAYOUT_SYSTEM_H_
#define LULLABY_CONTRIB_TAB_HEADER_LAYOUT_TAB_HEADER_LAYOUT_SYSTEM_H_

#include "lullaby/modules/ecs/blueprint.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/tab_header_layout_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// This system manages a set of tab headers that the user can switch between.
// This is similar to Android's TabLayout control.
// The entity containing a TabLayoutDef does *not* manage the actual view pages,
// it only contains the header buttons and the animation behavior when the user
// interacts with them.
class TabHeaderLayoutSystem : public System {
 public:
  explicit TabHeaderLayoutSystem(Registry* registry);
  void CreateComponent(Entity e, const Blueprint& blueprint) override;
  void PostCreateComponent(Entity e, const Blueprint& blueprint) override;
  void Destroy(Entity e) override;

  // Selects the indicated tab in a TabLayout (deselects whichever was
  // previously selected). Animates the state transition.
  void SelectTab(Entity layout, int tab_index);

 private:
  struct TabLayout : Component {
    explicit TabLayout(Entity e) : Component(e) {}

    TabHeaderLayoutDefT def;
    Entity tab_indicator = kNullEntity;
    Entity tab_container = kNullEntity;
    int selected_tab_index = 0;
    std::vector<Entity> tab_entities;
    std::vector<Entity> hit_targets;
    Clock::duration anim_time;
  };

  // Updates the Aabb of the tab_hit_target based on its text field, adding in
  // padding to make the text easier to click.
  void AddHitPadding(const TabLayout& tab_layout, int tab_index,
                     Entity tab_hit_target, Entity tab_text);

  void SetBackgroundQuad(const TabLayout& tab_layout);

  // Gets the location and scale of an indicator underlining the active tab.
  // tab_sqt is the location of the active tab.
  // tab_aabb is the bounding box of the active tab.
  // tab_layout is the configuration of the tab layout.
  Sqt GetUnderlineIndicatorPosition(const Sqt& tab_sqt, const Aabb& tab_aabb,
                                    const TabLayout& tab_layout) const;

  // Gets the location and scale of an indicator placed behind the active tab.
  // tab_sqt is the location of the active tab.
  // hit_target_aabb is the bounding box of the hit target for the tab.
  // tab_layout is the configuration of the tab layout.
  Sqt GetBehindIndicatorPosition(const Sqt& tab_sqt,
                                 const Aabb& hit_target_aabb,
                                 const TabLayout& tab_layout) const;

  // Clears any existing tabs and creates entities for the provided tabs.
  void ConfigureTabs(const std::vector<std::string>& tabs,
                     TabLayout* tab_layout);

  // Removes all existing tabs.
  void RemoveAllTabs(TabLayout* tab_layout);

  ComponentPool<TabLayout> tab_layouts_;
};

// Event sent when the active tab is changed.
struct TabChangedEvent {
  TabChangedEvent() {}
  explicit TabChangedEvent(Entity e, int index)
      : tab_layout(e), tab_index(index) {}
  Entity tab_layout = kNullEntity;
  int tab_index = 0;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&tab_index, Hash("tab_index"));
    archive(&tab_layout, Hash("tab_layout"));
  }
};

// Event sent when the tab indicator is repositioned.
struct TabIndicatorMovedEvent {
  // The tab indicator's new target SQT.
  Sqt sqt;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&sqt, Hash("sqt"));
  }
};

// This event causes the TabHeaderLayout to be reconfigured with a different set
// of tabs.
struct ConfigureTabsEvent {
  ConfigureTabsEvent() {}
  // Constructs an event to reconfigure the TabHeaderLayout specified by |e|
  // to have the tabs specified by |tabs|.
  ConfigureTabsEvent(Entity e, const std::vector<std::string>& tabs)
      : tab_layout(e), tabs(tabs) {}
  Entity tab_layout = kNullEntity;
  std::vector<std::string> tabs;
};

// Event used to change the currently selected tab.
struct ChangeTabEvent {
  ChangeTabEvent() {}
  explicit ChangeTabEvent(int index) : tab_index(index) {}
  int tab_index = 0;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&tab_index, Hash("tab_index"));
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ChangeTabEvent);
LULLABY_SETUP_TYPEID(lull::ConfigureTabsEvent);
LULLABY_SETUP_TYPEID(lull::TabChangedEvent);
LULLABY_SETUP_TYPEID(lull::TabHeaderLayoutSystem);
LULLABY_SETUP_TYPEID(lull::TabIndicatorMovedEvent);

#endif  // LULLABY_CONTRIB_TAB_HEADER_LAYOUT_TAB_HEADER_LAYOUT_SYSTEM_H_
