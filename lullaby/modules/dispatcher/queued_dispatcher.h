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

#ifndef LULLABY_MODULES_DISPATCHER_QUEUED_DISPATCHER_H_
#define LULLABY_MODULES_DISPATCHER_QUEUED_DISPATCHER_H_

#include <memory>
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/util/thread_safe_queue.h"

namespace lull {

// The QueuedDispatcher is a type of Dispatcher that stores Events in a queue
// rather than "sending" them immediately.  Instead, the sending of the events
// only occurs when the QueuedDispatcher::Dispatch() member function is called.
//
// Internally, the QueuedDispatcher uses a ThreadSafeQueue for storing the
// events.  This allows Events to be sent from multiple threads simultaneously
// and allows the owner of the QueuedDispatcher to control when those Events are
// actually handled by the owning thread.
//
// On destruction, any events that have been queued but not yet dispatched will
// be lost.
class QueuedDispatcher : public Dispatcher {
 public:
  QueuedDispatcher() {}

  // Dispatches the Events in the queue to the registered handlers on the
  // calling thread.  It is expected that this function will only be called by a
  // single thread at a time.
  void Dispatch() override;

  // Reports whether the underlying queue is empty or not.
  bool Empty() const;

 private:
  // Overrides the Dispatcher base-class SendImpl function to store the
  // EventWrapper objects in the queue rather than sending them to the
  // registered handlers.
  void SendImpl(const EventWrapper& event) override;

  typedef ThreadSafeQueue<std::unique_ptr<EventWrapper>> EventQueue;
  EventQueue queue_;

  QueuedDispatcher(const QueuedDispatcher&) = delete;
  QueuedDispatcher& operator=(const QueuedDispatcher&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::QueuedDispatcher);

#endif  // LULLABY_MODULES_DISPATCHER_QUEUED_DISPATCHER_H_
