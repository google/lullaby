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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_ASYNC_RENDER_OBJECT_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_ASYNC_RENDER_OBJECT_FACTORY_H_

#include <functional>
#include <memory>
#include "lullaby/util/thread_safe_deque.h"

namespace lull {
namespace detail {

/// Factory class for creating AsyncRenderObjects.
///
/// This class will take care of setting up the AsyncRenderAsset object with the
/// necessary callbacks for handling main thread/render thread functionality.
/// The RenderSystem can then execute the tasks on the correct thread by
/// calling ProcessMainThreadTasks/ProcessRenderThreadTasks.
class AsyncRenderObjectFactory {
 public:
  /// Configuration parameters for initializing the factory.
  struct InitParams {
    InitParams() {}
    bool async_render = true;
  };

  /// Constructs the factory using the specified initialization parameters.
  explicit AsyncRenderObjectFactory(const InitParams& params = InitParams());

  /// Executes all tasks that that are to be run on the main thread.  This
  /// function must be called on the main thread.
  void ProcessMainThreadTasks();

  /// Executes all tasks that that are to be run on the render thread.  This
  /// function must be called on the render thread.
  void ProcessRenderThreadTasks();

 protected:
  using Task = std::function<void()>;
  void RunOnMainThread(Task task);
  void RunOnRenderThread(Task task);

  template <typename T, typename... Args>
  std::shared_ptr<T> Create(Args&&... args) {
    auto obj = std::make_shared<T>(std::forward<Args>(args)...);
    auto main_thread_callback = [this](Task task) {
      RunOnMainThread(std::move(task));
    };
    auto render_thread_callback = [this](Task task) {
      RunOnRenderThread(std::move(task));
    };
    obj->SetCallbacks(main_thread_callback, render_thread_callback);
    return obj;
  }

 private:
  const InitParams params_;
  ThreadSafeDeque<Task> main_task_queue_;
  ThreadSafeDeque<Task> render_task_queue_;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_ASYNC_RENDER_OBJECT_FACTORY_H_
