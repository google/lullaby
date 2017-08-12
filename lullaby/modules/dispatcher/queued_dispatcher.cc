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

#include "lullaby/modules/dispatcher/queued_dispatcher.h"

namespace lull {

void QueuedDispatcher::Dispatch() {
  std::unique_ptr<EventWrapper> event;
  while (queue_.Dequeue(&event)) {
    Dispatcher::SendImpl(*event);
  }
}

bool QueuedDispatcher::Empty() const {
  return queue_.Empty();
}

void QueuedDispatcher::SendImpl(const EventWrapper& event) {
  // Copy the event in order to increase the lifetime of the event until it
  // is dispatched.  The original event can now safely go out-of-scope.
  std::unique_ptr<EventWrapper> clone(new EventWrapper(event));
  queue_.Enqueue(std::move(clone));
}

}  // namespace lull
