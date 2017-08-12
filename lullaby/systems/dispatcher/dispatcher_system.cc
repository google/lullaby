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

#include "lullaby/systems/dispatcher/dispatcher_system.h"

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/logging.h"

namespace lull {
const HashValue kEventResponseDefHash = Hash("EventResponseDef");
bool DispatcherSystem::enable_queued_dispatch_ = false;

DispatcherSystem::DispatcherSystem(Registry* registry) : System(registry) {
  RegisterDef(this, kEventResponseDefHash);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Dispatcher.Send",
                           &lull::DispatcherSystem::SendImpl);
  }
}

DispatcherSystem::~DispatcherSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Dispatcher.Send");
  }
}

void DispatcherSystem::EnableQueuedDispatch() {
  enable_queued_dispatch_ = true;
}

void DispatcherSystem::DisableQueuedDispatch() {
  enable_queued_dispatch_ = false;
}

void DispatcherSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kEventResponseDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting EventResponseDef!";
    return;
  }

  const EventResponseDef* data = ConvertDef<EventResponseDef>(def);
  if (data->outputs() && data->inputs()) {
    auto response = [this, entity, data](const EventWrapper&) {
      SendEventDefs(registry_, entity, data->outputs());
    };
    ConnectEventDefs(registry_, entity, data->inputs(), response);
  } else {
    LOG(DFATAL) << "EventResponseDef must have inputs and outputs defined.";
  }
}

void DispatcherSystem::Destroy(Entity entity) {
  connections_.erase(entity);
  if (dispatch_count_ > 0) {
    auto iter = dispatchers_.find(entity);
    if (iter != dispatchers_.end()) {
      iter->second.DisconnectAll(this);
    }
    queued_destruction_.insert(entity);
  } else {
    dispatchers_.erase(entity);
  }
}

void DispatcherSystem::ConnectEvent(Entity entity, const EventDef* input,
                                    const Dispatcher::EventHandler& handler) {
  if (input == nullptr) {
    LOG(DFATAL) << "EventDef is null.";
    return;
  }
  ConnectEventImpl(entity, Hash(input->event()->c_str()), input->local(),
                   input->global(), handler);
}

void DispatcherSystem::ConnectEvent(Entity entity, const EventDefT& input,
                                    const Dispatcher::EventHandler& handler) {
  ConnectEventImpl(entity, Hash(input.event.c_str()), input.local, input.global,
                   handler);
}

void DispatcherSystem::ConnectEventImpl(
    Entity entity, HashValue id, bool local, bool global,
    const Dispatcher::EventHandler& handler) {
  DCHECK(local || global) << "EventDefs must have local or global!";
  if (local) {
    Connect(entity, id, this, handler);
  }
  if (global) {
    auto dispatcher = registry_->Get<Dispatcher>();
    if (dispatcher) {
      auto connection = dispatcher->Connect(id, handler);
      connections_[entity].emplace_back(std::move(connection));
    }
  }
}

void DispatcherSystem::SendImpl(Entity entity, const EventWrapper& event) {
  if (enable_queued_dispatch_) {
    queue_.Enqueue(EntityEvent(entity, event));
  } else {
    SendImmediatelyImpl(entity, event);
  }
}

void DispatcherSystem::SendImmediatelyImpl(Entity entity,
                                           const EventWrapper& event) {
  ++dispatch_count_;
  // When an entity has been queued for destruction, treat it as already
  // destroyed.
  if (queued_destruction_.count(entity) == 0) {
    auto iter = dispatchers_.find(entity);
    if (iter != dispatchers_.end()) {
      iter->second.Send(event);
    }
    universal_dispatcher_.Send(EntityEvent(entity, event));
  }
  --dispatch_count_;
  DestroyQueued();
}

void DispatcherSystem::Dispatch() {
  EntityEvent event;
  while (queue_.Dequeue(&event)) {
    SendImmediatelyImpl(event.entity, event.event);
  }
}

Dispatcher* DispatcherSystem::GetDispatcher(Entity entity) {
  if (entity == kNullEntity) {
    return nullptr;
  }
  return &dispatchers_[entity];
}

void DispatcherSystem::Disconnect(Entity entity, TypeId type,
                                  const void* owner) {
  auto iter = dispatchers_.find(entity);
  if (iter == dispatchers_.end()) {
    return;
  }

  Dispatcher& dispatcher = iter->second;
  dispatcher.Disconnect(type, owner);
  if (dispatcher.GetHandlerCount() == 0) {
    Destroy(entity);
  }
}

Dispatcher::ScopedConnection DispatcherSystem::ConnectToAll(
    const EntityEventHandler& handler) {
  return universal_dispatcher_.Connect(handler);
}

size_t DispatcherSystem::GetHandlerCount(Entity entity, TypeId type) const {
  auto iter = dispatchers_.find(entity);
  if (iter != dispatchers_.end()) {
    return iter->second.GetHandlerCount(type);
  }
  return 0;
}

size_t DispatcherSystem::GetUniversalHandlerCount() const {
  return universal_dispatcher_.GetHandlerCount();
}

void DispatcherSystem::DestroyQueued() {
  if (dispatch_count_ == 0) {
    for (Entity entity : queued_destruction_) {
      dispatchers_.erase(entity);
    }
    queued_destruction_.clear();
  }
}

}  // namespace lull
