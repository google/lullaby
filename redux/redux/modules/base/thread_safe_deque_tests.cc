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

#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/thread_safe_deque.h"

namespace redux {
namespace {

using ::testing::Eq;

struct TestObject {
  explicit TestObject(int value) : value(value) {}
  int value;
};

using TestObjectPtr = std::unique_ptr<TestObject>;

TEST(ThreadSafeDeque, MultiProducerSingleConsumer) {
  static constexpr int kSentinel = -1;
  static constexpr int kNumProducers = 100;
  ThreadSafeDeque<TestObjectPtr> deque;

  // Create 100 threads that insert the numbers 1-100 into the deque.
  std::vector<std::thread> producers;
  producers.reserve(kNumProducers);
  for (int i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&deque]() {
      // Wait one second just to make sure we have started the dequeing loop
      // below.
      std::this_thread::sleep_for(std::chrono::seconds(1));
      for (int j = 0; j < 100; ++j) {
        deque.PushBack(std::make_unique<TestObject>(j + 1));
      }
      // Insert a -1 to mark the end of the insertion loop.
      deque.PushBack(std::make_unique<TestObject>(kSentinel));
    });
  }

  int end_count = 0;
  int total_count = 0;
  while (end_count < kNumProducers) {
    if (auto obj = deque.TryPopFront()) {
      if (obj.value()->value == kSentinel) {
        ++end_count;
      } else {
        total_count += obj.value()->value;
      }
    }
  }

  for (auto& thread : producers) {
    thread.join();
  }

  EXPECT_THAT(end_count, Eq(kNumProducers));
  EXPECT_THAT(total_count, Eq(5050 * kNumProducers));  // Sum(1..100) == 5050
}

TEST(ThreadSafeDeque, MultiProducerSingleConsumerWithWait) {
  static const int kSentinel = -1;
  static const int kNumProducers = 100;
  ThreadSafeDeque<TestObjectPtr> deque;

  // Create 100 threads that insert the numbers 1-100 into the deque.
  std::vector<std::thread> producers;
  producers.reserve(kNumProducers);
  for (int i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&deque]() {
      // Wait one second just to make sure we have started the dequeing loop
      // below.
      std::this_thread::sleep_for(std::chrono::seconds(1));
      for (int j = 0; j < 100; ++j) {
        deque.PushBack(std::make_unique<TestObject>(j + 1));
      }
      // Insert a -1 to mark the end of the insertion loop.
      deque.PushBack(std::make_unique<TestObject>(kSentinel));
    });
  }

  int end_count = 0;
  int total_count = 0;
  while (end_count < kNumProducers) {
    TestObjectPtr obj = deque.WaitPopFront();
    if (obj) {
      if (obj->value == kSentinel) {
        ++end_count;
      } else {
        total_count += obj->value;
      }
    }
  }

  for (auto& thread : producers) {
    thread.join();
  }

  EXPECT_THAT(end_count, Eq(kNumProducers));
  EXPECT_THAT(total_count, Eq(5050 * kNumProducers));  // Sum(1..100) == 5050
}

TEST(ThreadSafeDeque, MultiProducerMultiConsumer) {
  static const int kSentinel = -1;
  static const int kNumProducers = 100;
  static const int kNumConsumers = 20;
  ThreadSafeDeque<TestObjectPtr> deque;

  // Create 100 threads that insert the numbers 1-100 into the deque.
  std::vector<std::thread> producers;
  producers.reserve(kNumProducers);
  for (int i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&deque]() {
      // Wait one second just to make sure we have started the dequeing loop
      // below.
      std::this_thread::sleep_for(std::chrono::seconds(1));
      for (int j = 0; j < 100; ++j) {
        deque.PushBack(std::make_unique<TestObject>(j + 1));
      }
      // Insert a -1 to mark the end of the insertion loop.
      deque.PushBack(std::make_unique<TestObject>(kSentinel));
    });
  }

  std::mutex mutex;
  int end_count = 0;
  int total_count = 0;

  std::vector<std::thread> consumers;
  consumers.reserve(kNumConsumers);
  for (int i = 0; i < kNumConsumers; ++i) {
    consumers.emplace_back([&]() {
      bool done = false;
      while (!done) {
        if (auto obj = deque.TryPopFront()) {
          if (obj.value()->value == kSentinel) {
            mutex.lock();
            ++end_count;
            mutex.unlock();
          } else {
            mutex.lock();
            total_count += obj.value()->value;
            mutex.unlock();
          }
        }
        mutex.lock();
        done = end_count >= kNumProducers;
        mutex.unlock();
      }
    });
  }

  for (auto& thread : consumers) {
    thread.join();
  }
  for (auto& thread : producers) {
    thread.join();
  }

  EXPECT_THAT(end_count, Eq(kNumProducers));
  EXPECT_THAT(total_count, Eq(5050 * kNumProducers));  // Sum(1..100) == 5050
}

TEST(ThreadSafeDeque, RemoveIf) {
  ThreadSafeDeque<TestObjectPtr> deque;

  deque.PushBack(std::make_unique<TestObject>(0));
  deque.RemoveIf([](const TestObjectPtr& obj) { return obj->value == 0; });

  deque.PushBack(std::make_unique<TestObject>(0));
  deque.RemoveIf([](const TestObjectPtr& obj) { return obj->value == 1; });

  auto obj = deque.TryPopFront();
  EXPECT_TRUE(obj);

  deque.PushBack(std::make_unique<TestObject>(0));
  deque.PushBack(std::make_unique<TestObject>(1));
  deque.PushBack(std::make_unique<TestObject>(0));
  deque.PushBack(std::make_unique<TestObject>(2));
  deque.RemoveIf([](const TestObjectPtr& obj) { return obj->value == 0; });

  obj = deque.TryPopFront();
  EXPECT_TRUE(obj);
  EXPECT_THAT(obj.value()->value, Eq(1));

  obj = deque.TryPopFront();
  EXPECT_TRUE(obj);
  EXPECT_THAT(obj.value()->value, Eq(2));
}

}  // namespace
}  // namespace redux
