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

#include "redux/modules/dispatcher/queued_dispatcher.h"

namespace redux {

void QueuedDispatcher::Dispatch() {
  while (auto msg = queue_.TryPopFront()) {
    Dispatcher::SendImpl(*msg.value());
  }
}

void QueuedDispatcher::SendImpl(const Message& msg) {
  // Copy the event in order to increase the lifetime of the event until it
  // is dispatched.  The original event can now safely go out-of-scope.
  queue_.PushBack(std::make_unique<Message>(msg));
}

}  // namespace redux
