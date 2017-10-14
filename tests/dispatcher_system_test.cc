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

#include "lullaby/systems/dispatcher/dispatcher_system.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;

template <typename T>
void AddVariant(EventDefT* def, const std::string& key,
                const decltype(T::value) & value) {
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
 public:
  void SetUp() override {
    registry_.Create<FunctionBinder>(&registry_);
    registry_.Create<Dispatcher>();
    registry_.Create<DispatcherSystem>(&registry_);
    dispatcher_ = registry_.Get<DispatcherSystem>();
  }

  void TearDown() override { DispatcherSystem::DisableQueuedDispatch(); }

 protected:
  DispatcherSystem* dispatcher_;
  Registry registry_;
};

using DispatcherSystemDeathTest = DispatcherSystemTest;

TEST_F(DispatcherSystemTest, NullEntity) {
  HandlerClass h;
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_->Connect(kNullEntity, &h,
                       [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_->Send(kNullEntity, e);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_->Disconnect<EventClass>(kNullEntity, &h);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));
}

TEST_F(DispatcherSystemTest, NoConnections) {
  const Entity entity = Hash("test");

  HandlerClass h;
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));
}

TEST_F(DispatcherSystemTest, StaticFunction) {
  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, MemberFunction) {
  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_->Connect(entity, &h,
                       [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(0));
}

TEST_F(DispatcherSystemTest, MultiFunction) {
  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_->Connect(entity, &h,
                       [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, Disconnect) {
  const Entity entity = Hash("test");

  HandlerClass h;
  dispatcher_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_->Connect(entity, &h,
                       [&](const EventClass& e) { h.HandleEvent(e); });
  EXPECT_THAT(h.value, Eq(0));
  EXPECT_THAT(h.static_value, Eq(0));

  EventClass e(123);
  dispatcher_->Send(entity, e);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));

  dispatcher_->Disconnect<EventClass>(entity, &h);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));

  EventClass e2(456);
  dispatcher_->Send(entity, e2);
  EXPECT_THAT(h.value, Eq(e.value));
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, MultipleEntities) {
  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  HandlerClass h1;
  HandlerClass h2;
  dispatcher_->Connect(entity1, &h1, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_->Connect(entity1, &h1,
                       [&](const EventClass& e) { h1.HandleEvent(e); });
  dispatcher_->Connect(entity2, &h2,
                       [&](const EventClass& e) { h2.HandleEvent(e); });
  EXPECT_THAT(h1.value, Eq(0));
  EXPECT_THAT(h1.static_value, Eq(0));
  EXPECT_THAT(h2.value, Eq(0));
  EXPECT_THAT(h2.static_value, Eq(0));

  EventClass e1(123);
  dispatcher_->Send(entity1, e1);
  EventClass e2(234);
  dispatcher_->Send(entity2, e2);
  EXPECT_THAT(h1.value, Eq(e1.value));
  EXPECT_THAT(h2.value, Eq(e2.value));
  EXPECT_THAT(h1.static_value, Eq(e1.value));
  EXPECT_THAT(h2.static_value, Eq(e1.value));

  dispatcher_->Disconnect<EventClass>(entity1, &h1);
  EventClass e3(456);
  dispatcher_->Send(entity1, e3);
  EXPECT_THAT(h1.value, Eq(e1.value));
  EXPECT_THAT(h2.value, Eq(e2.value));
  EXPECT_THAT(h1.static_value, Eq(e1.value));
  EXPECT_THAT(h2.static_value, Eq(e1.value));
}

TEST_F(DispatcherSystemTest, MultipleEntitiesQueued) {
  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");
  DispatcherSystem::EnableQueuedDispatch();

  HandlerClass h1;
  HandlerClass h2;
  std::vector<Entity> order;
  dispatcher_->Connect(entity1, &h1, [&](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  dispatcher_->Connect(entity1, &h1, [&](const EventClass& e) {
    h1.HandleEvent(e);
    order.emplace_back(entity1);
  });
  dispatcher_->Connect(entity2, &h2, [&](const EventClass& e) {
    h2.HandleEvent(e);
    order.emplace_back(entity2);
  });

  EXPECT_THAT(h1.value, Eq(0));
  EXPECT_THAT(h1.static_value, Eq(0));

  EventClass e1(123);
  dispatcher_->Send(entity1, e1);

  EventClass e2(234);
  dispatcher_->Send(entity2, e2);

  EXPECT_THAT(h1.value, Eq(0));
  EXPECT_THAT(h1.static_value, Eq(0));

  dispatcher_->Dispatch();

  EXPECT_THAT(h1.value, Eq(e1.value));
  EXPECT_THAT(h2.value, Eq(e2.value));
  EXPECT_THAT(h1.static_value, Eq(e1.value));
  EXPECT_THAT(h2.static_value, Eq(e1.value));
  EXPECT_THAT(order, ElementsAre(entity1, entity2));

  dispatcher_->Send(entity2, e2);
  dispatcher_->Send(entity1, e1);

  dispatcher_->Dispatch();

  EXPECT_THAT(order, ElementsAre(entity1, entity2, entity2, entity1));
}

TEST_F(DispatcherSystemTest, SendImmediately) {
  const Entity entity = Hash("test");
  DispatcherSystem::EnableQueuedDispatch();

  HandlerClass h;
  dispatcher_->Connect(entity, &h, [](const EventClass& e) {
    HandlerClass::StaticHandleEvent(e);
  });
  EventClass e(123);

  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_->Send(entity, e);
  EXPECT_THAT(h.static_value, Eq(0));

  dispatcher_->SendImmediately(entity, e);
  EXPECT_THAT(h.static_value, Eq(e.value));
}

TEST_F(DispatcherSystemTest, EventWrapper) {
  const TypeId event_type_id = 123;
  const Entity entity = Hash("test");

  int count = 0;
  auto conn = dispatcher_->Connect(entity, event_type_id,
                                   [&](const EventWrapper& e) { ++count; });

  dispatcher_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));

  conn.Disconnect();

  dispatcher_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, OwnedEventWrapper) {
  const TypeId event_type_id = 123;
  const Entity entity = Hash("test");

  int count = 0;
  dispatcher_->Connect(entity, event_type_id, dispatcher_,
                       [&](const EventWrapper& e) { ++count; });

  dispatcher_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Disconnect(entity, event_type_id, dispatcher_);

  dispatcher_->Send(entity, EventWrapper(event_type_id));
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemDeathTest, NullEventDef) {
  const Entity entity = Hash("test");
  PORT_EXPECT_DEBUG_DEATH(
      dispatcher_->ConnectEvent(entity, nullptr, [](const EventWrapper& e) {}),
      "");
}

TEST_F(DispatcherSystemTest, EventDefs) {
  const bool local = true;
  const bool global = false;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_->ConnectEvent(entity, def,
                            [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  EventWrapper other_event(Hash("OtherEvent"));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(1));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(entity, other_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(2));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Destroy(entity);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(2));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, GlobalEventDef) {
  const bool local = false;
  const bool global = true;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_->ConnectEvent(entity, def,
                            [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  EventWrapper other_event(Hash("OtherEvent"));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(0));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(entity, other_event);
  EXPECT_THAT(count, Eq(1));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(1));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Destroy(entity);
  EXPECT_THAT(count, Eq(2));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, LocalAndGlobalEventDef) {
  const bool local = true;
  const bool global = true;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_->ConnectEvent(entity, def,
                            [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  EventWrapper other_event(Hash("OtherEvent"));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(1));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(entity, other_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(3));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(4));

  dispatcher_->Destroy(entity);
  EXPECT_THAT(count, Eq(4));

  registry_.Get<Dispatcher>()->Send(test_event);
  EXPECT_THAT(count, Eq(4));

  dispatcher_->Send(entity, test_event);
  EXPECT_THAT(count, Eq(4));
}

TEST_F(DispatcherSystemTest, SendEvent) {
  const bool local = true;
  const bool global = true;
  const char* event_id = "TestEvent";
  const Entity entity = Hash("test");

  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateEventDefDirect(fbb, event_id, local, global));
  const EventDef* def = flatbuffers::GetRoot<EventDef>(fbb.GetBufferPointer());

  int count = 0;
  dispatcher_->ConnectEvent(entity, def,
                            [&](const EventWrapper& e) { ++count; });

  EventWrapper test_event(Hash(event_id));
  SendEvent(&registry_, entity, test_event);
  EXPECT_THAT(count, Eq(2));

  dispatcher_->Destroy(entity);
  SendEvent(&registry_, entity, test_event);
  EXPECT_THAT(count, Eq(2));
}

