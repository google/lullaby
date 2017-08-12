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

#include "lullaby/systems/dispatcher/event.h"

#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"

namespace lull {
namespace {

const HashValue kEntityHash = Hash("entity");
const HashValue kTargetHash = Hash("target");
const HashValue kSelfHash = Hash("$self");

void SendEventDefsImpl(Registry* registry, Entity entity,
                       const EventDefArray* events, bool immediate) {
  if (!events) {
    return;
  }
  auto* dispatcher = registry->Get<Dispatcher>();
  auto* dispatcher_system = registry->Get<DispatcherSystem>();

  for (uint32_t i = 0; i < events->size(); ++i) {
    const EventDef* event_def = events->Get(i);
    DCHECK(event_def->local() || event_def->global());
    const HashValue hash = Hash(event_def->event()->c_str());

    EventWrapper event(hash, event_def->event()->c_str());
    const auto* values = event_def->values();
    if (values) {
      for (auto iter = values->begin(); iter != values->end(); ++iter) {
        const auto* key = iter->key();
        const void* variant_def = iter->value();
        if (key == nullptr || variant_def == nullptr) {
          LOG(ERROR) << "Invalid (nullptr) key-value data in EventDef.";
          continue;
        }

        Variant var;
        if (VariantFromFbVariant(iter->value_type(), variant_def, &var)) {
          const HashValue key_hash = Hash(key->c_str());
          if (key_hash == kEntityHash) {
            LOG(WARNING) << "Variant key 'entity' will get overwritten "
                         << "by event's entity.";
          } else if (key_hash == kTargetHash) {
            LOG(WARNING) << "Variant key 'target' will get overwritten "
                         << "by event's entity.";
          } else if (var.Get<HashValue>() &&
                     *var.Get<HashValue>() == kSelfHash) {
            event.SetValue(key_hash, entity);
          } else {
            event.SetValue(key_hash, var);
          }
        }
      }
    }
    event.SetValue(kEntityHash, entity);
    // TODO(b/31110750): Should not need to specify target as well.
    event.SetValue(kTargetHash, entity);

    if (immediate) {
      if (dispatcher && event_def->global()) {
        dispatcher->SendImmediately(event);
      }
      if (dispatcher_system && event_def->local()) {
        dispatcher_system->SendImmediately(entity, event);
      }
    } else {
      if (dispatcher && event_def->global()) {
        dispatcher->Send(event);
      }
      if (dispatcher_system && event_def->local()) {
        dispatcher_system->Send(entity, event);
      }
    }
  }
}
}  // namespace

void SendEventDefs(Registry* registry, Entity entity,
                   const EventDefArray* events) {
  SendEventDefsImpl(registry, entity, events, false);
}

void SendEventDefsImmediately(Registry* registry, Entity entity,
                              const EventDefArray* events) {
  SendEventDefsImpl(registry, entity, events, true);
}

void ConnectEventDefs(Registry* registry, Entity entity,
                      const EventDefArray* events,
                      const Dispatcher::EventHandler& handler) {
  if (!events) {
    return;
  }
  auto* dispatcher_system = registry->Get<DispatcherSystem>();
  if (dispatcher_system) {
    for (uint32_t i = 0; i < events->size(); ++i) {
      const EventDef* event = events->Get(i);
      dispatcher_system->ConnectEvent(entity, event, handler);
    }
  }
}

void ConnectEventDefs(Registry* registry, Entity entity,
                      const std::vector<EventDefT>& events,
                      const Dispatcher::EventHandler& handler) {
  auto* dispatcher_system = registry->Get<DispatcherSystem>();
  if (dispatcher_system) {
    for (const EventDefT& event : events) {
      dispatcher_system->ConnectEvent(entity, event, handler);
    }
  }
}

}  // namespace lull
