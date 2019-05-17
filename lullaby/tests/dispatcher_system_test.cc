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

#include "lullaby/systems/dispatcher/dispatcher_system.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;

template <typename T>
void AddVariant(EventDefT* def, const std::string& key,
                const decltype(T::value)& value) {
  KeyVariantPairDefT pair;
  pair.key = key;
  pair.value.set<T>()->value = value;
  def->values.emplace_back(std::move(pair));
}

struct EventClass {
  EventClass() {}
  explicit EventClass(int value) : value(value) {}
  int value = 0;
};

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(EventClass);

namespace lull {
namespace {

struct HandlerClass {
  HandlerClass() : value(0) { static_value = 0; }

  void HandleEvent(const EventClass& e) { value = e.value; }

  static void StaticHandleEvent(const EventClass& e) { static_value = e.value; }

  int value;
  static int static_value;
};

int HandlerClass::static_value = 0;

class DispatcherSystemTest : public testing::Test {
 protected:
  void CreateImmediateDispatcherSystem() {
    dispatcher_ = registry_.Create<Dispatcher>();
    dispatcher_system_ = registry_.Create<DispatcherSystem>(&registry_);
    dispatcher_system_->Initialize();
  }

  void CreateDispatcherSystem() {
    dispatcher_ = new QueuedDispatcher();
    registry_.Register(std::unique_ptr<Dispatcher>(dispatcher_));
    dispatcher_system_ = registry_.Create<DispatcherSystem>(&registry_);
    dispatcher_system_->Initialize();
  }

  Registry registry_;
  Dispatcher* dispatcher_;
  DispatcherSystem* dispatcher_system_;
};

using DispatcherSystemDeathTest = DispatcherSystemTest;

TEST_F(DispatcherSystemTest, CheckDependencies) {
  registry_.Create<Dispatcher>();
  registry_.Create<DispatcherSystem>(&registry_);
  registry_.CheckAllDependencies();
  SUCCEED();
}

TEST_F(DispatcherSystemTest, CheckDependenciesQueued) {
  auto* dispatcher = new QueuedDispatcher();
  registry_.Register(std::unique_ptr<Dispatcher>(dispatcher));
  registry_.Create<DispatcherSystem>(&registry_);
  registry_.CheckAllDependencies();
  SUCCEED();
}

TEST_F(DispatcherSystemDeathTest, CheckDependenciesFail) {
  registry_.Create<DispatcherSystem>(&registry_);
  PORT_EXPECT_DEBUG_DEATH(registry_.CheckAllDependencies(), "");
}

TEST_F(DispatcherSystemTest, NullEntity) {
  CreateImmediateDispatcherSystem();

  HandlerClass h;
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_system_->Connect(kNullEntity, &h,
                              [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_system_->Send(kNullEntity, e);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_system_->Disconnect<EventClass>(kNullEntity, &h);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));
}

TEST_F(DispatcherSystemTest, NoConnections) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  HandlerClass h;
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_system_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));
}

TEST_F(DispatcherSystemTest, StaticFunction) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_system_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_system_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, MemberFunction) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_system_->Connect(entity, &h,
                              [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_system_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(0));
}