TEST_F(DispatcherSystemTest, NullSendEventDefs) {
  const Entity entity = Hash("test");
  const EventDefArray* events = nullptr;
  SendEventDefs(&registry_, entity, events);
}

TEST_F(DispatcherSystemTest, NullSendEventDefsImmediately) {
  const Entity entity = Hash("test");
  const EventDefArray* events = nullptr;
  SendEventDefsImmediately(&registry_, entity, events);
}

TEST_F(DispatcherSystemTest, NullConnectEventDefs) {
  const Entity entity = Hash("test");
  const EventDefArray* events = nullptr;
  ConnectEventDefs(&registry_, entity, events, [](const EventWrapper& e) {});
}

TEST_F(DispatcherSystemTest, EventResponseDef) {
  const Entity entity = Hash("test");

  int count = 0;
  const HashValue id = Hash("OutputEvent");
  auto conn = dispatcher_->Connect(entity, id,
                                   [&](const EventWrapper& event) { ++count; });

  const EventWrapper event(Hash("InputEvent"));
  dispatcher_->Send(entity, event);
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

  dispatcher_->CreateComponent(entity, blueprint);
  dispatcher_->Send(entity, event);
  EXPECT_THAT(count, Eq(1));
}

TEST_F(DispatcherSystemDeathTest, EmptyEventResponseDef) {
  const Entity entity = Hash("test");

  EventResponseDefT response;
  const Blueprint blueprint(&response);
  PORT_EXPECT_DEBUG_DEATH(dispatcher_->CreateComponent(entity, blueprint), "");
}

