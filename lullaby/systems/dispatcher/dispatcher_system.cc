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

#include "lullaby/systems/dispatcher/dispatcher_system.h"

#include "lullaby/modules/dispatcher/dispatcher_binder.h"
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/logging.h"

namespace lull {
const HashValue kEventResponseDefHash = ConstHash("EventResponseDef");

DispatcherSystem::DispatcherSystem(Registry* registry) : System(registry) {
  RegisterDef<EventResponseDefT>(this);
  RegisterDependency<Dispatcher>(this);
}

void DispatcherSystem::Initialize() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const EntityEvent& entity_event) {
    SendImmediatelyImpl(entity_event);
  });

  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    void (DispatcherSystem::*send)(Entity, const EventWrapper&) =
        &lull::DispatcherSystem::SendImmediatelyImpl;
    binder->RegisterMethod("lull.Dispatcher.Dispatch",
                           &lull::DispatcherSystem::Dispatch);
    binder->RegisterMethod("lull.Dispatcher.Send",
                           &lull::DispatcherSystem::SendImpl);
    binder->RegisterMethod("lull.Dispatcher.SendImmediately", send);
    void (DispatcherSystem::*disconnect)(Entity entity, TypeId,
                                         Dispatcher::ConnectionId) =
        &lull::DispatcherSystem::Disconnect;
    binder->RegisterMethod("lull.Dispatcher.Disconnect", disconnect);
    binder->RegisterFunction(
        "lull.Dispatcher.Connect",
        [this](Entity entity, TypeId type, Dispatcher::EventHandler handler) {
          auto connection = Connect(entity, type, this, std::move(handler));
          return connection.GetId();
        });

    registry_->Create<DispatcherBinder>(registry_);
  }
}

DispatcherSystem::~DispatcherSystem() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder && binder->IsFunctionRegistered("lull.Dispatcher.Dispatch")) {
    binder->UnregisterFunction("lull.Dispatcher.Dispatch");
    binder->UnregisterFunction("lull.Dispatcher.Send");
    binder->UnregisterFunction("lull.Dispatcher.SendImmediately");
    binder->UnregisterFunction("lull.Dispatcher.Disconnect");
    binder->UnregisterFunction("lull.Dispatcher.Connect");
  }
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
    Dispatcher* dispatcher = registry_->Get<Dispatcher>();
    auto connection = dispatcher->Connect(id, handler);
    connections_[entity].emplace_back(std::move(connection));
  }
}

void DispatcherSystem::SendImpl(Entity entity, const EventWrapper& event) {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Send(EntityEvent(entity, event));
}

void DispatcherSystem::SendImmediatelyImpl(Entity entity,
                                           const EventWrapper& event) {
  SendImmediatelyImpl(EntityEvent(entity, event));
}

void DispatcherSystem::SendImmediatelyImpl(const EntityEvent& entity_event) {
  ++dispatch_count_;
  // When an entity has been queued for destruction, treat it as already
  // destroyed.
  if (queued_destruction_.count(entity_event.entity) == 0) {
    auto iter = dispatchers_.find(entity_event.entity);
    if (iter != dispatchers_.end()) {
      iter->second.Send(entity_event.event);
    }
    universal_dispatcher_.Send(entity_event);
  }
  --dispatch_count_;
  DestroyQueued();
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

void DispatcherSystem::Disconnect(Entity entity, TypeId type,
                                  Dispatcher::ConnectionId id) {
  auto iter = dispatchers_.find(entity);
  if (iter == dispatchers_.end()) {
    return;
  }

  Dispatcher& dispatcher = iter->second;
  dispatcher.Disconnect(type, id);
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
