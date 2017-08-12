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
#include <string>
#include <thread>
#include <vector>
#include "gtest/gtest.h"

namespace lull {
namespace {

struct QueuedEvent {
  QueuedEvent() {}
  explicit QueuedEvent(int value) : value(value) {}
  QueuedEvent(int value, const std::string& text) : value(value), text(text) {}
  const int value = 0;
  const std::string text;
};

struct QueuedEventHandlerClass {
  QueuedEventHandlerClass() {
    // Ensure the static values are reset correctly for each test.
    static_value = 0;
    static_accumulator = 0;
  }

  void HandleEvent(const QueuedEvent& e) {
    accumulator += e.value;
    value = e.value;
    text = e.text;
  }

  static void StaticHandleEvent(const QueuedEvent& e) {
    static_value = e.value;
    static_accumulator += e.value;
  }

  int value = 0;
  int accumulator = 0;
  std::string text;
  static int static_value;
  static int static_accumulator;
};

int QueuedEventHandlerClass::static_value = 0;
int QueuedEventHandlerClass::static_accumulator = 0;

TEST(QueuedDispatcher, BaseTestNoRegisteredHandlers) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  QueuedEvent e(123);
  d.Send(e);
  d.Dispatch();

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);
}

TEST(QueuedDispatcher, StaticFunction) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;
  auto c = d.Connect([](const QueuedEvent& event) {
    QueuedEventHandlerClass::StaticHandleEvent(event);
  });

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  QueuedEvent e(123);
  d.Send(e);

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  d.Dispatch();
  EXPECT_EQ(0, h.value);
  EXPECT_EQ(e.value, h.static_value);
}

TEST(QueuedDispatcher, MemberFunction) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;
  auto c = d.Connect([&](const QueuedEvent& event) { h.HandleEvent(event); });

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  QueuedEvent e(123, "hello");
  d.Send(e);

  EXPECT_EQ(0, h.value);
  EXPECT_EQ("", h.text);
  EXPECT_EQ(0, h.static_value);

  d.Dispatch();
  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ("hello", h.text);
  EXPECT_EQ(0, h.static_value);
}

TEST(QueuedDispatcher, MultiFunction) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;
  auto c1 = d.Connect([](const QueuedEvent& event) {
    QueuedEventHandlerClass::StaticHandleEvent(event);
  });
  auto c2 = d.Connect([&](const QueuedEvent& event) { h.HandleEvent(event); });

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  QueuedEvent e(123);
  d.Send(e);

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  d.Dispatch();
  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);
}

TEST(QueuedDispatcher, Unregister) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;
  auto c1 = d.Connect([](const QueuedEvent& event) {
    QueuedEventHandlerClass::StaticHandleEvent(event);
  });
  auto c2 = d.Connect([&](const QueuedEvent& event) { h.HandleEvent(event); });

  EXPECT_EQ(static_cast<size_t>(2), d.GetHandlerCount());

  EXPECT_EQ(0, h.value);
  EXPECT_EQ(0, h.static_value);

  QueuedEvent e(123);
  d.Send(e);
  d.Dispatch();

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);

  c1.Disconnect();

  EXPECT_EQ(static_cast<size_t>(1), d.GetHandlerCount());

  c2.Disconnect();

  EXPECT_EQ(static_cast<size_t>(0), d.GetHandlerCount());

  QueuedEvent e2(456);
  d.Send(e2);
  d.Dispatch();

  EXPECT_EQ(e.value, h.value);
  EXPECT_EQ(e.value, h.static_value);
}

TEST(QueuedDispatcher, EventWrapper) {
  static const TypeId kTestTypeId = 123;

  int count = 0;
  QueuedDispatcher d;
  auto conn = d.Connect(kTestTypeId, [&](const EventWrapper& e) { ++count; });

  d.Send(EventWrapper(kTestTypeId));
  d.Dispatch();
  EXPECT_EQ(1, count);

  d.Send(EventWrapper(kTestTypeId));
  d.Dispatch();
  EXPECT_EQ(2, count);

  conn.Disconnect();

  d.Send(EventWrapper(kTestTypeId));
  d.Dispatch();
  EXPECT_EQ(2, count);
}

TEST(ThreadSafeQueue, Multithreaded) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;
  auto c1 = d.Connect([](const QueuedEvent& event) {
    QueuedEventHandlerClass::StaticHandleEvent(event);
  });
  auto c2 = d.Connect([&](const QueuedEvent& event) { h.HandleEvent(event); });

  // Create 100 threads that send the numbers 1-100 to the dispatcher.
  std::vector<std::thread> producers;
  static const int kNumProducers = 100;
  for (int i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&d]() {
      for (int j = 0; j < 100; ++j) {
        d.Send(QueuedEvent(j + 1));
      }
    });
  }
  for (auto& thread : producers) {
    thread.join();
  }

  EXPECT_EQ(0, h.accumulator);
  EXPECT_EQ(0, h.static_accumulator);

  d.Dispatch();
  EXPECT_EQ(5050 * kNumProducers, h.accumulator);  // Sum(1..100) == 5050
  EXPECT_EQ(5050 * kNumProducers, h.static_accumulator);
}

TEST(ThreadSafeQueue, MultithreadedSendWhileDispatching) {
  QueuedDispatcher d;
  QueuedEventHandlerClass h;
  auto c1 = d.Connect([](const QueuedEvent& event) {
    QueuedEventHandlerClass::StaticHandleEvent(event);
  });
  auto c2 = d.Connect([&](const QueuedEvent& event) { h.HandleEvent(event); });

  // Create 1 thread that continuously Dispatches the dispatcher while another
  // 1000 threads that send the numbers 1-100 to the dispatcher.
  static const int kNumProducers = 1000;

  int dispatch_count = 0;
  std::thread consumer([&]() {
    while (5050 * kNumProducers != h.accumulator) {
      d.Dispatch();
      ++dispatch_count;
    }
  });

  std::vector<std::thread> producers;
  for (int i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&d]() {
      for (int j = 0; j < 100; ++j) {
        d.Send(QueuedEvent(j + 1));
      }
    });
  }

  consumer.join();
  for (auto& thread : producers) {
    thread.join();
  }

  EXPECT_EQ(5050 * kNumProducers, h.accumulator);  // Sum(1..100) == 5050
  EXPECT_EQ(5050 * kNumProducers, h.static_accumulator);
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(QueuedEvent);