TEST_F(DispatcherSystemTest, SendEventDefs) {
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
  DispatcherSystem::EnableQueuedDispatch();
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
  EXPECT_THAT(count, Eq(3));

  dispatcher_->Dispatch();
  EXPECT_THAT(count, Eq(4));
}

TEST_F(DispatcherSystemTest, SendViaFunctionBinder) {
  const Entity entity = Hash("test");
  int x = 0;

  dispatcher_->Connect(entity, this,
                       [&x](const EventClass& e) { x = e.value; });
  EXPECT_THAT(x, Eq(0));

  EventClass e(123);
  EventWrapper wrap(e);
  registry_.Get<FunctionBinder>()->Call("lull.Dispatcher.Send", entity, wrap);
  EXPECT_THAT(x, Eq(123));
}

TEST_F(DispatcherSystemTest, EventResponseDef_Values) {
  const Entity entity = Hash("test");

  int count = 0;
  const HashValue id = Hash("OutputEvent");
  auto conn = dispatcher_->Connect(entity, id, [&](const EventWrapper& event) {
    ++count;

    EXPECT_THAT(*event.GetValue<bool>(Hash("bool_key")), Eq(true));
    EXPECT_THAT(*event.GetValue<int>(Hash("int_key")), Eq(123));
    EXPECT_THAT(*event.GetValue<float>(Hash("float_key")), Eq(456.f));
    EXPECT_THAT(*event.GetValue<std::string>(Hash("string_key")), Eq("hello"));
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
    EXPECT_THAT(*event.GetValue<HashValue>(Hash("self_key")), Eq(Hash("test")));
  });

  const EventWrapper event(Hash("InputEvent"));
  dispatcher_->Send(entity, event);
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

  dispatcher_->CreateComponent(entity, blueprint);
  dispatcher_->Send(entity, event);
  EXPECT_THAT(count, Eq(1));
}

TEST_F(DispatcherSystemTest, ConnectToAll) {
  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  int count_all = 0;
  int count_local = 0;
  auto c1 = dispatcher_->ConnectToAll(
      [&](const DispatcherSystem::EntityEvent& event) { ++count_all; });
  auto c2 = dispatcher_->Connect(entity1,
                                 [&](const EventClass& e) { ++count_local; });

  EXPECT_EQ(static_cast<size_t>(1),
            dispatcher_->GetHandlerCount(entity1, GetTypeId<EventClass>()));
  EXPECT_EQ(static_cast<size_t>(1), dispatcher_->GetUniversalHandlerCount());

  EXPECT_EQ(count_all, 0);
  EXPECT_EQ(count_local, 0);

  EventClass event(123);

  dispatcher_->Send(entity1, event);

  EXPECT_EQ(count_all, 1);
  EXPECT_EQ(count_local, 1);

  dispatcher_->Send(entity2, event);

  EXPECT_EQ(count_all, 2);
  EXPECT_EQ(count_local, 1);

  c2.Disconnect();

  dispatcher_->Send(entity1, event);

  EXPECT_EQ(count_all, 3);
  EXPECT_EQ(count_local, 1);

  c1.Disconnect();

  dispatcher_->Send(entity1, event);

  EXPECT_EQ(count_all, 3);
  EXPECT_EQ(count_local, 1);
}

