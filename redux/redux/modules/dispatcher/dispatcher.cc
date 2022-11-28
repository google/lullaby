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

#include "redux/modules/dispatcher/dispatcher.h"

#include <vector>

#include "absl/container/flat_hash_map.h"

namespace redux {

// Stores a map of TypeId to Handlers that is used by the Dispatcher for
// sending events.
//
// The individual handlers in the map can be invoked by the Dispatch() function.
// Adding and removing handlers during a Dispatch() call is safe as these
// operations are cached into a "command queue" which is then processed when the
// dispatching is complete. As a result, any Handler added during Dispatch()
// will not be invoked.
//
// This class is not thread-safe. All calls to an instance of this class must
// be done synchronously.s
class Dispatcher::HandlerMap {
 public:
  HandlerMap() = default;

  // Associates an Handler with the specified event `type`.
  void Add(TypeId type, ConnectionId id, const void* owner, MessageHandler fn);

  // Removes the Handler with the given `id`. While the `type` isn't
  // strictly necessary, it does improve the performance of the remove
  // operation.
  void RemoveByConnection(TypeId type, ConnectionId id);

  // Removes all the Handler that are owned by the given `owner`.
  void RemoveByOwner(const void* owner);

  // Removes the Handler of the given `type` that is owned by the `owner`.
  void RemoveByTypeAndOwner(TypeId type, const void* owner);

  // Pass the `event` to all EventHandlers associated with the same TypeId as
  // the `event`.
  void Dispatch(const Message& msg);

  // Returns the total number of handlers stored in the map.
  std::size_t GetTotalCount() const;

  // Returns the number of handlers for the given `type` stored in the map.
  std::size_t GetCount(TypeId type) const;

 private:
  // The TaggedHandler serves two purposes. Its primary purpose is as the actual
  // object stored in the map. In this case, it stores the Handler function that
  // will be invoked on Dispatch, as well as information about its connection
  // and ownership.
  //
  // It also acts as the "operation" that will be performed on the map. If it
  // has a non-null Handler, then it is intended to be added to the map. If it
  // has a null Handler, then any items in the map that match the ownership or
  // connection information are to be removed.
  struct TaggedHandler {
    TaggedHandler(ConnectionId id, const void* owner,
                  MessageHandler fn = nullptr)
        : id(id), owner(owner), fn(std::move(fn)) {}

    ConnectionId id;
    const void* owner;
    MessageHandler fn;
  };

  using HandlerList = std::vector<TaggedHandler>;

  void ApplyOp(TypeId type, TaggedHandler handler);
  void Add(TypeId type, TaggedHandler handler);
  void RemoveById(TypeId type, TaggedHandler handler);
  void RemoveByOwner(TypeId type, TaggedHandler handler);

  // The mapping of TypeId to a list of Handlers.
  absl::flat_hash_map<TypeId, HandlerList> map_;

  // Queues up add/remove commands while a Dispatch() call is in progress.
  std::vector<std::pair<TypeId, TaggedHandler>> command_queue_;

  // Tracks the total number of registered handlers.
  int handler_count_ = 0;

