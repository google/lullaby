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

#ifndef REDUX_MODULES_DISPATCHER_QUEUED_DISPATCHER_H_
#define REDUX_MODULES_DISPATCHER_QUEUED_DISPATCHER_H_

#include <memory>

#include "redux/modules/base/thread_safe_deque.h"
#include "redux/modules/dispatcher/dispatcher.h"

namespace redux {

// The QueuedDispatcher is a type of Dispatcher that stores events in a queue
// rather than sending them immediately. Instead, the sending of the events
// only occurs when QueuedDispatcher::Dispatch() is called.
//
// Internally, the QueuedDispatcher uses a thread-safe queue for storing the
// events. This allows Events to be sent from multiple threads simultaneously
// and allows the owner of the QueuedDispatcher to control when those Events are
// actually handled by the owning thread.
//
// On destruction, any events that have been queued but not yet dispatched will
// be lost.
class QueuedDispatcher : public Dispatcher {
 public:
  QueuedDispatcher() = default;

  // Dispatches the events in the queue to the registered handlers on the
  // calling thread. It is expected that this function will only be called by a
  // single thread at a time.
  void Dispatch() override;

 private:
  using MessagePtr = std::unique_ptr<Message>;

  // Overrides the Dispatcher base-class SendImpl function to store Messages in
  // the queue rather than sending them to the registered handlers.
  void SendImpl(const Message& msg) override;

  ThreadSafeDeque<MessagePtr> queue_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::QueuedDispatcher);

#endif  // REDUX_MODULES_DISPATCHER_QUEUED_DISPATCHER_H_
