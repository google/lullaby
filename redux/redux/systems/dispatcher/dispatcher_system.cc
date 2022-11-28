/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/systems/dispatcher/dispatcher_system.h"

#include <functional>
#include <utility>

#include "redux/modules/base/choreographer.h"
#include "redux/modules/dispatcher/queued_dispatcher.h"

namespace redux {

// We defer removing dispatchers from Entities by a couple of frames in case
// we want to do any kind of notification for the Entity during/after it is
// destroyed.
static constexpr int kDestructionDelay = 2;

DispatcherSystem::DispatcherSystem(Registry* registry)
    : System(registry), fns_(registry) {}

void DispatcherSystem::OnRegistryInitialize() {
  root_dispatcher_.reset(new QueuedDispatcher());
  root_dispatcher_->Connect(this, [this](const EntityMessage& entity_event) {
    SendNowImpl(entity_event.entity, entity_event.message);
  });

  using GlobalFn = void (DispatcherSystem::*)(const Message&);
  using EntityFn = void (DispatcherSystem::*)(Entity, const Message&);

  fns_.RegisterMemFn("rx.Dispatcher.Send", this,
                     static_cast<GlobalFn>(&DispatcherSystem::Send));
  fns_.RegisterMemFn("rx.Dispatcher.SendToEntity", this,
                     static_cast<EntityFn>(&DispatcherSystem::SendToEntity));
  fns_.RegisterMemFn("rx.Dispatcher.SendNow", this,
                     static_cast<GlobalFn>(&DispatcherSystem::SendNow));
  fns_.RegisterMemFn("rx.Dispatcher.SendToEntityNow", this,
                     static_cast<EntityFn>(&DispatcherSystem::SendToEntityNow));
  fns_.RegisterMemFn("rx.Dispatcher.Dispatch", this,
                     &DispatcherSystem::Dispatch);

  auto choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo->Add<&DispatcherSystem::Dispatch>(Choreographer::Stage::kEvents);
  }
}

DispatcherSystem::~DispatcherSystem() { root_dispatcher_->DisconnectAll(this); }

void DispatcherSystem::Dispatch() {
  CHECK_EQ(dispatch_count_, 0) << "Should not call Dispatch during dispatch.";
  root_dispatcher_->Dispatch();

  // Decrement the counter for all Entities queued for destruction, removing
  // any Entities that have reached the end of their countdown.
  for (auto iter = queued_destruction_.begin();
       iter != queued_destruction_.end();) {
    --iter->second;

    if (iter->second == 0) {
      dispatchers_.erase(iter->first);
      queued_destruction_.erase(iter++);
    } else {
      ++iter;
    }
  }
}

void DispatcherSystem::SendImpl(Entity entity, const Message& message) {
  root_dispatcher_->Send(EntityMessage(entity, message));
}

void DispatcherSystem::SendNowImpl(Entity entity, const Message& message) {
  ++dispatch_count_;
  // When an entity has been queued for destruction, treat it as already
  // destroyed.
  if (!queued_destruction_.contains(entity)) {
    if (auto iter = dispatchers_.find(entity); iter != dispatchers_.end()) {
      iter->second->Send(message);
    }
  }
  --dispatch_count_;
}

Dispatcher* DispatcherSystem::GetDispatcher(Entity entity) {
  auto& dispatcher = dispatchers_[entity];
  if (dispatcher == nullptr) {
    dispatcher = std::make_unique<Dispatcher>();
  }
  queued_destruction_.erase(entity);
  return dispatcher.get();
}

void DispatcherSystem::DisconnectImpl(Entity entity, TypeId type,
                                      const void* owner) {
  auto iter = dispatchers_.find(entity);
  if (iter == dispatchers_.end()) {
    return;
  }

  auto& dispatcher = iter->second;
  dispatcher->Disconnect(type, owner);
  if (dispatcher->GetTotalConnectionCount() == 0) {
    RemoveImpl(entity);
  }
}

void DispatcherSystem::Send(const Message& message) {
  SendImpl(kNullEntity, message);
}

void DispatcherSystem::SendToEntity(Entity entity, const Message& message) {
  CHECK(entity != kNullEntity) << "Cannot send event to null entity.";
  SendImpl(entity, message);
}

void DispatcherSystem::SendNow(const Message& message) {
  SendNowImpl(kNullEntity, message);
}

void DispatcherSystem::SendToEntityNow(Entity entity, const Message& message) {
  CHECK(entity != kNullEntity) << "Cannot send event to null entity.";
  SendNowImpl(entity, message);
}

DispatcherSystem::ScopedConnection DispatcherSystem::Connect(
    TypeId type, std::function<void(const Message&)> handler) {
  return GetDispatcher(kNullEntity)->Connect(type, std::move(handler));
}

DispatcherSystem::ScopedConnection DispatcherSystem::Connect(
    Entity entity, TypeId type, std::function<void(const Message&)> handler) {
  CHECK(entity != kNullEntity) << "Cannot acquire dispatcher for null entity.";
  return GetDispatcher(entity)->Connect(type, std::move(handler));
}

DispatcherSystem::Connection DispatcherSystem::Connect(
    TypeId type, const void* owner,
    std::function<void(const Message&)> handler) {
  return GetDispatcher(kNullEntity)->Connect(type, owner, std::move(handler));
}

DispatcherSystem::Connection DispatcherSystem::Connect(
    Entity entity, TypeId type, const void* owner,
    std::function<void(const Message&)> handler) {
  CHECK(entity != kNullEntity) << "Cannot acquire dispatcher for null entity.";
  return GetDispatcher(entity)->Connect(type, owner, std::move(handler));
}

void DispatcherSystem::Disconnect(TypeId type, const void* owner) {
  DisconnectImpl(kNullEntity, type, owner);
}

void DispatcherSystem::Disconnect(Entity entity, TypeId type,
                                  const void* owner) {
  CHECK(entity != kNullEntity) << "Must specify entity to disconnect.";
  DisconnectImpl(entity, type, owner);
}

std::size_t DispatcherSystem::GetConnectionCount(TypeId type) const {
  return GetConnectionCount(kNullEntity, type);
}

std::size_t DispatcherSystem::GetConnectionCount(Entity entity,
                                                 TypeId type) const {
  if (auto iter = dispatchers_.find(entity); iter != dispatchers_.end()) {
    return iter->second->GetConnectionCount(type);
  } else {
    return 0;
  }
}

void DispatcherSystem::OnDestroy(Entity entity) {
  CHECK(entity != kNullEntity) << "Cannot remove from kNullEntity.";
  queued_destruction_.insert_or_assign(entity, kDestructionDelay);
}

void DispatcherSystem::RemoveImpl(Entity entity) {
  if (auto iter = dispatchers_.find(entity); iter != dispatchers_.end()) {
    iter->second->DisconnectAll(this);
  }
  queued_destruction_.insert_or_assign(entity, kDestructionDelay);
}

}  // namespace redux