  // Tracks the number of times we're in a Dispatch() call.
  int dispatch_count_ = 0;
};

void Dispatcher::HandlerMap::Add(TypeId type, ConnectionId id,
                                 const void* owner, MessageHandler fn) {
  ApplyOp(type, TaggedHandler(id, owner, std::move(fn)));
}

void Dispatcher::HandlerMap::RemoveByConnection(TypeId type, ConnectionId id) {
  ApplyOp(type, TaggedHandler(id, nullptr));
}

void Dispatcher::HandlerMap::RemoveByOwner(const void* owner) {
  ApplyOp(0, TaggedHandler(0, owner));
}

void Dispatcher::HandlerMap::RemoveByTypeAndOwner(TypeId type,
                                                  const void* owner) {
  ApplyOp(type, TaggedHandler(0, owner));
}

void Dispatcher::HandlerMap::ApplyOp(TypeId type, TaggedHandler handler) {
  if (dispatch_count_ > 0) {
    command_queue_.emplace_back(type, std::move(handler));
  } else if (handler.fn) {
    Add(type, std::move(handler));
  } else if (handler.id) {
    RemoveById(type, std::move(handler));
  } else {
    RemoveByOwner(type, std::move(handler));
  }
}

void Dispatcher::HandlerMap::Add(TypeId type, TaggedHandler handler) {
  CHECK(handler.fn != nullptr);
  CHECK_NE(handler.id, 0);
  ++handler_count_;
  map_[type].emplace_back(std::move(handler));
}

void Dispatcher::HandlerMap::RemoveById(TypeId type, TaggedHandler handler) {
  CHECK(handler.fn == nullptr);

  auto iter = (type == 0) ? map_.begin() : map_.find(type);
  while (iter != map_.end()) {
    for (auto it = iter->second.begin(); it != iter->second.end(); ++it) {
      if (it->id == handler.id) {
        iter->second.erase(it);
        --handler_count_;
        break;
      }
    }

    if (iter->second.empty()) {
      map_.erase(iter++);
    } else {
      ++iter;
    }
    if (type != 0) {
      break;
    }
  }
}

void Dispatcher::HandlerMap::RemoveByOwner(TypeId type, TaggedHandler handler) {
  auto iter = (type == 0) ? map_.begin() : map_.find(type);
  while (iter != map_.end()) {
    for (auto it = iter->second.begin(); it != iter->second.end();) {
      if (it->owner == handler.owner) {
        it = iter->second.erase(it);
        --handler_count_;
      } else {
        ++it;
      }
    }

    if (iter->second.empty()) {
      map_.erase(iter++);
    } else {
      ++iter;
    }
    if (type != 0) {
      break;
    }
  }
}

void Dispatcher::HandlerMap::Dispatch(const Message& msg) {
  const TypeId type = msg.GetTypeId();

  ++dispatch_count_;
  auto iter = map_.find(type);
  if (iter != map_.end()) {
    for (auto& handler : iter->second) {
      handler.fn(msg);
    }
  }
  // Send to handlers that are listening for all events.
  iter = map_.find(0);
  if (iter != map_.end()) {
    for (auto& handler : iter->second) {
      handler.fn(msg);
    }
  }
  --dispatch_count_;

  if (dispatch_count_ == 0) {
    for (auto& cmd : command_queue_) {
      ApplyOp(cmd.first, std::move(cmd.second));
    }
    command_queue_.clear();
  }
}

std::size_t Dispatcher::HandlerMap::GetTotalCount() const {
  return handler_count_;
}

std::size_t Dispatcher::HandlerMap::GetCount(TypeId type) const {
  auto iter = map_.find(type);
  return iter != map_.end() ? iter->second.size() : 0;
}

Dispatcher::Dispatcher() { handlers_ = std::make_shared<HandlerMap>(); }

Dispatcher::ScopedConnection Dispatcher::Connect(TypeId type,
                                                 MessageHandler handler) {
  return ConnectImpl(type, nullptr, std::move(handler));
}

Dispatcher::Connection Dispatcher::Connect(TypeId type, const void* owner,
                                           MessageHandler handler) {
  return ConnectImpl(type, owner, std::move(handler));
}

Dispatcher::ScopedConnection Dispatcher::ConnectToAll(MessageHandler handler) {
  return ConnectImpl(0, nullptr, std::move(handler));
}

void Dispatcher::Disconnect(TypeId type, const void* owner) {
  DisconnectImpl(type, owner);
}

void Dispatcher::SendImpl(const Message& msg) {
  // Save the pointer to the handler map in case the dispatcher is destroyed
  // during the Dispatch call.
  std::shared_ptr<HandlerMap> ptr = handlers_;
  ptr->Dispatch(msg);
}

Dispatcher::Connection Dispatcher::ConnectImpl(TypeId type, const void* owner,
                                               MessageHandler handler) {
  const ConnectionId id = ++id_;
  handlers_->Add(type, id, owner, std::move(handler));
  return Connection(type, id, handlers_);
}

void Dispatcher::DisconnectAll(const void* owner) {
  handlers_->RemoveByOwner(owner);
}

void Dispatcher::DisconnectImpl(TypeId type, const void* owner) {
  handlers_->RemoveByTypeAndOwner(type, owner);
}

std::size_t Dispatcher::GetTotalConnectionCount() const {
  return handlers_->GetTotalCount();
}

std::size_t Dispatcher::GetConnectionCount(TypeId type) const {
  return handlers_->GetCount(type);
}

void Dispatcher::Connection::Disconnect() {
  if (auto handlers = handlers_.lock()) {
    handlers->RemoveByConnection(type_, id_);
    handlers_.reset();
  }
}

Dispatcher::ScopedConnection::ScopedConnection(ScopedConnection&& rhs)
    : connection_(std::exchange(rhs.connection_, {})) {}

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

}  // namespace redux
