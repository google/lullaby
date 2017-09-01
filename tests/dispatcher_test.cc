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
#include "gtest/gtest.h"
#include "lullaby/util/common_types.h"

namespace lull {
namespace {

struct Event {
  Event() {}
  explicit Event(int value) : value(value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&value, Hash("value"));
  }

  int value = 0;
};

struct OtherEvent {
  OtherEvent() {}
  explicit OtherEvent(const std::string& value) : value(value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&value, Hash("value"));
  }

  std::string value;
};

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(Event);
LULLABY_SETUP_TYPEID(OtherEvent);

namespace lull {
namespace {

struct EventHandlerClass {
  EventHandlerClass() {
    // Ensure the static value is reset correctly for each test.
    static_value = 0;
    other_static_value = "";
  }

  void HandleEvent(const Event& e) { value = e.value; }

  static void StaticHandleEvent(const Event& e) { static_value = e.value; }

  void HandleOtherEvent(const OtherEvent& e) { other_value = e.value; }

  static void StaticHandleOtherEvent(const OtherEvent& e) {
    other_static_value = e.value;
  }

  int value = 0;
  std::string other_value;
  static int static_value;
  static std::string other_static_value;
};

int EventHandlerClass::static_value = 0;
std::string EventHandlerClass::other_static_value = "";

TEST(Dispatcher, BaseTestNoRegisteredHandlers) {
  Dispatcher d;
  EventHandlerClass h;

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  Event e(123);
  d.Send(e);

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);
}

TEST(Dispatcher, StaticFunction) {
  Dispatcher d;
  EventHandlerClass h;
  auto c = d.Connect(
      [](const Event& event) { EventHandlerClass::StaticHandleEvent(event); });

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<OtherEvent>()));


  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  Event e(123);
  d.Send(e);

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(e.value, h.static_value);
}

TEST(Dispatcher, MemberFunction) {
  Dispatcher d;
  EventHandlerClass h;
  auto c = d.Connect([&](const Event& event) { h.HandleEvent(event); });

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  Event e(123);
  d.Send(e);

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(0, h.static_value);
}

TEST(Dispatcher, MultiFunction) {
  Dispatcher d;
  EventHandlerClass h;
  auto c1 = d.Connect(
      [](const Event& event) { EventHandlerClass::StaticHandleEvent(event); });
  auto c2 = d.Connect([&](const Event& event) { h.HandleEvent(event); });

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<OtherEvent>()));

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  Event e(123);
  d.Send(e);

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);
}

TEST(Dispatcher, MultiFunctionAndEvent) {
  Dispatcher d;
  EventHandlerClass h;
  auto c1 = d.Connect(
      [](const Event& event) { EventHandlerClass::StaticHandleEvent(event); });
  auto c2 = d.Connect([&](const Event& event) { h.HandleEvent(event); });
  auto c3 = d.Connect([](const OtherEvent& event) {
    EventHandlerClass::StaticHandleOtherEvent(event);
  });
  auto c4 =
      d.Connect([&](const OtherEvent& event) { h.HandleOtherEvent(event); });

  EXPECT_EQ(static_cast<size_t>(4), d.GetHandlerCount());

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount(GetTypeId<OtherEvent>()));


  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);
  EXPECT_EQ("", h.other_value);
  EXPECT_EQ("", h.other_static_value);

  Event e(123);
  d.Send(e);

  OtherEvent e2("hello");
  d.Send(e2);

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);

  EXPECT_EQ("hello", h.other_value);
  EXPECT_EQ("hello", h.other_static_value);
}

TEST(Dispatcher, MutableFunction) {
  Dispatcher d;
  int temp = 0;  // Only used in lambda scope.
  int sum = 0;
  auto c = d.Connect([temp, &sum](const Event& event) mutable {
    temp += event.value;
    sum = temp;
  });
  EXPECT_EQ(0, sum);

  Event e(123);
  d.Send(e);
  EXPECT_EQ(123, sum);

  sum = 0;
  EXPECT_EQ(0, sum);

  d.Send(e);
  EXPECT_EQ(246, sum);
}

