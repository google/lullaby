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

#include "lullaby/modules/dispatcher/dispatcher.h"

#include <unordered_map>
#include <vector>

namespace lull {

// Stores a map of TypeId to EventHandlers that is used by the Dispatcher for
// sending events.
//
// The EventHandlers can be invoked via the Dispatch() function.  Adding and
// removing EventHandlers during Dispatch() is safely handled by storing the
// add/remove request in a "command queue" and processing the queue when the
// dispatch process is complete.  As a result, any EventHandler added during a
// Dispatch() will not be invoked.
//
// This class is not thread-safe.  All calls to an instance of this class must
// be done synchronously.
class Dispatcher::EventHandlerMap {
 public:
  EventHandlerMap();

  // Associates an EventHandler with the specified event |type|.
  void Add(TypeId type, ConnectionId id, const void* owner, EventHandler fn);

  // Removes an EventHandler that matches the given parameters as best as
  // possible.
  void Remove(TypeId type, ConnectionId id, const void* owner);

  // Pass the |event| to all EventHandlers associated with the same TypeId as
  // the |event|.
  void Dispatch(const EventWrapper& event);

  // Returns the number of active connections.
  size_t Size() const;

  /// Returns the number of connections for an event of |type|.
  size_t GetHandlerCount(TypeId type) const;

 private:
  // Wraps an EventHandler with two extra "tags" (ConnectionId id and const
  // void* owner) that can be used to find specific EventHandler instances.
  struct TaggedEventHandler {
    TaggedEventHandler(ConnectionId id, const void* owner, EventHandler fn)
        : id(id), owner(owner), fn(std::move(fn)) {}

    ConnectionId id;
    const void* owner;
    EventHandler fn;
  };

  // Actually add the EventHandler.
  void AddImpl(TypeId type, TaggedEventHandler handler);

  // Actually remove the EventHandler.
  void RemoveImpl(TypeId type, TaggedEventHandler handler);

  // Counter for tracking Dispatch() calls.
  int dispatch_count_;

  // Deferred queue of add/remove commands for when a Dispatch() is in progress.
  std::vector<std::pair<TypeId, TaggedEventHandler>> command_queue_;

