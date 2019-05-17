/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/modules/dispatcher/dispatcher_binder.h"

#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/logging.h"

namespace lull {

DispatcherBinder::DispatcherBinder(Registry* registry) : registry_(registry) {
  auto* binder = registry->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }
  auto* dispatcher = registry->Get<Dispatcher>();
  if (!dispatcher) {
    LOG(DFATAL) << "No Dispatcher.";
    return;
  }

  binder->RegisterMethod("lull.Dispatcher.DispatchGlobal",
                         &lull::Dispatcher::Dispatch);
  void (Dispatcher::*send)(const EventWrapper&) = &lull::Dispatcher::Send;
  void (Dispatcher::*send_immediately)(const EventWrapper&) =
      &lull::Dispatcher::SendImmediately;
  void (Dispatcher::*disconnect)(TypeId, Dispatcher::ConnectionId) =
      &lull::Dispatcher::Disconnect;
  binder->RegisterMethod("lull.Dispatcher.SendGlobal", send);
  binder->RegisterMethod("lull.Dispatcher.SendGlobalImmediately",
                         send_immediately);
  binder->RegisterMethod("lull.Dispatcher.DisconnectGlobal", disconnect);
  binder->RegisterFunction(
      "lull.Dispatcher.ConnectGlobal",
      [this, dispatcher](TypeId type, Dispatcher::EventHandler handler) {
        auto connection = dispatcher->Connect(type, this, std::move(handler));
        return connection.GetId();
      });
}

Dispatcher* DispatcherBinder::CreateQueuedDispatcher(Registry* registry) {
  Dispatcher* dispatcher = new QueuedDispatcher();
  registry->Register(std::unique_ptr<Dispatcher>(dispatcher));
  registry->Create<DispatcherBinder>(registry);
  return dispatcher;
}

DispatcherBinder::~DispatcherBinder() {
  auto* binder = registry_->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->UnregisterFunction("lull.Dispatcher.DispatchGlobal");
  binder->UnregisterFunction("lull.Dispatcher.SendGlobal");
  binder->UnregisterFunction("lull.Dispatcher.SendGlobalImmediately");
  binder->UnregisterFunction("lull.Dispatcher.DisconnectGlobal");
  binder->UnregisterFunction("lull.Dispatcher.ConnectGlobal");
}

}  // namespace lull
