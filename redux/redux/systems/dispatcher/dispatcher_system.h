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

#ifndef REDUX_SYSTEMS_DISPATCHER_DISPATCHER_SYSTEM_H_
#define REDUX_SYSTEMS_DISPATCHER_DISPATCHER_SYSTEM_H_

#include <cstddef>
#include <functional>
#include <utility>

#include "redux/engines/script/function_binder.h"
#include "redux/modules/dispatcher/dispatcher.h"
#include "redux/modules/ecs/system.h"

namespace redux {

// Event/messaging System that uses Dispatchers for communication between
// Entities and Systems.
//
// There are two key concepts users need to consider when using the
// DispatcherSystem.
//
//   Events vs. Messages:
//     Dispatchers can process either Events or Messages. Events are the
//     generic name used for any C++ object instance that can be copied into the
//     Dispatcher. Messages are a special C++ class which is basically just a
//     TypeId + VarTable (see dispatcher/message.h). Sending/receiving concrete
//     Events is generally faster, but Messages are useful for data-driven or
//     scripting use-cases. Dispatchers will also automatically convert between
//     Events and Messages as needed (though, it comes at some cost).
//
//   Entity vs. Global:
//     Events can be sent/received by either Entity-specific dispatchers or a
//     special "Global" dispatcher depending on the use-case.
//
// There are 4 key operations:
//
// - Connect: Registers a function that will invoked with an Event/Message as
//            its argument.
//
// - Send: Enqueues an Event/Message into an internal buffer.
//
// - Dispatch: Removes all enqueued Events/Messages from the buffer and passes
//             them  as arugments to the connected functions.
//
// - Disconnect: Unregisters a function that was previously registered.
//
// Putting all this together, it means that almost every API in this class comes
// with 4 overloads. Event-based functions take a template <Event> argument
// whereas Message-based functions take a `const Message&`. Entity-based
// functions take an Entity argument, whereas there is no argument for Global
// Dispatcher functions.
//
// More information about how Dispatchers and QueuedDispatchers work can be
// found in the dispatcher module.
class DispatcherSystem : public System {
 public:
  // Alias some Dispatcher connection types; see dispatcher.h for more info.
  using Connection = Dispatcher::Connection;
  using ScopedConnection = Dispatcher::ScopedConnection;

  // Constructor taking the Registry which owns this instance.
  explicit DispatcherSystem(Registry* registry);
  ~DispatcherSystem() override;

  // Called by Registry::Initialize() at which point it is safe to access other
  // objects from the Registry.
  void OnRegistryInitialize();

  // Pops all events off the internal queue and passes them to corresponding
  // event handlers. Note: this function is automatically called by the
  // redux::Choreographer if it is available in the registry. (See
  // redux/modules/base/choreographer.h.)
  void Dispatch();

  // Queues an event or message with the DispatcherSystem. Calling
  // DispatcherSystem::Dispatch will process the queue, sending each event to
  // it's appropriate handler. The order of events in the queue will be
  // preserved.
  //
  // Events can be sent globally (using Send) or associated with a specific
  // Entity (using SendToEntity).
  template <typename Event>
  void Send(const Event& event);
  void Send(const Message& message);
  template <typename Event>
  void SendToEntity(Entity entity, const Event& event);
  void SendToEntity(Entity entity, const Message& message);

  // Similar to Send/SendToEntity, but by-passes the internal queue and invokes
  // the corresponding handlers immediately with the event or message.
  template <typename Event>
  void SendNow(const Event& event);
  void SendNow(const Message& message);
  template <typename Event>
  void SendToEntityNow(Entity entity, const Event& event);
  void SendToEntityNow(Entity entity, const Message& message);

  // Connects the `handler` to listen to events, where the type of event is
  // specified by the signature of the `handler` (e.g. void(const Event&));
  // Fn should be a callable type.
  template <typename Fn>
  [[nodiscard]] ScopedConnection Connect(Fn&& handler);
  template <typename Fn>
  [[nodiscard]] ScopedConnection Connect(Entity entity, Fn&& handler);

  // Connects `handler` to listen directly to Message instances of the
  // specified `type`.
  [[nodiscard]] ScopedConnection Connect(
      TypeId type, std::function<void(const Message&)> handler);
  [[nodiscard]] ScopedConnection Connect(
      Entity entity, TypeId type, std::function<void(const Message&)> handler);

  // Connects the `handler` to listen to events, where the type of event is
  // specified by the signature of the `handler` (i.e. void(const Event&));
  // A non-null `owner` must also be specified which can be used as an
  // alternative way to disconnect the function.
  template <typename Fn>
  Connection Connect(const void* owner, Fn&& handler);
  template <typename Fn>
  Connection Connect(Entity entity, const void* owner, Fn&& handler);

  // Connects `handler` to listen directly to Message objects of the
  // specified `type`. A non-null `owner` must also be specified which can be
  // used as an alternative way to disconnect the function.
  Connection Connect(TypeId type, const void* owner,
                     std::function<void(const Message&)> handler);
  Connection Connect(Entity entity, TypeId type, const void* owner,
                     std::function<void(const Message&)> handler);

