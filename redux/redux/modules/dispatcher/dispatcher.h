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

#ifndef REDUX_MODULES_DISPATCHER_DISPATCHER_H_
#define REDUX_MODULES_DISPATCHER_DISPATCHER_H_

#include <cstddef>
#include <functional>

#include "redux/modules/base/function_traits.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/dispatcher/message.h"

namespace redux {

// A simple event handling mechanism.
//
// Usage example:
//
// struct SomeEvent {
//   int x = 0;
// };
// REDUX_SETUP_TYPEID(SomeEvent);
//
// struct SomeClass {
//   void HandleEvent(const SomeEvent& e) {
//     // Do something with `e`.
//   }
// };
//
// void GlobalHandleEvent(const SomeEvent& e) {
//   // Do something with `e`.
// }
//
// int main() {
//   Dispatcher dispatcher;
//   auto c1 = dispatcher.Connect([](const SomeEvent& event) {
//     GlobalHandleEvent(event);
//   });
//
//   SomeClass obj;
//   auto c2 = dispatcher.Connect([&](const SomeEvent& event) {
//     obj.HandleEvent(event);
//   });
//
//   dispatcher.Send(SomeEvent{123});
// }
//
// Running the above will result in calls to GlobalHandleEvent and
// obj.HandleEvent with SomeEvent.x == 123. The call order is not specified.
//
// The Connect() function returns a ScopedConnection object which must be
// stored by the client. When this object goes out of scope, the connected
// function is removed from the Dispatcher.
//
// Alternatively, clients can provide an additional "owner" const void* tag
// when connecting to a Dispatcher. In this case, a non-scoped Connection
// object is returned. The client can then call Disconnect on the Connection
// object, or disconnect from the Dispatcher using the same "owner" pointer as a
// way to identify the connection to close. A single "owner" pointer can be
// associated with multiple connections.
//
// In addition to sending/receiving concrete Event types, clients can connect
// and send Message objects directly. This allows clients to process
// events in a more generic way.
class Dispatcher {
 private:
  // Internal class that stores the map of TypeId to Handlers.
  class HandlerMap;

 public:
  using ConnectionId = std::uint32_t;
  using MessageHandler = std::function<void(const Message&)>;

  // Connection object returned by Dispatcher::Connect() which must be
  // explicitly disconnected by calling Connection::Disconnect().
  class Connection {
   public:
    Connection() = default;
    void Disconnect();

   private:
    Connection(TypeId type, ConnectionId id, std::weak_ptr<HandlerMap> handlers)
        : type_(type), id_(id), handlers_(std::move(handlers)) {}

    friend class Dispatcher;
    TypeId type_ = 0;
    ConnectionId id_ = 0;
    std::weak_ptr<HandlerMap> handlers_;
  };

  // ScopedConnection object returned by Dispatcher::Connect() which will
  // automatically disconnect the connection when it goes out of scope. One can
  // also explicitly disconnect the connection by calling
  // ScopedConnection::Disconnect().
  class ScopedConnection {
   public:
    ScopedConnection() = default;
    ScopedConnection(Connection c);
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&& rhs);
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ScopedConnection& operator=(ScopedConnection&& rhs);
    ~ScopedConnection();
    void Disconnect();

   private:
    Connection connection_;
  };

  Dispatcher();
  virtual ~Dispatcher() = default;

  Dispatcher(const Dispatcher&) = delete;
  Dispatcher& operator=(const Dispatcher&) = delete;

  // Processes any events that are being stored in a Dispatcher. While the
  // base-class Dispatcher does not queue any events, several subclasses
  // benefit from having such a function exposed as a base-class API.
  virtual void Dispatch() {}

  // Enqueues an event to all functions registered with the dispatcher. By
  // default, events are sent immediately to all handlers without any additional
  // processing, but derived classes can tweak/change this behaviour by
  // overridding the virtual SendImpl function.
  template <typename Event>
  void Send(const Event& event);

  // Same as the above Send function, but explicitly by-passes derived class
  // dynamic dispatch functionality. In other words, passes the Event directly
  // to registered handlers without giving a derived class that chance to
  // intervene.
  template <typename Event>
  void SendDirectly(const Event& event);

