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

#ifndef LULLABY_SYSTEMS_DISPATCHER_DISPATCHER_SYSTEM_H_
#define LULLABY_SYSTEMS_DISPATCHER_DISPATCHER_SYSTEM_H_

#include "lullaby/generated/dispatcher_def_generated.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/thread_safe_queue.h"

namespace lull {

/// Provides a Dispatcher as a Component for each Entity.
class DispatcherSystem : public System {
 public:
  /// Pair of Entity and EventWrapper. Publicly this is only used to listen for
  /// all events via ConnectToAll.
  struct EntityEvent {
    EntityEvent() {}
    EntityEvent(Entity e, const EventWrapper& event)
        : entity(e), event(event) {}
    Entity entity = kNullEntity;
    EventWrapper event;
  };

  /// A function to allow event dispatches to be tracked and logged.
  using EntityEventHandler = std::function<void(const EntityEvent&)>;

  static void EnableQueuedDispatch();
  static void DisableQueuedDispatch();

  explicit DispatcherSystem(Registry* registry);

  ~DispatcherSystem() override;

  /// Associates EventResponses with the Entity based on the |def|.
  void Create(Entity entity, HashValue type, const Def* def) override;

  /// Destroys the Dispatcher and any Connections associated with the Entity.
  /// If currently dispatching, this will queue the dispatcher to be destroyed
  /// and prevent other events from being sent to it.  Otherwise it will destroy
  /// the dispatcher immediately.
  void Destroy(Entity entity) override;

  /// Sends |event| to all functions registered with the dispatcher associated
  /// with |entity|.  The |Event| type must be registered with
  /// LULLABY_SETUP_TYPEID.
  template <typename Event>
  void Send(Entity entity, const Event& event) {
    SendImpl(entity, EventWrapper(event));
  }

  void Send(Entity entity, const EventWrapper& event_wrapper) {
    SendImpl(entity, event_wrapper);
  }

  /// As Send, but will always send immediately regardless of QueuedDispatch
  /// setting.
  template <typename Event>
  void SendImmediately(Entity entity, const Event& event) {
    SendImmediatelyImpl(entity, EventWrapper(event));
  }

  void SendImmediately(Entity entity, const EventWrapper& event_wrapper) {
    SendImmediatelyImpl(entity, event_wrapper);
  }

  /// Dispatches all events currently queued in the DispatcherSystem.
  void Dispatch();

  /// Connects an event handler to the Dispatcher associated with |entity|.
  /// This function is a simple wrapper around the various Dispatcher::Connect
  /// functions.  For more information, please refer to the Dispatcher API.
  template <typename... Args>
  auto Connect(Entity entity, Args&&... args) -> decltype(
      std::declval<Dispatcher>().Connect(std::forward<Args>(args)...)) {
    Dispatcher* dispatcher = GetDispatcher(entity);
    if (dispatcher) {
      // If a dispatcher is queued to be destroyed and a new connection is
      // made, that dispatcher needs to be kept alive.
      queued_destruction_.erase(entity);
      return dispatcher->Connect(std::forward<Args>(args)...);
    } else {
      return Dispatcher::Connection();
    }
  }

  /// Connects the |handler| to an event as described by the |input|.
  void ConnectEvent(Entity entity, const EventDef* input,
                    const Dispatcher::EventHandler& handler);

  void ConnectEvent(Entity entity, const EventDefT& input,
                    const Dispatcher::EventHandler& handler);

  void ConnectEventImpl(
    Entity entity, HashValue id, bool local, bool global,
    const Dispatcher::EventHandler& handler);

  //// Adds a function that will be called for every event that is dispatched.
  Dispatcher::ScopedConnection ConnectToAll(const EntityEventHandler& handler);

  /// Disconnects an event handler identified by the |owner| from the Dispatcher
  /// associated with |entity|.  See Dispatcher::Disconnect for more
  /// information.
  template <typename Event>
  void Disconnect(Entity entity, const void* owner) {
    Disconnect(entity, GetTypeId<Event>(), owner);
  }

  /// Disconnects an event handler identified by the |owner| from the Dispatcher
  /// associated with |entity|.  See Dispatcher::Disconnect for more
  /// information.
  void Disconnect(Entity entity, TypeId type, const void* owner);

  /// Returns the number of functions listening for an event of |type|.
  size_t GetHandlerCount(Entity entity, TypeId type) const;

  /// Returns the number of functions listening for all events.
  size_t GetUniversalHandlerCount() const;

 private:
  using EventQueue = ThreadSafeQueue<EntityEvent>;
  using EntityDispatcherMap = std::unordered_map<Entity, Dispatcher>;
  using EntityConnections =
      std::unordered_map<Entity, std::vector<Dispatcher::ScopedConnection>>;

  void SendImpl(Entity entity, const EventWrapper& event);
  void SendImmediatelyImpl(Entity entity, const EventWrapper& event);

  Dispatcher* GetDispatcher(Entity entity);

  void DestroyQueued();

  EventQueue queue_;
  EntityConnections connections_;
  EntityDispatcherMap dispatchers_;
  static bool enable_queued_dispatch_;
  int dispatch_count_ = 0;

  /// Destroying dispatchers will invalidate any iterators in the dispatchers_
  /// map, and will cause problems if the code doing the destruction is
  /// executing in an event sent by the dispatcher being removed.  For safety,
  /// queue the destruction and handle it once |dispatch_count_| reaches 0.
  std::unordered_set<Entity> queued_destruction_;


  Dispatcher universal_dispatcher_;

  DispatcherSystem(const DispatcherSystem&);
  DispatcherSystem& operator=(const DispatcherSystem&);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DispatcherSystem::EntityEvent);
LULLABY_SETUP_TYPEID(lull::DispatcherSystem);

#endif  // LULLABY_SYSTEMS_DISPATCHER_DISPATCHER_SYSTEM_H_