  // Map of registered handlers.
  std::unordered_multimap<TypeId, TaggedEventHandler> map_;
};

Dispatcher::Dispatcher() { handlers_.reset(new EventHandlerMap()); }

void Dispatcher::Send(const EventWrapper& event) { SendImpl(event); }

Dispatcher::ScopedConnection Dispatcher::Connect(TypeId type,
                                                 EventHandler handler) {
  return ConnectImpl(type, nullptr, std::move(handler));
}

Dispatcher::Connection Dispatcher::Connect(TypeId type, const void* owner,
                                           EventHandler handler) {
  return ConnectImpl(type, owner, std::move(handler));
}

Dispatcher::ScopedConnection Dispatcher::ConnectToAll(EventHandler handler) {
  return ConnectImpl(0, nullptr, std::move(handler));
}

void Dispatcher::Disconnect(TypeId type, const void* owner) {
  DisconnectImpl(type, owner);
}

void Dispatcher::SendImpl(const EventWrapper& event) {
  handlers_->Dispatch(event);
}

Dispatcher::Connection Dispatcher::ConnectImpl(TypeId type, const void* owner,
                                               EventHandler handler) {
  const ConnectionId id = ++id_;
  handlers_->Add(type, id, owner, std::move(handler));
  return Connection(handlers_, type, id);
}

void Dispatcher::DisconnectAll(const void* owner) {
  handlers_->Remove(0, 0, owner);
}

void Dispatcher::DisconnectImpl(TypeId type, const void* owner) {
  handlers_->Remove(type, 0, owner);
}

size_t Dispatcher::GetHandlerCount() const { return handlers_->Size(); }

size_t Dispatcher::GetHandlerCount(TypeId type) const {
  return handlers_->GetHandlerCount(type);
}

Dispatcher::Connection::Connection() : type_(0), id_(0), handlers_() {}

Dispatcher::Connection::Connection(const EventHandlerMapPtr& handlers,
                                   TypeId type, ConnectionId id)
    : type_(type), id_(id), handlers_(handlers) {}

void Dispatcher::Connection::Disconnect() {
  if (auto handlers = handlers_.lock()) {
    handlers->Remove(type_, id_, nullptr);
    handlers_.reset();
  }
}

Dispatcher::ScopedConnection::ScopedConnection(ScopedConnection&& rhs)
    : connection_(rhs.connection_) {
  rhs.connection_ = Connection();
}

Dispatcher::ScopedConnection& Dispatcher::ScopedConnection::operator=(
    ScopedConnection&& rhs) {
  if (this != &rhs) {
    Disconnect();
    connection_ = rhs.connection_;
    rhs.connection_ = Connection();
  }
  return *this;
}

Dispatcher::ScopedConnection::ScopedConnection(Connection c) : connection_(c) {}

Dispatcher::ScopedConnection::~ScopedConnection() { Disconnect(); }

void Dispatcher::ScopedConnection::Disconnect() { connection_.Disconnect(); }

Dispatcher::EventHandlerMap::EventHandlerMap() : dispatch_count_(0) {}

void Dispatcher::EventHandlerMap::Add(TypeId type, ConnectionId id,
                                      const void* owner, EventHandler fn) {
  TaggedEventHandler handler(id, owner, std::move(fn));
  if (dispatch_count_ > 0) {
    command_queue_.emplace_back(type, std::move(handler));
  } else {
    AddImpl(type, std::move(handler));
  }
}

void Dispatcher::EventHandlerMap::Remove(TypeId type, ConnectionId id,
                                         const void* owner) {
  TaggedEventHandler handler(id, owner, nullptr);
  if (dispatch_count_ > 0) {
    command_queue_.emplace_back(type, std::move(handler));
  } else {
    RemoveImpl(type, std::move(handler));
  }
}

void Dispatcher::EventHandlerMap::AddImpl(TypeId type,
                                          TaggedEventHandler handler) {
  assert(handler.id != 0);
  assert(handler.fn != nullptr);
  map_.emplace(type, std::move(handler));
}

void Dispatcher::EventHandlerMap::RemoveImpl(TypeId type,
                                             TaggedEventHandler handler) {
  assert(handler.fn == nullptr);
  assert(handler.id != 0 || handler.owner != nullptr);

  auto range = std::make_pair(map_.begin(), map_.end());
  if (type != 0) {
    range = map_.equal_range(type);
  }

  if (handler.id) {
    for (auto it = range.first; it != range.second; ++it) {
      if (it->second.id == handler.id) {
        map_.erase(it);
        return;
      }
    }
  } else if (handler.owner) {
    for (auto it = range.first; it != range.second;) {
      if (it->second.owner == handler.owner) {
        it = map_.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void Dispatcher::EventHandlerMap::Dispatch(const EventWrapper& event) {
  // NOTE: if you crash in this function, it may be because you destroyed an
  // an Entity from inside an event handler.
  // Call EntityFactory::QueueForDestruction instead.
  const TypeId type = event.GetTypeId();

  ++dispatch_count_;
  auto range = map_.equal_range(type);
  for (auto it = range.first; it != range.second; ++it) {
    it->second.fn(event);
  }
  // Send to handlers that are listening for all events.
  range = map_.equal_range(0);
  for (auto it = range.first; it != range.second; ++it) {
    it->second.fn(event);
  }
  --dispatch_count_;

  if (dispatch_count_ == 0) {
    for (auto& cmd : command_queue_) {
      // A non-null EventHandler implies that the operation is a remove.
      if (cmd.second.fn) {
        AddImpl(cmd.first, std::move(cmd.second));
      } else {
        RemoveImpl(cmd.first, std::move(cmd.second));
      }
    }
    command_queue_.clear();
  }
}

size_t Dispatcher::EventHandlerMap::Size() const { return map_.size(); }

size_t Dispatcher::EventHandlerMap::GetHandlerCount(TypeId type) const {
  return map_.count(type);
}

}  // namespace lull
