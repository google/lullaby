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

#ifndef LULLABY_MODULES_DISPATCHER_DISPATCHER_H_
#define LULLABY_MODULES_DISPATCHER_DISPATCHER_H_

#include <assert.h>
#include <stdint.h>
#include <functional>
#include <memory>
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/util/macros.h"
#include "lullaby/util/typeid.h"

namespace lull {

/// A simple event handling mechanism.
///
/// The easiest way to explain it is probably through code:
///
/// struct SomeEvent {
///   int x = 0;
/// };
/// LULLABY_SETUP_TYPEID(SomeEvent);
///
/// struct SomeClass {
///   void HandleEvent(const SomeEvent& e) {
///     // Do something with |e|.
///   }
/// };
///
/// void GlobalHandleEvent(const SomeEvent& e) {
///   // Do something with |e|.
/// }
///
/// void main() {
///   Dispatcher dispatcher;
///   auto c1 = dispatcher.Connect([](const SomeEvent& event) {
///     GlobalHandleEvent(event);
///   });
///
///   SomeClass cls;
///   auto c2 = dispatcher.Connect([&](const SomeEvent& event) {
///     cls.HandleEvent(event);
///   });
///
///   dispatcher.Send(SomeEvent{123});
/// }
///
/// Running the above will result in calls to GlobalHandleEvent and
/// cls.HandleEvent with SomeEvent.x == 123.  Call order is not guaranteed.
///
/// The Dispatcher uses a "double-dispatch" mechanism to handle events that are
/// "sent" through it.  Internally, the Dispatcher wraps all events in an
/// EventWrapper class that stores the Event instance as a const void*.  The
/// EventWrapper is then passed to an EventHandler functor that first converts
/// the const void* back to the concrete Event instance and then calls the
/// functor connected with the Event.  (The use of two functors is where the
/// term "double-dispatch" gets its name.)
///
/// The Dispatcher stores a map of EventHandlers associated with the TypeIds of
/// Event.  Similarly, the EventWrapper stores the TypeId of the event instance
/// it is wrapping (as well as some additional metadata).  When an Event is
/// "sent" through the dispatcher, the list of EventHandlers associated with the
/// same TypeId as the EventWrapper is looked up, and the EventWrapper is passed
/// to those handlers.
///
/// The Connect() function returns a ScopedConnection object which must be
/// stored by the client.  When this object goes out of scope, the connected
/// function will be removed from the Dispatcher.
///
/// Alternatively, clients can provide an additional "owner" const void* tag
/// when connecting to a Dispatcher.  In this case, a non-scoped Connection
/// object is returned.  The client can then either call Disconnect on the
/// Connection object, or disconnect from the Dispatcher using the same "owner"
/// pointer as a way to identify the connection to close.  A single "owner"
/// pointer can be associated with multiple connections.
///
/// In addition to sending/receiving concrete Event types, clients can connect
/// and send EventWrapper objects directly.  This allows clients to process
/// events in a more generic way.
class Dispatcher {
 private:
  /// Unique identifier given to each connection.
  typedef uint32_t ConnectionId;

  /// Internal class that stores the map of TypeId to EventHandlers (and
  /// associated typedefs).
  class EventHandlerMap;
  typedef std::shared_ptr<EventHandlerMap> EventHandlerMapPtr;
  typedef std::weak_ptr<EventHandlerMap> EventHandlerMapWeakPtr;

 public:
  /// The underlying functor used for handling events.
  using EventHandler = std::function<void(const EventWrapper&)>;

  /// Connection object returned by Dispatcher::Connect which must be explicitly
  /// disconnected by calling Connection::Disconnect().
  class Connection {
   public:
    Connection();
    Connection(const EventHandlerMapPtr& handlers, TypeId type,
               ConnectionId id);

    /// Disconnect event handler from the dispatcher.  It is safe to call this
    /// function multiple times.
    void Disconnect();

   private:
    TypeId type_;
    ConnectionId id_;
    EventHandlerMapWeakPtr handlers_;
  };

  /// ScopedConnection object returned by Dispatcher::Connect which will
  /// automatically disconnect the connection when this object goes out of
  /// scope.
  class LULLABY_WARN_UNUSED_RESULT ScopedConnection {
   public:
    ScopedConnection() {}
    ScopedConnection(Connection c);
    ScopedConnection(ScopedConnection&& rhs);
    ScopedConnection& operator=(ScopedConnection&& rhs);
    ~ScopedConnection();

    /// Explicitly disconnect the connection rather than waiting for the
    /// ScopedConnection to go out of scope.
    void Disconnect();

   private:
    Connection connection_;

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
  };

  Dispatcher();
  virtual ~Dispatcher() {}

  /// Processes any events that are being stored in a Dispatcher.  While the
  /// base-class Dispatcher does not queue any events, several subclasses
  /// benefit from having such a function exposed as a base-class API.
  virtual void Dispatch() {}

