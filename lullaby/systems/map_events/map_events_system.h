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

#ifndef LULLABY_SYSTEMS_MAP_EVENTS_MAP_EVENTS_SYSTEM_H_
#define LULLABY_SYSTEMS_MAP_EVENTS_MAP_EVENTS_SYSTEM_H_

#include <unordered_set>

#include "lullaby/generated/map_events_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/dispatcher/event.h"

namespace lull {

// MapEventsSystem exposes EventResponders that allow complex event mapping.
// Using the |input_events| and |output_events| properties, events can cause
// new events to be sent to a set of targets (i.e. hover causing "dim" events to
// be sent to all siblings.)
class MapEventsSystem : public System {
 public:
  explicit MapEventsSystem(Registry* registry);

  void Create(Entity e, HashValue type, const Def* def) override;

  void Destroy(Entity e) override;

 private:
  using Group = std::unordered_set<Entity>;
  using EventSender = std::function<void(Entity target)>;

  enum TargetMode {
    kChildren,
    kParent,
    kSiblings,
    kGroup,
  };

  struct ResponseData {
    Entity entity = kNullEntity;
    TargetMode mode = kChildren;
    HashValue group_id = 0;
    bool include_self = false;
    const EventDefArray* input_events = nullptr;
    const EventDefArray* output_events = nullptr;
  };

  static void ProcessEventMap(const EventMapDef* data, ResponseData* response);
  void MapEvent(const ResponseData& response, const EventWrapper& event) const;
  void SendEventsToTargets(const ResponseData& response,
                           const EventSender& sender) const;

  std::unordered_map<HashValue, Group> groups_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MapEventsSystem);

#endif  // LULLABY_SYSTEMS_MAP_EVENTS_MAP_EVENTS_SYSTEM_H_
