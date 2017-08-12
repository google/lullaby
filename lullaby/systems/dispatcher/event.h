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

#ifndef LULLABY_SYSTEMS_DISPATCHER_EVENT_H_
#define LULLABY_SYSTEMS_DISPATCHER_EVENT_H_

#include "lullaby/generated/dispatcher_def_generated.h"
#include "lullaby/modules/ecs/entity.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/util/registry.h"

namespace lull {

// Flatbuffer's type for [EventDef].
using EventDefArray = flatbuffers::Vector<flatbuffers::Offset<EventDef>>;

// Sends |event| to dispatcher and the dispatcher system (if it exists).
template <typename Event>
void SendEvent(Registry* registry, Entity e, const Event& event) {
  auto* dispatcher = registry->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Send(event);
  }

  auto* dispatcher_system = registry->Get<DispatcherSystem>();
  if (dispatcher_system) {
    dispatcher_system->Send(e, event);
  }
}

// Sends |event| to dispatcher and the dispatcher system (if it exists)
// immediately.
template <typename Event>
void SendEventImmediately(Registry* registry, Entity e, const Event& event) {
  auto* dispatcher = registry->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->SendImmediately(event);
  }

  auto* dispatcher_system = registry->Get<DispatcherSystem>();
  if (dispatcher_system) {
    dispatcher_system->SendImmediately(e, event);
  }
}

// Sends out a flatbuffer array of EventDefs.
void SendEventDefs(Registry* registry, Entity entity,
                   const EventDefArray* events);

// As above, but will bypass any queuing and send out the events immediately.
void SendEventDefsImmediately(Registry* registry, Entity entity,
                   const EventDefArray* events);

// Connect a handler to an array of EventDefs.  NOTE: Depends on and instance of
// dispatcher_system being in the registry.
void ConnectEventDefs(Registry* registry, Entity entity,
                      const EventDefArray* events,
                      const Dispatcher::EventHandler& handler);

void ConnectEventDefs(Registry* registry, Entity entity,
                      const std::vector<EventDefT>& events,
                      const Dispatcher::EventHandler& handler);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_DISPATCHER_EVENT_H_