  /// Sends an event to all functions registered with the dispatcher.  The
  /// |Event| object must be registered with LULLABY_SETUP_TYPEID.
  template <typename Event>
  void Send(const Event& event);

  /// Same as the above Send function, but this will be sent immediately
  /// regardless of thread safety, order, etc.
  template <typename Event>
  void SendImmediately(const Event& event);

  /// Sends the EventWrapper to all the functions Connected with the Dispatcher
  /// with the same TypeId as the EventWrapper.
  void Send(const EventWrapper& event);

  /// Connects the |handler| to listen to events, where the type of event is
  /// specified by the signature of the |handler| (eg. void(const Event&));
  /// Returns: ScopedConnection which will automatically disconnect the function
  ///   when it goes out of scope.
  template <typename Fn>
  ScopedConnection Connect(Fn&& handler);

  /// Connects |handler| to listen directly to EventWrapper instances of the
  /// specified |type|.
  /// Returns: ScopedConnection which will automatically disconnect the function
  ///   when it goes out of scope.
  ScopedConnection Connect(TypeId type, EventHandler handler);

  /// Connects the |handler| to listen to events, where the type of event is
  /// specified by the signature of the |handler| (ie. void(const Event&));
  /// A non-null |owner| must also be specified which can be used as an
  /// alternative way to disconnect the function.
  /// Returns: Connection which can be used to disconnect the function.
  template <typename Fn>
  Connection Connect(const void* owner, Fn&& fn);

  /// Connects |handler| to listen directly to EventWrapper objects of the
  /// specified |type|.  A non-null |owner| must also be specified which can be
  /// used as an alternative way to disconnect the function.
  /// Returns: Connection which can be used to disconnect the function.
  Connection Connect(TypeId type, const void* owner, EventHandler handler);

  /// Adds a handler that will be called with every event that goes through this
  /// dispatcher.
  ScopedConnection ConnectToAll(EventHandler handler);

  /// Disconnects all functions listening to the |Event| associated with the
  /// specified |owner|.
  template <typename Event>
  void Disconnect(const void* owner);

  /// Disconnects all functions listening to events of the specified |type|
  /// associated with the specified |owner|.
  void Disconnect(TypeId type, const void* owner);

  /// Disconnects all functions with the specified |owner|.
  void DisconnectAll(const void* owner);

  /// Returns the number of functions currently registered with this dispatcher.
  size_t GetHandlerCount() const;

  /// Returns the number of functions listening for an event of |type|.
  size_t GetHandlerCount(TypeId type) const;

 protected:
  /// Passes the EventWrapper to all the functions Connected with the Dispatcher
  /// with the same TypeId as the EventWrapper.
  virtual void SendImpl(const EventWrapper& event);

 private:
  /// Helper function declaration that is used to extract the Event type from
  /// an event handler.
  template <typename Fn, typename Arg>
  Arg ConnectHelper(void (Fn::*)(const Arg&) const);

  /// Mutable helper function declaration that is used to extract the Event type
  /// from an event handler.
  template <typename Fn, typename Arg>
  Arg ConnectHelper(void (Fn::*)(const Arg&));

  /// Creates the actual Handler instance, registers it with the map, and
  /// returns the corresponding Connection object.
  Connection ConnectImpl(TypeId type, const void* owner, EventHandler handler);

  /// Removes the Handler that matches the |type| and |owner|.
  void DisconnectImpl(TypeId type, const void* owner);

  /// Autoincrementing value for generating unique connection IDs.
  ConnectionId id_ = 0;

  /// Map of TypeId to EventHandlers.  Uses a shared_ptr to allow Connection
  /// objects to safely "disconnect" from Dispatchers that have been destroyed.
  EventHandlerMapPtr handlers_;

  Dispatcher(const Dispatcher&) = delete;
  Dispatcher& operator=(const Dispatcher&) = delete;
};

template <typename Event>
inline void Dispatcher::Send(const Event& event) {
  SendImpl(EventWrapper(event));
}

template <typename Event>
inline void Dispatcher::SendImmediately(const Event& event) {
  Dispatcher::SendImpl(EventWrapper(event));
}

template <typename Fn>
Dispatcher::ScopedConnection Dispatcher::Connect(Fn&& handler) {
  return Connect(nullptr, std::forward<Fn>(handler));
}

template <typename Fn>
Dispatcher::Connection Dispatcher::Connect(const void* owner, Fn&& handler) {
  using FnType = typename std::remove_reference<Fn>::type;
  using Event = decltype(ConnectHelper(&FnType::operator()));

  const TypeId type = GetTypeId<Event>();
  return ConnectImpl(type, owner, [handler](const EventWrapper& event) mutable {
    const Event* obj = event.Get<Event>();
    handler(*obj);
  });
}

template <typename Event>
void Dispatcher::Disconnect(const void* owner) {
  const TypeId type = GetTypeId<Event>();
  DisconnectImpl(type, owner);
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Dispatcher);

#endif  // LULLABY_MODULES_DISPATCHER_DISPATCHER_H_