TEST(Dispatcher, AddRentrant) {
  Dispatcher d;
  EventHandlerClass h;

  d.Connect(&h, [&](const Event& event) {
    d.Connect(&h, [&](const Event& event) { h.HandleEvent(event); });
  });

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));


  Event e(123);
  d.Send(e);
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(0, h.value);

  d.Send(e);
  EXPECT_EQ(123, h.value);
}

TEST(Dispatcher, RemoveRentrant) {
  Dispatcher d;
  EventHandlerClass h;

  auto c1 = d.Connect([&](const Event& event) { h.HandleEvent(event); });
  auto c2 = d.Connect([&](const Event& event) { c1.Disconnect(); });

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount(GetTypeId<Event>()));

  Event e1(123);
  d.Send(e1);
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(123, h.value);

  Event e2(456);
  d.Send(e2);
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(123, h.value);
}

TEST(Dispatcher, DisconnectAfterDelete) {
  {
    Dispatcher::ScopedConnection c;
    {
      Dispatcher d;
      EventHandlerClass h;
      c = d.Connect([&](const Event& event) { h.HandleEvent(event); });

      Event e1(123);
      d.Send(e1);
      EXPECT_EQ(123, h.value);
    }
    c.Disconnect();
  }
}

TEST(Dispatcher, Disconnect) {
  Dispatcher d;
  EventHandlerClass h;
  auto c1 = d.Connect(
      [](const Event& event) { EventHandlerClass::StaticHandleEvent(event); });
  auto c2 = d.Connect([&](const Event& event) { h.HandleEvent(event); });

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount(GetTypeId<Event>()));

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  Event e(123);
  d.Send(e);

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);

  c1.Disconnect();

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));

  c2.Disconnect();

  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<Event>()));

  Event e2(456);
  d.Send(e2);

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);
}

TEST(Dispatcher, DisconnectOwner) {
  Dispatcher d;
  EventHandlerClass h;

  d.Connect(&h, [&](const Event& event) { h.HandleEvent(event); });
  d.Connect(&h, [&](const OtherEvent& event) { h.HandleOtherEvent(event); });

  EXPECT_EQ(0, h.static_value);
  EXPECT_EQ("", h.other_static_value);
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<OtherEvent>()));

  d.Send(Event(123));
  d.Send(OtherEvent("hello"));

  EXPECT_EQ(123, h.value);
  EXPECT_EQ("hello", h.other_value);
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<OtherEvent>()));

  d.Disconnect<Event>(&h);

  d.Send(Event(456));
  d.Send(OtherEvent("world"));

  EXPECT_EQ(123, h.value);
  EXPECT_EQ("world", h.other_value);
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<OtherEvent>()));

  d.Disconnect<OtherEvent>(&h);

  d.Send(Event(789));
  d.Send(OtherEvent("goodbye"));

  EXPECT_EQ(123, h.value);
  EXPECT_EQ("world", h.other_value);
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<OtherEvent>()));
}

TEST(Dispatcher, DisconnectAll) {
  Dispatcher d;
  EventHandlerClass h;

  d.Connect(&h, [&](const Event& event) { h.HandleEvent(event); });
  d.Connect(&h, [&](const OtherEvent& event) { h.HandleOtherEvent(event); });

  EXPECT_EQ(0, h.value);
  EXPECT_EQ("", h.other_value);
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<OtherEvent>()));

  d.Send(Event(123));
  d.Send(OtherEvent("hello"));

  EXPECT_EQ(123, h.value);
  EXPECT_EQ("hello", h.other_value);
  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<OtherEvent>()));

  d.DisconnectAll(&h);

  d.Send(Event(456));
  d.Send(OtherEvent("world"));

  EXPECT_EQ(123, h.value);
  EXPECT_EQ("hello", h.other_value);
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<Event>()));
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<OtherEvent>()));
}