  // Connects the `handler` to listen to events, where the type of event is
  // specified by the signature of the `handler` (eg. void(const Event&));
  template <typename Fn>
  [[nodiscard]] ScopedConnection Connect(Fn&& handler);

  // Connects `handler` to listen directly to Message instances of the
  // specified `type`.
  [[nodiscard]] ScopedConnection Connect(TypeId type, MessageHandler handler);

  // Connects the `handler` to listen to events, where the type of event is
  // specified by the signature of the `handler` (ie. void(const Event&));
  // A non-null `owner` must also be specified which can be used as an
  // alternative way to disconnect the function.
  template <typename Fn>
  Connection Connect(const void* owner, Fn&& handler);

  // Connects `handler` to listen directly to Message objects of the
  // specified `type`. A non-null `owner` must also be specified which can be
  // used as an alternative way to disconnect the function.
  Connection Connect(TypeId type, const void* owner, MessageHandler handler);

  // Adds a handler that will be called with every event that goes through this
  // dispatcher.
  [[nodiscard]] ScopedConnection ConnectToAll(MessageHandler handler);

  // Disconnects all functions listening to the `Event` associated with the
  // specified `owner`.
  template <typename Event>
  void Disconnect(const void* owner);

  // Disconnects all functions listening to events of the specified `type`
  // associated with the specified `owner`.
  void Disconnect(TypeId type, const void* owner);

  // Disconnects all functions with the specified `owner`.
  void DisconnectAll(const void* owner);

  // Returns the total number of connections registered with this dispatcher.
  std::size_t GetTotalConnectionCount() const;

  // Returns the number of connections to an event of `type`.
  std::size_t GetConnectionCount(TypeId type) const;

 protected:
  // Passes `msg` to all the connected handlers with the same TypeId as the
  // Message. Derived classes can intercept these messages to perform
  // additional operations by overriding this function. They can then continue
  // with "normal" processing by explicitly calling Dispatcher::SendImpl on
  // the message.
  virtual void SendImpl(const Message& msg);

 private:
  Connection ConnectImpl(TypeId type, const void* owner,
                         MessageHandler handler);

  void DisconnectImpl(TypeId type, const void* owner);

  ConnectionId id_ = 0;
  std::shared_ptr<HandlerMap> handlers_;
};

template <typename Event>
inline void Dispatcher::Send(const Event& event) {
  if constexpr (std::is_same_v<Event, Message>) {
    SendImpl(event);
  } else {
    SendImpl(Message(event));
  }
}

template <typename Event>
inline void Dispatcher::SendDirectly(const Event& event) {
  if constexpr (std::is_same_v<Event, Message>) {
    Dispatcher::SendImpl(event);
  } else {
    Dispatcher::SendImpl(Message(event));
  }
}

template <typename Fn>
Dispatcher::ScopedConnection Dispatcher::Connect(Fn&& handler) {
  return Connect(nullptr, std::forward<Fn>(handler));
}

template <typename Fn>
Dispatcher::Connection Dispatcher::Connect(const void* owner, Fn&& handler) {
  using Traits = FunctionTraits<Fn>;
  static_assert(Traits::kNumArgs == 1,
                "Event handlers must only have a single argument");

  using Arg0 = typename Traits::template Arg<0>::Type;
  using Event = typename std::decay_t<Arg0>;
  static_assert(!std::is_same_v<Event, Message>,
                "Event handlers for Messages must use the Connect function "
                "overload that also takes a TypeId argument.");

  const TypeId type = GetTypeId<Event>();
  return ConnectImpl(type, owner, [handler](const Message& msg) mutable {
    const Event* obj = msg.Get<Event>();
    CHECK(obj != nullptr) << "Unable to extract Event from Message";
    handler(*obj);
  });
}

template <typename Event>
void Dispatcher::Disconnect(const void* owner) {
  const TypeId type = GetTypeId<Event>();
  DisconnectImpl(type, owner);
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Dispatcher);
REDUX_SETUP_TYPEID(redux::Dispatcher::MessageHandler);

#endif  // REDUX_MODULES_DISPATCHER_DISPATCHER_H_
