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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/systems/dispatcher/dispatcher_system.h"

namespace redux {
namespace {

struct TestEvent {
  explicit TestEvent(int value = 0) : value(value) {}
  int value = 0;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(value, ConstHash("value"));
  }
};

}  // namespace
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TestEvent);

namespace redux {
namespace {

using ::testing::ElementsAre;

class DispatcherSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    ScriptEngine::Create(&registry_);
    dispatcher_system_ = registry_.Create<DispatcherSystem>(&registry_);
    registry_.Initialize();
    script_engine_ = registry_.Get<ScriptEngine>();
    received_values_.clear();
  }

  static Message ToMessage(const TestEvent& event) {
    Message msg(kTestEventTypeId);
    msg.SetValue(ConstHash("value"), event.value);
    return msg;
  }

  void HandleMessage(const Message& m) {
    received_values_.push_back(m.ValueOr(ConstHash("value"), 0));
  }

  void HandleEvent(const TestEvent& e) { received_values_.push_back(e.value); }

  Registry registry_;
  DispatcherSystem* dispatcher_system_;
  ScriptEngine* script_engine_;

  std::vector<int> received_values_;

  static const TypeId kTestEventTypeId = GetTypeId<TestEvent>();
};

TEST_F(DispatcherSystemTest, GlobalSendNow) {
  dispatcher_system_->Connect(this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendNow(TestEvent(123));
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, GlobalSendMessageNow) {
  dispatcher_system_->Connect(kTestEventTypeId, this,
                              [this](const Message& m) { HandleMessage(m); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendNow(ToMessage(TestEvent(123)));
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, GlobalSend) {
  dispatcher_system_->Connect(this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Send(TestEvent(123));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, GlobalSendMessage) {
  dispatcher_system_->Connect(kTestEventTypeId, this,
                              [this](const Message& m) { HandleMessage(m); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Send(ToMessage(TestEvent(123)));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, GlobalDisconnect) {
  dispatcher_system_->Connect(this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendNow(TestEvent(123));
  EXPECT_THAT(received_values_, ElementsAre(123));

  dispatcher_system_->Disconnect<TestEvent>(this);

  dispatcher_system_->SendNow(TestEvent(456));
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, GlobalDisconnectMessage) {
  dispatcher_system_->Connect(kTestEventTypeId, this,
                              [this](const Message& m) { HandleMessage(m); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendNow(ToMessage(TestEvent(123)));
  EXPECT_THAT(received_values_, ElementsAre(123));

  dispatcher_system_->Disconnect(kTestEventTypeId, this);

  dispatcher_system_->SendNow(TestEvent(456));
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, EntitySendNow) {
  const Entity entity1(67);
  const Entity entity2(89);

  dispatcher_system_->Connect(entity1, this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendNow(TestEvent(12));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendToEntityNow(entity1, TestEvent(34));
  EXPECT_THAT(received_values_, ElementsAre(34));

  dispatcher_system_->SendToEntityNow(entity2, TestEvent(56));
  EXPECT_THAT(received_values_, ElementsAre(34));
}

TEST_F(DispatcherSystemTest, EntitySendMessageNow) {
  const Entity entity1(67);
  const Entity entity2(89);

  dispatcher_system_->Connect(entity1, kTestEventTypeId, this,
                              [this](const Message& m) { HandleMessage(m); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendNow(ToMessage(TestEvent(12)));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendToEntityNow(entity1, ToMessage(TestEvent(34)));
  EXPECT_THAT(received_values_, ElementsAre(34));

  dispatcher_system_->SendToEntityNow(entity2, ToMessage(TestEvent(56)));
  EXPECT_THAT(received_values_, ElementsAre(34));
}

TEST_F(DispatcherSystemTest, EntitySend) {
  const Entity entity1(67);
  const Entity entity2(89);

  dispatcher_system_->Connect(entity1, this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Send(TestEvent(12));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendToEntity(entity1, TestEvent(34));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(34));

  dispatcher_system_->SendToEntity(entity2, TestEvent(56));
  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(34));
}

TEST_F(DispatcherSystemTest, EntitySendMessage) {
  const Entity entity1(67);
  const Entity entity2(89);

  dispatcher_system_->Connect(entity1, kTestEventTypeId, this,
                              [this](const Message& m) { HandleMessage(m); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Send(ToMessage(TestEvent(12)));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendToEntity(entity1, ToMessage(TestEvent(34)));
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(34));

  dispatcher_system_->SendToEntity(entity2, ToMessage(TestEvent(56)));
  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(34));
}

TEST_F(DispatcherSystemTest, EntityDisconnect) {
  const Entity entity1(67);

  dispatcher_system_->Connect(entity1, this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendToEntityNow(entity1, TestEvent(34));
  EXPECT_THAT(received_values_, ElementsAre(34));

  dispatcher_system_->Disconnect<TestEvent>(entity1, this);

  dispatcher_system_->SendToEntityNow(entity1, TestEvent(56));
  EXPECT_THAT(received_values_, ElementsAre(34));
}

TEST_F(DispatcherSystemTest, EntityDisconnectMessage) {
  const Entity entity1(67);

  dispatcher_system_->Connect(entity1, kTestEventTypeId, this,
                              [this](const Message& m) { HandleMessage(m); });
  EXPECT_THAT(received_values_, ElementsAre());

  dispatcher_system_->SendToEntityNow(entity1, ToMessage(TestEvent(34)));
  EXPECT_THAT(received_values_, ElementsAre(34));

  dispatcher_system_->Disconnect(entity1, kTestEventTypeId, this);

  dispatcher_system_->SendToEntityNow(entity1, ToMessage(TestEvent(56)));
  EXPECT_THAT(received_values_, ElementsAre(34));
}

TEST_F(DispatcherSystemTest, InterleavedGlobalAndEntitySends) {
  const Entity entity1(67);
  const Entity entity2(89);

  std::vector<Entity> received_entities;

  dispatcher_system_->Connect(this, [&](const TestEvent& e) {
    received_entities.push_back(kNullEntity);
    HandleEvent(e);
  });
  dispatcher_system_->Connect(entity1, this, [&](const TestEvent& e) {
    received_entities.push_back(entity1);
    HandleEvent(e);
  });
  dispatcher_system_->Connect(entity2, this, [&](const TestEvent& e) {
    received_entities.push_back(entity2);
    HandleEvent(e);
  });

  dispatcher_system_->Send(TestEvent(12));
  dispatcher_system_->SendToEntity(entity1, TestEvent(34));
  dispatcher_system_->SendToEntity(entity2, TestEvent(56));
  dispatcher_system_->Send(TestEvent(78));
  dispatcher_system_->SendToEntity(entity2, TestEvent(90));

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(12, 34, 56, 78, 90));
  EXPECT_THAT(received_entities,
              ElementsAre(kNullEntity, entity1, entity2, kNullEntity, entity2));
}

TEST_F(DispatcherSystemTest, MultipleHandlers) {
  const Entity entity1(67);

  std::vector<int> order;
  dispatcher_system_->Connect(this, [&](const TestEvent& e) {
    order.push_back(1);
    HandleEvent(e);
  });
  dispatcher_system_->Connect(this, [&](const TestEvent& e) {
    order.push_back(2);
    HandleEvent(e);
  });
  dispatcher_system_->Connect(entity1, this, [&](const TestEvent& e) {
    order.push_back(3);
    HandleEvent(e);
  });
  dispatcher_system_->Connect(entity1, this, [&](const TestEvent& e) {
    order.push_back(4);
    HandleEvent(e);
  });

  dispatcher_system_->Send(TestEvent(12));
  dispatcher_system_->SendToEntity(entity1, TestEvent(34));
  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(12, 12, 34, 34));
  EXPECT_THAT(order, ElementsAre(1, 2, 3, 4));
}

TEST_F(DispatcherSystemTest, RemoveEntityDuringEvent) {
  const Entity entity1(67);

  dispatcher_system_->Connect(entity1, this,
                              [this](const TestEvent& e) { HandleEvent(e); });
  dispatcher_system_->Connect(entity1, this, [&](const TestEvent& e) {
    dispatcher_system_->Disconnect<TestEvent>(entity1, this);
    HandleEvent(e);
  });
  dispatcher_system_->Connect(entity1, this,
                              [this](const TestEvent& e) { HandleEvent(e); });

  dispatcher_system_->SendToEntity(entity1, TestEvent(12));
  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(12, 12, 12));

  dispatcher_system_->SendToEntity(entity1, TestEvent(12));
  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(12, 12, 12));
}

TEST_F(DispatcherSystemTest, DisconnectThenConnectSelfWithinEvent) {
  const TypeId event1 = ConstHash("e1").get();
  const TypeId event2 = ConstHash("e2").get();

  const Entity entity1(67);

  bool added_event_called = false;
  bool removed_event_called = false;

  dispatcher_system_->Connect(entity1, event2, this, [&](const Message& e) {
    removed_event_called = true;
  });

  dispatcher_system_->Connect(entity1, event1, this, [&](const Message& m) {
    dispatcher_system_->Disconnect(entity1, event2, this);
    dispatcher_system_->Connect(entity1, event2, this, [&](const Message& e) {
      added_event_called = true;
    });
  });

  dispatcher_system_->SendToEntity(entity1, Message(event1));
  dispatcher_system_->SendToEntity(entity1, Message(event2));
  dispatcher_system_->Dispatch();

  EXPECT_FALSE(removed_event_called);
  EXPECT_TRUE(added_event_called);
}

TEST_F(DispatcherSystemTest, DisconnectThenConnectOtherEntityWithinEvent) {
  const TypeId event1 = ConstHash("e1").get();

  const Entity entity1(67);
  const Entity entity2(89);

  bool added_event_called = false;
  bool removed_event_called = false;

  dispatcher_system_->Connect(entity2, event1, this, [&](const Message& e) {
    removed_event_called = true;
  });
  dispatcher_system_->Connect(entity1, event1, this, [&](const Message& e) {
    dispatcher_system_->Disconnect(entity2, event1, this);
    dispatcher_system_->Connect(entity2, event1, this, [&](const Message& e) {
      added_event_called = true;
    });
  });

  dispatcher_system_->SendToEntity(entity1, Message(event1));
  dispatcher_system_->SendToEntity(entity2, Message(event1));
  dispatcher_system_->Dispatch();

  EXPECT_FALSE(removed_event_called);
  EXPECT_TRUE(added_event_called);
}

TEST_F(DispatcherSystemTest, GlobalSendFromScript) {
  dispatcher_system_->Connect(this,
                              [this](const TestEvent& e) { HandleEvent(e); });

  script_engine_->RunNow(
      "(rx.Dispatcher.Send "
      "    (make-msg :redux::TestEvent "
      "        { (:value 123) }"
      "    )"
      ")");

  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(123));
}

TEST_F(DispatcherSystemTest, EntitySendFromScript) {
  const Entity entity1(67);

  dispatcher_system_->Connect(entity1, this,
                              [this](const TestEvent& e) { HandleEvent(e); });

  script_engine_->RunNow(
      "(rx.Dispatcher.SendToEntity (entity 67) "
      "    (make-msg :redux::TestEvent "
      "        { (:value 123) }"
      "    )"
      ")");
  dispatcher_system_->Dispatch();
  EXPECT_THAT(received_values_, ElementsAre(123));
}

}  // namespace
}  // namespace redux