TEST(Dispatcher, EventWrapper) {
  static const TypeId kTestTypeId = 123;

  int count = 0;
  Dispatcher d;
  auto conn = d.Connect(kTestTypeId, [&](const EventWrapper& e) { ++count; });

  d.Send(EventWrapper(kTestTypeId));
  EXPECT_EQ(1, count);

  d.Send(EventWrapper(kTestTypeId));
  EXPECT_EQ(2, count);

  conn.Disconnect();

  d.Send(EventWrapper(kTestTypeId));
  EXPECT_EQ(2, count);
}

TEST(Dispatcher, OwnedEventWrapper) {
  static const TypeId kTestTypeId = 123;

  int count = 0;
  Dispatcher d;
  d.Connect(kTestTypeId, &d, [&](const EventWrapper& e) { ++count; });

  d.Send(EventWrapper(kTestTypeId));
  EXPECT_EQ(1, count);

  d.Send(EventWrapper(kTestTypeId));
  EXPECT_EQ(2, count);

  d.Disconnect(kTestTypeId, &d);

  d.Send(EventWrapper(kTestTypeId));
  EXPECT_EQ(2, count);
}

TEST(Dispatcher, ConnectEventWrapperWithData) {
  Dispatcher d;

  const TypeId type = Hash("Event");

  int value1 = 0;
  int value2 = 0;
  int value3 = 0;
  auto c = d.Connect(type, [&](const EventWrapper& event) {
    auto ptr = event.GetValue<int>(Hash("value"));
    if (ptr) {
      value1 = *ptr;
    }
    value2 = event.GetValueWithDefault(Hash("value"), 0);
    value3 = event.GetValueWithDefault(Hash("value3"), 123);
  });

  d.Send(Event(123));
  EXPECT_EQ(123, value1);
  EXPECT_EQ(123, value2);
  EXPECT_EQ(123, value3);
}

TEST(Dispatcher, SendEventWrapperWithData) {
  Dispatcher d;

  const TypeId type = Hash("Event");

  int value = 0;
  auto c = d.Connect([&value](const Event& event) { value = event.value; });

  EventWrapper event(type);
  event.SetValue(Hash("value"), 123);
  d.Send(event);

  EXPECT_EQ(123, value);
}

TEST(Dispatcher, ConnectToAll) {
  Dispatcher d;
  EventHandlerClass h;

  auto c1 = d.Connect([&](const Event& event) { h.HandleEvent(event); });

  int value1 = 0;
  int value2 = 0;
  auto c2 = d.ConnectToAll([&](const EventWrapper& event) {
    value1 += 1;
    value2 = event.GetValueWithDefault(Hash("value"), 0);
  });

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(0));
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(GetTypeId<Event>()));


  d.Send(Event(123));
  EXPECT_EQ(1, value1);
  EXPECT_EQ(123, value2);
  EXPECT_EQ(123, h.value);

  d.Send(Event(456));
  EXPECT_EQ(2, value1);
  EXPECT_EQ(456, value2);
  EXPECT_EQ(456, h.value);

  d.Send(OtherEvent("Hello"));
  EXPECT_EQ(3, value1);
  EXPECT_EQ(0, value2);
  EXPECT_EQ(456, h.value);

  c1.Disconnect();

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount(0));
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<Event>()));

  d.Send(Event(123));
  EXPECT_EQ(4, value1);
  EXPECT_EQ(123, value2);
  EXPECT_EQ(456, h.value);

  c2.Disconnect();
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount());
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(0));
  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount(GetTypeId<Event>()));

  d.Send(Event(789));
  EXPECT_EQ(4, value1);
  EXPECT_EQ(123, value2);
  EXPECT_EQ(456, h.value);
}

}  // namespace
}  // namespace lull