TEST_F(DispatcherSystemTest, DestroyEntityInEvent) {
  const Entity entity1 = Hash("test");

  bool first_called = false;
  bool second_called = false;
  auto c1 = dispatcher_->Connect(
      entity1, [&](const EventClass& e) { first_called = true; });
  auto c2 = dispatcher_->Connect(
      entity1, [&](const EventClass& e) { dispatcher_->Destroy(entity1); });
  auto c3 = dispatcher_->Connect(
      entity1, [&](const EventClass& e) { second_called = false; });

  EventClass e(123);
  dispatcher_->Send(entity1, e);

  EXPECT_TRUE(first_called);
  EXPECT_FALSE(second_called);
  EXPECT_EQ(size_t{0},
            dispatcher_->GetHandlerCount(entity1, GetTypeId<EventClass>()));
}

TEST_F(DispatcherSystemTest, DisconnectThenConnectSelfWithinEvent) {
  DispatcherSystem::EnableQueuedDispatch();
  const Entity entity = Hash("test");

  const HashValue event_hash = Hash("TestEvent");
  const HashValue event_hash2 = Hash("TestEvent2");

  bool added_event_called = false;
  bool removed_event_called = false;

  dispatcher_->Connect(entity, event_hash2, this, [&](const EventWrapper& e) {
    removed_event_called = true;
  });
  dispatcher_->Connect(entity, event_hash, this, [&](const EventWrapper& e) {
    // Remove all connected event handlers, which will cause dispatcher to be
    // queued for destruction.
    dispatcher_->Disconnect(entity, event_hash, this);
    dispatcher_->Disconnect(entity, event_hash2, this);

    // Reconnect event handlers
    dispatcher_->Connect(entity, event_hash2, this, [&](const EventWrapper& e) {
      added_event_called = true;
    });
  });

  dispatcher_->Send(entity, EventWrapper(event_hash));
  dispatcher_->Send(entity, EventWrapper(event_hash2));
  dispatcher_->Dispatch();

  EXPECT_FALSE(removed_event_called);
  EXPECT_TRUE(added_event_called);
  EXPECT_EQ(size_t{0}, dispatcher_->GetHandlerCount(entity, event_hash));
  EXPECT_EQ(size_t{1}, dispatcher_->GetHandlerCount(entity, event_hash2));
}

TEST_F(DispatcherSystemTest, DisconnectThenConnectOtherEntityWithinEvent) {
  DispatcherSystem::EnableQueuedDispatch();
  const Entity entity1 = Hash("test");
  const Entity entity2 = Hash("test2");

  const HashValue event_hash = Hash("TestEvent");

  bool added_event_called = false;
  bool removed_event_called = false;

  dispatcher_->Connect(entity2, event_hash, this, [&](const EventWrapper& e) {
    removed_event_called = true;
  });
  dispatcher_->Connect(entity1, event_hash, this, [&](const EventWrapper& e) {
    // Remove all connected event handlers, which will cause dispatcher to be
    // queued for destruction.
    dispatcher_->Disconnect(entity2, event_hash, this);

    // Reconnect event handlers
    dispatcher_->Connect(entity2, event_hash, this, [&](const EventWrapper& e) {
      added_event_called = true;
    });
  });

  dispatcher_->Send(entity1, EventWrapper(event_hash));
  dispatcher_->Send(entity2, EventWrapper(event_hash));
  dispatcher_->Dispatch();

  EXPECT_FALSE(removed_event_called);
  EXPECT_TRUE(added_event_called);
  EXPECT_EQ(size_t{1}, dispatcher_->GetHandlerCount(entity1, event_hash));
  EXPECT_EQ(size_t{1}, dispatcher_->GetHandlerCount(entity2, event_hash));
}

}  // namespace
}  // namespace lull