TEST_F(DispatcherSystemTest, MultiFunction) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_system_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_system_->Connect(entity, &h,
                              [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_system_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, Disconnect) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_system_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_system_->Connect(entity, &h,
                              [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_system_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));

  dispatcher_system_->Disconnect<EventClass>(entity, &h);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));

  EventClass e2(456);
  dispatcher_system_->Send(entity, e2);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, MultipleEntities) {
  CreateImmediateDispatcherSystem();

  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  HandlerClass h1;
  HandlerClass h2;
  dispatcher_system_->Connect(entity1, &h1, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_system_->Connect(entity1, &h1,
                              [&](const EventClass& e) { h1.HandleEvent(e); });
  dispatcher_system_->Connect(entity2, &h2,
                              [&](const EventClass& e) { h2.HandleEvent(e); });
  EXPECT_THAT(h1.value, Eq(0));
  EXPECT_THAT(h1.static_value, Eq(0));
  EXPECT_THAT(h2.value, Eq(0));
  EXPECT_THAT(h2.static_value, Eq(0));

  EventClass e1(123);
  dispatcher_system_->Send(entity1, e1);
  EventClass e2(234);
  dispatcher_system_->Send(entity2, e2);
  EXPECT_THAT(h1.value, Eq(e1.value));
  EXPECT_THAT(h2.value, Eq(e2.value));
  EXPECT_THAT(h1.static_value, Eq(e1.value));
  EXPECT_THAT(h2.static_value, Eq(e1.value));

  dispatcher_system_->Disconnect<EventClass>(entity1, &h1);
  EventClass e3(456);
  dispatcher_system_->Send(entity1, e3);
  EXPECT_THAT(h1.value, Eq(e1.value));
  EXPECT_THAT(h2.value, Eq(e2.value));
  EXPECT_THAT(h1.static_value, Eq(e1.value));
  EXPECT_THAT(h2.static_value, Eq(e1.value));
}

TEST_F(DispatcherSystemTest, MultipleEntitiesQueued) {
  CreateDispatcherSystem();

  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  HandlerClass h1;
  HandlerClass h2;
  std::vector<Entity> order;
  dispatcher_system_->Connect(entity1, &h1, [&](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_system_->Connect(entity1, &h1, [&](const EventClass& e) {
    h1.HandleEvent(e);
    order.emplace_back(entity1);
  });
  dispatcher_system_->Connect(entity2, &h2, [&](const EventClass& e) {
    h2.HandleEvent(e);
    order.emplace_back(entity2);
  });

  EXPECT_THAT(h1.value, Eq(0));
  EXPECT_THAT(h1.static_value, Eq(0));

  EventClass e1(123);
  dispatcher_system_->Send(entity1, e1);

  EventClass e2(234);
  dispatcher_system_->Send(entity2, e2);

  EXPECT_THAT(h1.value, Eq(0));
  EXPECT_THAT(h1.static_value, Eq(0));

  dispatcher_->Dispatch();

  EXPECT_THAT(h1.value, Eq(e1.value));
  EXPECT_THAT(h2.value, Eq(e2.value));
  EXPECT_THAT(h1.static_value, Eq(e1.value));
  EXPECT_THAT(h2.static_value, Eq(e1.value));
  EXPECT_THAT(order, ElementsAre(entity1, entity2));

  dispatcher_system_->Send(entity2, e2);
  dispatcher_system_->Send(entity1, e1);

  dispatcher_->Dispatch();

  EXPECT_THAT(order, ElementsAre(entity1, entity2, entity2, entity1));
}

TEST_F(DispatcherSystemTest, QueuedInterleaving) {
  CreateDispatcherSystem();

  const Entity entity1 = Hash("test");
  const void* owner = static_cast<const void*>(&entity1);

  std::vector<int> order;
  dispatcher_system_->Connect(entity1, owner, [&](const EventClass& e) {
    order.emplace_back(e.value);
  });
  dispatcher_->Connect(
      owner, [&](const EventClass& e) { order.emplace_back(e.value); });
  EXPECT_THAT(order, ElementsAre());

  // We should be able to send events to either dispatcher, and the order is
  // preserved.
  dispatcher_->Send(EventClass(1));
  dispatcher_system_->Send(entity1, EventClass(2));
  dispatcher_->Send(EventClass(3));
  dispatcher_system_->Send(entity1, EventClass(4));
  dispatcher_->Send(EventClass(5));
  EXPECT_THAT(order, ElementsAre());

  dispatcher_->Dispatch();
  EXPECT_THAT(order, ElementsAre(1, 2, 3, 4, 5));
}

TEST_F(DispatcherSystemTest, SendImmediately) {
  CreateDispatcherSystem();

  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_system_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  EventClass e(123);

  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_system_->Send(entity, e);
  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_system_->SendImmediately(entity, e);
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, EventWrapper) {
  CreateImmediateDispatcherSystem();

  const TypeId event_type_id = 123;
  const Entity entity = Hash("test");

  int count = 0;
  auto conn = dispatcher_system_->Connect(
      entity, event_type_id, [&](const EventWrapper& e) { ++count; });

  dispatcher_system_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(1));

  dispatcher_system_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));

  conn.Disconnect();

  dispatcher_system_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, OwnedEventWrapper) {
  CreateImmediateDispatcherSystem();

  const TypeId event_type_id = 123;
  const Entity entity = Hash("test");

  int count = 0;
  dispatcher_system_->Connect(entity, event_type_id, dispatcher_system_,
                              [&](const EventWrapper& e) { ++count; });

  dispatcher_system_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(1));

  dispatcher_system_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Disconnect(entity, event_type_id, dispatcher_system_);

  dispatcher_system_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemDeathTest, NullEventDef) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");
  PORT_EXPECT_DEBUG_DEATH(dispatcher_system_->ConnectEvent(
                              entity, nullptr, [](const EventWrapper& e) {}),
                          "");
}

