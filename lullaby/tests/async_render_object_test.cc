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

#include <mutex>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/systems/render/next/detail/async_render_object.h"
#include "lullaby/systems/render/next/detail/async_render_object_factory.h"
#include "lullaby/util/make_unique.h"

namespace lull {
namespace {

using ::testing::Eq;

class TestAsyncRenderObject : public detail::AsyncRenderObject {
 public:
  TestAsyncRenderObject() {}

  void TestFinish(Task deleter) { Finish(std::move(deleter)); }

  void TestRunOnMainThread(Task task) { RunOnMainThread(std::move(task)); }

  void TestRunOnRenderThread(Task task) { RunOnRenderThread(std::move(task)); }
};

class TestAsyncRenderObjectFactory : public detail::AsyncRenderObjectFactory {
 public:
  explicit TestAsyncRenderObjectFactory(const InitParams& params)
      : detail::AsyncRenderObjectFactory(params) {}

  std::shared_ptr<TestAsyncRenderObject> Create() {
    return AsyncRenderObjectFactory::Create<TestAsyncRenderObject>();
  }
};

class AsyncRenderObjectTest : public ::testing::Test {
 public:
  void SetUp() override {
    exit_threads_ = false;

    detail::AsyncRenderObjectFactory::InitParams params;
    params.async_render = true;
    factory_ = MakeUnique<TestAsyncRenderObjectFactory>(params);

    main_thread_ = std::thread([this]() {
      while (true) {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (exit_threads_) {
            break;
          }
        }
        factory_->ProcessMainThreadTasks();
      }
      factory_->ProcessMainThreadTasks();
    });
    render_thread_ = std::thread([this]() {
      while (true) {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (exit_threads_) {
            break;
          }
        }
        factory_->ProcessRenderThreadTasks();
      }
      factory_->ProcessRenderThreadTasks();
    });
    main_thread_id_ = main_thread_.get_id();
    render_thread_id_ = render_thread_.get_id();
  }

  void FlushThreads() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      exit_threads_ = true;
    }
    if (main_thread_.joinable()) {
      main_thread_.join();
    }
    if (render_thread_.joinable()) {
      render_thread_.join();
    }
  }

  void TearDown() override { FlushThreads(); }

 protected:
  std::mutex mutex_;
  bool exit_threads_ = false;
  std::thread main_thread_;
  std::thread render_thread_;
  std::thread::id main_thread_id_;
  std::thread::id render_thread_id_;
  std::unique_ptr<TestAsyncRenderObjectFactory> factory_;
};

TEST_F(AsyncRenderObjectTest, RunOnCorrectThreads) {
  std::thread::id main_thread_id;
  std::thread::id render_thread_id;

  auto asset = factory_->Create();
  asset->TestRunOnMainThread(
      [&main_thread_id]() { main_thread_id = std::this_thread::get_id(); });
  asset->TestRunOnRenderThread(
      [&render_thread_id]() { render_thread_id = std::this_thread::get_id(); });
  asset.reset();

  FlushThreads();
  EXPECT_THAT(main_thread_id, Eq(main_thread_id_));
  EXPECT_THAT(render_thread_id, Eq(render_thread_id_));
}

TEST_F(AsyncRenderObjectTest, ReadyOnMainThread) {
  std::thread::id ready_thread_id;

  auto asset = factory_->Create();
  asset->AddReadyTask(
      [&ready_thread_id]() { ready_thread_id = std::this_thread::get_id(); });
  asset->TestFinish([]() {});

  FlushThreads();
  EXPECT_THAT(ready_thread_id, Eq(main_thread_id_));
}

TEST_F(AsyncRenderObjectTest, DeleteOnRenderThread) {
  std::thread::id delete_thread_id;

  auto asset = factory_->Create();
  asset->TestFinish(
      [&delete_thread_id]() { delete_thread_id = std::this_thread::get_id(); });
  asset.reset();

  FlushThreads();
  EXPECT_THAT(delete_thread_id, Eq(render_thread_id_));
}

}  // namespace
}  // namespace lull
