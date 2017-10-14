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

#include <chrono>            
#include <condition_variable>
#include <mutex>             
#include <thread>            
#include <unordered_set>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/async_processor.h"

namespace lull {
namespace {

using ::testing::Eq;

constexpr int kNumObjects = 10;

struct TestObject {
  TestObject() : value(0) {}
  int value;
};
typedef std::shared_ptr<TestObject> TestObjectPtr;

void WaitForNJobs(AsyncProcessor<TestObjectPtr>* processor, int num_jobs) {
  int count = 0;
  while (count < num_jobs) {
    TestObjectPtr res;
    if (processor->Dequeue(&res)) {
      ++count;
    }
  }
}

void StopAndDrainCompletedQueue(AsyncProcessor<TestObjectPtr>* processor) {
  processor->Stop();

  TestObjectPtr ptr;
  while (processor->Dequeue(&ptr)) {}
}

TEST(AsyncProcessor, OneObject) {
  AsyncProcessor<TestObjectPtr> processor;

  TestObjectPtr ptr(new TestObject());
  processor.Enqueue(ptr, [](TestObjectPtr* object) { (*object)->value = 123; });

  TestObjectPtr other;
  while (!other) {
    processor.Dequeue(&other);
  }

  EXPECT_THAT(ptr, Eq(other));
  EXPECT_THAT(ptr->value, Eq(123));
}

TEST(AsyncProcessor, MultiObject) {
  AsyncProcessor<TestObjectPtr> processor;

  int value = 0;

  std::vector<TestObjectPtr> objects;
  for (int i = 0; i < kNumObjects; ++i) {
    objects.emplace_back(new TestObject());
    processor.Enqueue(objects.back(), [&](TestObjectPtr* object) {
      (*object)->value = ++value;
    });
  }

  std::vector<TestObjectPtr> results;
  while (results.size() < kNumObjects) {
    TestObjectPtr res;
    if (processor.Dequeue(&res)) {
      results.push_back(res);
    }
  }

  EXPECT_THAT(objects.size(), Eq(static_cast<size_t>(kNumObjects)));
  EXPECT_THAT(results.size(), Eq(static_cast<size_t>(kNumObjects)));
  for (int i = 0; i < kNumObjects; ++i) {
    EXPECT_THAT(results[i], Eq(objects[i]));
    EXPECT_THAT(objects[i]->value, Eq(i + 1));
  }
}

TEST(AsyncProcessor, StopStart) {
  AsyncProcessor<TestObjectPtr> processor;

  std::mutex mu;
  std::condition_variable cv;
  bool job1_started = false;

  int value = 0;
  TestObjectPtr object1 = std::make_shared<TestObject>();
  processor.Enqueue(object1, [&](TestObjectPtr* object) {
    {
      std::lock_guard<std::mutex> lock(mu);
      job1_started = true;
    }
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    (*object)->value = ++value;
  });

  TestObjectPtr object2 = std::make_shared<TestObject>();
  processor.Enqueue(object2,
                    [&](TestObjectPtr* object) { (*object)->value = ++value; });

  // Blocks until the current job has started.
  std::unique_lock<std::mutex> lock(mu);
  cv.wait(lock, [&]() { return job1_started; });

  // Blocks until the current job has finished.
  processor.Stop();
  TestObjectPtr res;
  EXPECT_TRUE(processor.Dequeue(&res));
  EXPECT_THAT(res->value, Eq(1));

  // The second job should not have started or be ready even if we wait.
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_FALSE(processor.Dequeue(&res));

  processor.Start();
  while (!processor.Dequeue(&res)) {
  }
  EXPECT_THAT(res->value, Eq(2));
}

TEST(AsyncProcessor, Cancel) {
  using TaskId = AsyncProcessor<TestObjectPtr>::TaskId;

  AsyncProcessor<TestObjectPtr> processor;

  // First test the simple case of cancelling a pending task.
  processor.Stop();
  TaskId task_to_cancel =
      processor.Enqueue(nullptr, [](TestObjectPtr* object) { LOG(FATAL); });
  EXPECT_TRUE(processor.Cancel(task_to_cancel));

  // Next test that it doesn't cancel or reorder other tasks.
  TestObjectPtr object = std::make_shared<TestObject>();
  processor.Enqueue(object,
                    [](TestObjectPtr* object) { (*object)->value = 1; });

  task_to_cancel =
      processor.Enqueue(nullptr, [](TestObjectPtr* object) { LOG(FATAL); });

  processor.Enqueue(object, [](TestObjectPtr* object) {
    (*object)->value = 2;
  });

  EXPECT_TRUE(processor.Cancel(task_to_cancel));

  processor.Start();
  WaitForNJobs(&processor, 2);
  StopAndDrainCompletedQueue(&processor);
  EXPECT_EQ(object->value, 2);

  // Finally, test that cancelling unknown, executing, executed, or already
  // cancelled tasks returns false.
  EXPECT_FALSE(processor.Cancel(AsyncProcessor<TestObjectPtr>::kInvalidTaskId));

  task_to_cancel = processor.Enqueue(nullptr, [](TestObjectPtr*) {});
  EXPECT_TRUE(processor.Cancel(task_to_cancel));
  EXPECT_FALSE(processor.Cancel(task_to_cancel));

  volatile bool started = false;
  task_to_cancel = processor.Enqueue(nullptr, [&](TestObjectPtr*) {
    started = true;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  });
  processor.Start();
  while (!started) {}
  EXPECT_FALSE(processor.Cancel(task_to_cancel));
  StopAndDrainCompletedQueue(&processor);

  task_to_cancel = processor.Enqueue(nullptr, [](TestObjectPtr*) {});
  processor.Start();
  WaitForNJobs(&processor, 1);
  EXPECT_FALSE(processor.Cancel(task_to_cancel));
  StopAndDrainCompletedQueue(&processor);
}

TEST(AsyncProcessor, EnqueueThreadSafety) {
  using Lock = std::unique_lock<std::mutex>;
  using TaskId = AsyncProcessor<TestObjectPtr>::TaskId;

  AsyncProcessor<TestObjectPtr> processor;
  std::vector<std::thread> threads;
  std::unordered_set<TaskId> known_tasks;
  std::mutex known_tasks_mutex;

  auto thread_fn = [&]() {
    constexpr int kNumTasks = 16;
    for (int i = 0; i < kNumTasks; ++i) {
      const TaskId task = processor.Enqueue(nullptr, [](TestObjectPtr*) {});
      Lock lock(known_tasks_mutex);
      const bool found = known_tasks.find(task) != known_tasks.end();
      EXPECT_FALSE(found);
      known_tasks.emplace(task);
    }
  };

  constexpr int kNumThreads = 64;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back(thread_fn);
  }

  for (size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
}

}  // namespace
}  // namespace lull
