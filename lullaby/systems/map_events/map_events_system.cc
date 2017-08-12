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

#include "lullaby/systems/map_events/map_events_system.h"

#include <vector>

#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"

namespace lull {

namespace {
const HashValue kMapEventsToChildrenHash = Hash("MapEventsToChildrenDef");
const HashValue kMapEventsToParentHash = Hash("MapEventsToParentDef");
const HashValue kMapEventsToSiblingsHash = Hash("MapEventsToSiblingsDef");
const HashValue kMapEventsToGroupHash = Hash("MapEventsToGroupDef");

}  // namespace

MapEventsSystem::MapEventsSystem(Registry* registry)
    : System(registry), groups_() {
  RegisterDef(this, kMapEventsToChildrenHash);
  RegisterDef(this, kMapEventsToParentHash);
  RegisterDef(this, kMapEventsToSiblingsHash);
  RegisterDef(this, kMapEventsToGroupHash);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void MapEventsSystem::Create(Entity entity, HashValue type, const Def* def) {
  ResponseData response;
  response.entity = entity;
  if (type == kMapEventsToChildrenHash) {
    const auto* data = ConvertDef<MapEventsToChildrenDef>(def);
    response.mode = kChildren;
    ProcessEventMap(data->events(), &response);
  } else if (type == kMapEventsToParentHash) {
    const auto* data = ConvertDef<MapEventsToParentDef>(def);
    response.mode = kParent;
    ProcessEventMap(data->events(), &response);
  } else if (type == kMapEventsToSiblingsHash) {
    const auto* data = ConvertDef<MapEventsToSiblingsDef>(def);
    response.mode = kSiblings;
    ProcessEventMap(data->events(), &response);
    response.include_self = data->include_self();
  } else if (type == kMapEventsToGroupHash) {
    const auto* data = ConvertDef<MapEventsToGroupDef>(def);
    response.mode = kGroup;
    ProcessEventMap(data->events(), &response);
    response.include_self = data->include_self();
    if (!data->group()) {
      LOG(DFATAL) << "Group id must be set on a MapEventsToGroupDef!";
      return;
    }
    response.group_id = Hash(data->group()->c_str());
    groups_[response.group_id].insert(entity);
  } else {
    LOG(DFATAL) << "Unsupported ComponentDef type: " << type;
    return;
  }

  // Handle mapping events
  if (response.input_events != nullptr && response.output_events != nullptr) {
    ConnectEventDefs(registry_, entity, response.input_events,
                     [this, response](const EventWrapper& event) {
                       MapEvent(response, event);
                     });
  }
}

void MapEventsSystem::Destroy(Entity entity) {
  for (auto iter = groups_.begin(); iter != groups_.end();) {
    Group group = iter->second;
    group.erase(entity);
    if (group.empty()) {
      iter = groups_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void MapEventsSystem::ProcessEventMap(const EventMapDef* data,
                                      ResponseData* response) {
  if (data != nullptr) {
    if (data->input_events() || !data->output_events()) {
      response->input_events = data->input_events();
      response->output_events = data->output_events();
    } else {
      LOG(DFATAL) << "EventMap must have input and output events";
    }
  }
}

void MapEventsSystem::MapEvent(const ResponseData& response,
                               const EventWrapper& event) const {
  EventSender map_sender = [this, &response](Entity target) {
    SendEventDefsImmediately(registry_, target, response.output_events);
  };
  SendEventsToTargets(response, map_sender);
}

void MapEventsSystem::SendEventsToTargets(const ResponseData& response,
                                          const EventSender& sender) const {
  const auto* transform_system = registry_->Get<TransformSystem>();
  if (response.mode == kChildren) {
    const std::vector<Entity>* children =
        transform_system->GetChildren(response.entity);
    for (const Entity child : *children) {
      sender(child);
    }
  } else if (response.mode == kParent) {
    sender(transform_system->GetParent(response.entity));
  } else if (response.mode == kSiblings) {
    const Entity parent = transform_system->GetParent(response.entity);
    const std::vector<Entity>* siblings = transform_system->GetChildren(parent);
    for (const Entity child : *siblings) {
      if (response.include_self || child != response.entity) {
        sender(child);
      }
    }
  } else if (response.mode == kGroup) {
    const auto group = groups_.find(response.group_id);
    if (group != groups_.end()) {
      for (const Entity member : group->second) {
        if (response.include_self || member != response.entity) {
          sender(member);
        }
      }
    }
  }
}

}  // namespace lull