TEST_F(DispatcherSystemTest, EventDefs) {
  CreateImmediateDispatcherSystem();

  const bool local = true;
  const bool global = false;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_system_->ConnectEvent(entity, def,
                                   [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  EventWrapper other_event(Hash("OtherEvent"));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_system_->Send(entity, other_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Destroy(entity);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, GlobalEventDef) {
  CreateImmediateDispatcherSystem();

  const bool local = false;
  const bool global = true;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_system_->ConnectEvent(entity, def,
                                   [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  EventWrapper other_event(Hash("OtherEvent"));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(0));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_system_->Send(entity, other_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Destroy(entity);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, LocalAndGlobalEventDef) {
  CreateImmediateDispatcherSystem();

  const bool local = true;
  const bool global = true;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_system_->ConnectEvent(entity, def,
                                   [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  EventWrapper other_event(Hash("OtherEvent"));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Send(entity, other_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(3));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(4));

  dispatcher_system_->Destroy(entity);
  EXPECT_THAT(count, Eq(4));

  dispatcher_->Send(test_event);
  EXPECT_THAT(count, Eq(4));

  dispatcher_system_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(4));
}

TEST_F(DispatcherSystemTest, SendEvent) {
  CreateImmediateDispatcherSystem();

  const bool local = true;
  const bool global = true;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_system_->ConnectEvent(entity, def,
                                   [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  SendEvent(&registry_, entity, test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_system_->Destroy(entity);
  SendEvent(&registry_, entity, test_event);
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, NullSendEventDefs) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");
  const EventDefArray* events = nullptr;
  SendEventDefs(&registry_, entity, events);
}

TEST_F(DispatcherSystemTest, NullSendEventDefsImmediately) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");
  const EventDefArray* events = nullptr;
  SendEventDefsImmediately(&registry_, entity, events);
}

TEST_F(DispatcherSystemTest, NullConnectEventDefs) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");
  const EventDefArray* events = nullptr;
  ConnectEventDefs(&registry_, entity, events, [](const EventWrapper& e) {});
}

TEST_F(DispatcherSystemTest, EventResponseDef) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  int count = 0;
  const HashValue id = Hash("OutputEvent");
  auto conn = dispatcher_system_->Connect(
      entity, id, [&](const EventWrapper& event) { ++count; });

  const EventWrapper event(Hash("InputEvent"));
  dispatcher_system_->Send(entity, event);
  EXPECT_THAT(count, Eq(0));

  EventDefT input;
  input.event = "InputEvent";
  input.local = true;
  input.global = true;

  EventDefT output;
  output.event = "OutputEvent";
  output.local = true;
  output.global = true;

  EventResponseDefT response;
  response.inputs.emplace_back(input);
  response.outputs.emplace_back(output);
  const Blueprint blueprint(&response);

  dispatcher_system_->CreateComponent(entity, blueprint);
  dispatcher_system_->Send(entity, event);
  EXPECT_THAT(count, Eq(1));
}

TEST_F(DispatcherSystemDeathTest, EmptyEventResponseDef) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  EventResponseDefT response;
  const Blueprint blueprint(&response);
  PORT_EXPECT_DEBUG_DEATH(
      dispatcher_system_->CreateComponent(entity, blueprint), "");
}

TEST_F(DispatcherSystemTest, SendEventDefs) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  EventDefT event;
  event.event = "TestEvent";
  event.local = true;
  event.global = true;

  EventResponseDefT responses;
  responses.inputs.emplace_back(event);

  InwardBuffer buffer(256);
  const void* flatbuffer = WriteFlatbuffer(&responses, &buffer);
  const auto* def = flatbuffers::GetRoot<EventResponseDef>(flatbuffer);
  auto events = def->inputs();

  int count = 0;
  auto handler = [&](const EventWrapper& e) { ++count; };
  ConnectEventDefs(&registry_, entity, events, handler);

  SendEventDefsImmediately(&registry_, entity, events);
  EXPECT_THAT(count, Eq(2));

  SendEventDefs(&registry_, entity, events);
  EXPECT_THAT(count, Eq(4));
}

TEST_F(DispatcherSystemTest, SendEventDefsImmediately) {
  CreateDispatcherSystem();

  const Entity entity = Hash("test");

  EventDefT event;
  event.event = "TestEvent";
  event.local = true;
  event.global = true;

  EventResponseDefT responses;
  responses.inputs.emplace_back(event);

  InwardBuffer buffer(256);
  const void* flatbuffer = WriteFlatbuffer(&responses, &buffer);
  const auto* def = flatbuffers::GetRoot<EventResponseDef>(flatbuffer);
  auto events = def->inputs();

  int count = 0;
  auto handler = [&](const EventWrapper& e) { ++count; };
  ConnectEventDefs(&registry_, entity, events, handler);

  SendEventDefsImmediately(&registry_, entity, events);
  EXPECT_THAT(count, Eq(2));

  // DispatcherSystem being queued means main Dispatcher must also be queued.
  SendEventDefs(&registry_, entity, events);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Dispatch();
  EXPECT_THAT(count, Eq(4));
}

TEST_F(DispatcherSystemTest, SendViaFunctionBinderLocal) {
  auto* function_binder = registry_.Create<FunctionBinder>(&registry_);
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");
  int x = 0;

  dispatcher_system_->Connect(entity, this,
                              [&x](const EventClass& e) { x = e.value; });
  EXPECT_THAT(x, Eq(0));

  EventClass e(123);
  EventWrapper wrap(e);
  function_binder->Call("lull.Dispatcher.Send", entity, wrap);
  EXPECT_THAT(x, Eq(123));
}

TEST_F(DispatcherSystemTest, SendViaFunctionBinderGlobal) {
  auto* function_binder = registry_.Create<FunctionBinder>(&registry_);
  CreateImmediateDispatcherSystem();

  int x = 0;
  dispatcher_->Connect(this, [&x](const EventClass& e) { x = e.value; });
  EXPECT_THAT(x, Eq(0));

  EventClass e(123);
  EventWrapper wrap(e);
  function_binder->Call("lull.Dispatcher.SendGlobal", wrap);
  EXPECT_THAT(x, Eq(123));
}

TEST_F(DispatcherSystemTest, EventResponseDef_Values) {
  CreateImmediateDispatcherSystem();

  const Entity entity = Hash("test");

  int count = 0;
  const HashValue id = Hash("OutputEvent");
  auto conn =
      dispatcher_system_->Connect(entity, id, [&](const EventWrapper& event) {
        ++count;

        EXPECT_THAT(*event.GetValue<bool>(Hash("bool_key")), Eq(true));
        EXPECT_THAT(*event.GetValue<int>(Hash("int_key")), Eq(123));
        EXPECT_THAT(*event.GetValue<float>(Hash("float_key")), Eq(456.f));
        EXPECT_THAT(*event.GetValue<std::string>(Hash("string_key")),
                    Eq("hello"));
        EXPECT_THAT(*event.GetValue<HashValue>(Hash("hash_key")),
                    Eq(Hash("world")));
        EXPECT_THAT(*event.GetValue<mathfu::vec2>(Hash("vec2_key")),
                    Eq(mathfu::vec2(1, 2)));
        EXPECT_THAT(*event.GetValue<mathfu::vec3>(Hash("vec3_key")),
                    Eq(mathfu::vec3(3, 4, 5)));
        EXPECT_THAT(*event.GetValue<mathfu::vec4>(Hash("vec4_key")),
                    Eq(mathfu::vec4(6, 7, 8, 9)));
        EXPECT_THAT(event.GetValue<mathfu::quat>(Hash("quat_key"))->vector(),
                    Eq(mathfu::quat(1, 0, 0, 0).vector()));
        EXPECT_THAT(event.GetValue<mathfu::quat>(Hash("quat_key"))->scalar(),
                    Eq(mathfu::quat(1, 0, 0, 0).scalar()));
        EXPECT_THAT(*event.GetValue<Entity>(Hash("self_key")),
                    Eq(Hash("test")));
      });

  const EventWrapper event(Hash("InputEvent"));
  dispatcher_system_->Send(entity, event);
  EXPECT_THAT(count, Eq(0));

  EventDefT input;
  input.event = "InputEvent";
  input.local = true;
  input.global = true;

  EventDefT output;
  output.event = "OutputEvent";
  output.local = true;
  output.global = true;

  AddVariant<DataBoolT>(&output, "bool_key", true);
  AddVariant<DataIntT>(&output, "int_key", 123);
  AddVariant<DataFloatT>(&output, "float_key", 456.f);
  AddVariant<DataStringT>(&output, "string_key", "hello");
  AddVariant<DataHashValueT>(&output, "hash_key", Hash("world"));
  AddVariant<DataVec2T>(&output, "vec2_key", mathfu::vec2(1, 2));
  AddVariant<DataVec3T>(&output, "vec3_key", mathfu::vec3(3, 4, 5));
  AddVariant<DataVec4T>(&output, "vec4_key", mathfu::vec4(6, 7, 8, 9));
  AddVariant<DataQuatT>(&output, "quat_key", mathfu::quat(1, 0, 0, 0));
  AddVariant<DataHashValueT>(&output, "self_key", Hash("$self"));

  EventResponseDefT response;
  response.inputs.emplace_back(input);
  response.outputs.emplace_back(output);
  Blueprint blueprint(&response);

  dispatcher_system_->CreateComponent(entity, blueprint);
  dispatcher_system_->Send(entity, event);
  EXPECT_THAT(count, Eq(1));
}

TEST_F(DispatcherSystemTest, ConnectToAll) {
  CreateImmediateDispatcherSystem();

  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  int count_all = 0;
  int count_local = 0;
  auto c1 = dispatcher_system_->ConnectToAll(
      [&](const DispatcherSystem::EntityEvent& event) { ++count_all; });
  auto c2 = dispatcher_system_->Connect(
      entity1, [&](const EventClass& e) { ++count_local; });

  EXPECT_EQ(static_cast<size_t>(1), dispatcher_system_->GetHandlerCount(
                                        entity1, GetTypeId<EventClass>()));
  EXPECT_EQ(static_cast<size_t>(1),
            dispatcher_system_->GetUniversalHandlerCount());

  EXPECT_EQ(count_all, 0);
  EXPECT_EQ(count_local, 0);

  EventClass event(123);

  dispatcher_system_->Send(entity1, event);

  EXPECT_EQ(count_all, 1);
  EXPECT_EQ(count_local, 1);

  dispatcher_system_->Send(entity2, event);

  EXPECT_EQ(count_all, 2);
  EXPECT_EQ(count_local, 1);

  c2.Disconnect();

  dispatcher_system_->Send(entity1, event);

  EXPECT_EQ(count_all, 3);
  EXPECT_EQ(count_local, 1);

  c1.Disconnect();

  dispatcher_system_->Send(entity1, event);

  EXPECT_EQ(count_all, 3);
  EXPECT_EQ(count_local, 1);
}

TEST_F(DispatcherSystemTest, DestroyEntityInEvent) {
  CreateImmediateDispatcherSystem();

  const Entity entity1 = Hash("test");

  bool first_called = false;
  bool second_called = false;
  auto c1 = dispatcher_system_->Connect(
      entity1, [&](const EventClass& e) { first_called = true; });
  auto c2 = dispatcher_system_->Connect(entity1, [&](const EventClass& e) {
    dispatcher_system_->Destroy(entity1);
  });
  auto c3 = dispatcher_system_->Connect(
      entity1, [&](const EventClass& e) { second_called = false; });

  EventClass e(123);
  dispatcher_system_->Send(entity1, e);

  EXPECT_TRUE(first_called);
  EXPECT_FALSE(second_called);
  EXPECT_EQ(size_t{0}, dispatcher_system_->GetHandlerCount(
                           entity1, GetTypeId<EventClass>()));
}

TEST_F(DispatcherSystemTest, DisconnectThenConnectSelfWithinEvent) {
  CreateDispatcherSystem();

  const Entity entity = Hash("test");

  const HashValue event_hash = Hash("TestEvent");
  const HashValue event_hash2 = Hash("TestEvent2");

  bool added_event_called = false;
  bool removed_event_called = false;

  dispatcher_system_->Connect(
      entity, event_hash2, this,
      [&](const EventWrapper& e) { removed_event_called = true; });
  dispatcher_system_->Connect(
      entity, event_hash, this, [&](const EventWrapper& e) {
        // Remove all connected event handlers, which will cause dispatcher to
        // be queued for destruction.
        dispatcher_system_->Disconnect(entity, event_hash, this);
        dispatcher_system_->Disconnect(entity, event_hash2, this);

        // Reconnect event handlers
        dispatcher_system_->Connect(
            entity, event_hash2, this,
            [&](const EventWrapper& e) { added_event_called = true; });
      });

  dispatcher_system_->Send(entity, EventWrapper(event_hash));
  dispatcher_system_->Send(entity, EventWrapper(event_hash2));
  dispatcher_->Dispatch();

  EXPECT_FALSE(removed_event_called);
  EXPECT_TRUE(added_event_called);
  EXPECT_EQ(size_t{0}, dispatcher_system_->GetHandlerCount(entity, event_hash));
  EXPECT_EQ(size_t{1},
            dispatcher_system_->GetHandlerCount(entity, event_hash2));
}

TEST_F(DispatcherSystemTest, DisconnectThenConnectOtherEntityWithinEvent) {
  CreateDispatcherSystem();

  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  const HashValue event_hash = Hash("TestEvent");

  bool added_event_called = false;
  bool removed_event_called = false;

  dispatcher_system_->Connect(
      entity2, event_hash, this,
      [&](const EventWrapper& e) { removed_event_called = true; });
  dispatcher_system_->Connect(
      entity1, event_hash, this, [&](const EventWrapper& e) {
        // Remove all connected event handlers, which will cause dispatcher to
        // be queued for destruction.
        dispatcher_system_->Disconnect(entity2, event_hash, this);

        // Reconnect event handlers
        dispatcher_system_->Connect(
            entity2, event_hash, this,
            [&](const EventWrapper& e) { added_event_called = true; });
      });

  dispatcher_system_->Send(entity1, EventWrapper(event_hash));
  dispatcher_system_->Send(entity2, EventWrapper(event_hash));
  dispatcher_->Dispatch();

  EXPECT_FALSE(removed_event_called);
  EXPECT_TRUE(added_event_called);
  EXPECT_EQ(size_t{1},
            dispatcher_system_->GetHandlerCount(entity1, event_hash));
  EXPECT_EQ(size_t{1},
            dispatcher_system_->GetHandlerCount(entity2, event_hash));
}

}  // namespace
}  // namespace lull
