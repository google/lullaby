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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_ASYNC_RENDER_OBJECT_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_ASYNC_RENDER_OBJECT_H_

#include <memory>
#include "lullaby/modules/file/asset.h"
#include "lullaby/util/thread_safe_queue.h"

namespace lull {
namespace detail {

/// Provides mechanisms for graphics objects to deal with a multithreaded
/// RenderSystem.
class AsyncRenderObject {
 public:
  using Task = std::function<void()>;
  using TaskRunner = std::function<void(Task)>;

  virtual ~AsyncRenderObject();

  /// Sets the callbacks for executing tasks on either the main thread or the
  /// render thread.
  void SetCallbacks(TaskRunner main_thread, TaskRunner render_thread);

  /// Must be called from main thread. Will execute the specified task on the
  /// main thread once the GPU resource is ready.
  void AddReadyTask(Task task);

 protected:
  /// Must be called by the derived class once the GPU resource has been setup
  /// on the render thread. The provided task will be used to free the GPU
  /// resources on the render thread. Note that the task will probably run
  /// _after_ |this| object is destroyed, so make sure that the task explicitly
  /// captures all the data it needs.
  void Finish(Task deleter);

  /// Runs the given task on the main thread.
  void RunOnMainThread(Task task);

  /// Runs the given task on the render thread.
  void RunOnRenderThread(Task task);

 private:
  Task deleter_;
  TaskRunner main_thread_runner_;
  TaskRunner render_thread_runner_;
  ThreadSafeQueue<Task> ready_tasks_;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_ASYNC_RENDER_OBJECT_H_