  // Disconnects a previously connected event handler.
  template <typename Event>
  void Disconnect(const void* owner);
  template <typename Event>
  void Disconnect(Entity entity, const void* owner);
  void Disconnect(TypeId type, const void* owner);
  void Disconnect(Entity entity, TypeId type, const void* owner);

  // Returns the number of connections listening to the event.
  template <typename T>
  std::size_t GetConnectionCount() const;
  template <typename T>
  std::size_t GetConnectionCount(Entity entity) const;
  std::size_t GetConnectionCount(TypeId type) const;
  std::size_t GetConnectionCount(Entity entity, TypeId type) const;

  // Events intended for a specific Entity are bundled into this struct so that
  // they can be redirected as needed upon dispatch.
  struct EntityMessage {
    EntityMessage() = default;
    EntityMessage(Entity entity, const Message& message)
        : entity(entity), message(message) {}
    Entity entity = kNullEntity;
    Message message;
  };

 private:
  Dispatcher* GetDispatcher(Entity entity);
  void SendImpl(Entity entity, const Message& message);
  void SendNowImpl(Entity entity, const Message& message);
  void DisconnectImpl(Entity entity, TypeId type, const void* owner);
  void RemoveImpl(Entity entity);
  void OnDestroy(Entity entity) override;

  // The primary queued dispatcher through which all events are sent in order.
  std::unique_ptr<Dispatcher> root_dispatcher_;

  // The individual dispatchers for each Entity. Note that we redirect all
  // global-related opertions to kNullEntity to simplify implementation.
  absl::flat_hash_map<Entity, std::unique_ptr<Dispatcher>> dispatchers_;

  // Destroying dispatchers will invalidate any iterators in the dispatchers_
  // map, and will cause problems if the code doing the destruction is
  // executing in an event sent by the dispatcher being removed.  For safety,
  // queue the destruction and handle it once dispatch_count_ reaches 0.
  absl::flat_hash_map<Entity, int> queued_destruction_;

  // Counter for tracking "depth" of potentially recursive dispatch calls.
  int dispatch_count_ = 0;

  FunctionBinder fns_;
};

template <typename Event>
void DispatcherSystem::SendToEntity(Entity entity, const Event& event) {
  CHECK(entity != kNullEntity) << "Cannot send event to null entity.";
  SendImpl(entity, Message(event));
}

template <typename Event>
void DispatcherSystem::Send(const Event& event) {
  SendImpl(kNullEntity, Message(event));
}

template <typename Event>
void DispatcherSystem::SendToEntityNow(Entity entity, const Event& event) {
  CHECK(entity != kNullEntity) << "Cannot send event to null entity.";
  SendNowImpl(entity, Message(event));
}

template <typename Event>
void DispatcherSystem::SendNow(const Event& event) {
  SendNowImpl(kNullEntity, Message(event));
}

template <typename Fn>
DispatcherSystem::ScopedConnection DispatcherSystem::Connect(Entity entity,
                                                             Fn&& handler) {
  CHECK(entity != kNullEntity) << "Cannot acquire dispatcher for null entity.";
  return GetDispatcher(entity)->Connect(std::forward<Fn>(handler));
}

template <typename Fn>
DispatcherSystem::ScopedConnection DispatcherSystem::Connect(Fn&& handler) {
  return GetDispatcher(kNullEntity)->Connect(std::forward<Fn>(handler));
}

template <typename Fn>
DispatcherSystem::Connection DispatcherSystem::Connect(Entity entity,
                                                       const void* owner,
                                                       Fn&& handler) {
  CHECK(entity != kNullEntity) << "Cannot acquire dispatcher for null entity.";
  return GetDispatcher(entity)->Connect(owner, std::forward<Fn>(handler));
}

template <typename Fn>
DispatcherSystem::Connection DispatcherSystem::Connect(const void* owner,
                                                       Fn&& handler) {
  return GetDispatcher(kNullEntity)->Connect(owner, std::forward<Fn>(handler));
}

template <typename Event>
void DispatcherSystem::Disconnect(const void* owner) {
  DisconnectImpl(kNullEntity, GetTypeId<Event>(), owner);
}

template <typename Event>
void DispatcherSystem::Disconnect(Entity entity, const void* owner) {
  CHECK(entity != kNullEntity) << "Must specify entity to disconnect.";
  DisconnectImpl(entity, GetTypeId<Event>(), owner);
}

template <typename Event>
std::size_t DispatcherSystem::GetConnectionCount() const {
  return GetConnectionCount(GetTypeId<Event>());
}

template <typename Event>
std::size_t DispatcherSystem::GetConnectionCount(Entity entity) const {
  return GetConnectionCount(entity, GetTypeId<Event>());
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::DispatcherSystem);
REDUX_SETUP_TYPEID(redux::DispatcherSystem::EntityMessage);

#endif  // REDUX_SYSTEMS_DISPATCHER_DISPATCHER_SYSTEM_H_
