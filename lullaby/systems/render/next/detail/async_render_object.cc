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

#include "lullaby/systems/render/next/detail/async_render_object.h"

namespace lull {
namespace detail {

AsyncRenderObject::~AsyncRenderObject() { RunOnRenderThread(deleter_); }

void AsyncRenderObject::SetCallbacks(TaskRunner main_thread,
                                     TaskRunner render_thread) {
  main_thread_runner_ = std::move(main_thread);
  render_thread_runner_ = std::move(render_thread);
}

void AsyncRenderObject::AddReadyTask(Task task) {
  ready_tasks_.Enqueue(std::move(task));
}

void AsyncRenderObject::Finish(Task deleter) {
  deleter_ = std::move(deleter);
  Task task;
  while (ready_tasks_.Dequeue(&task)) {
    RunOnMainThread(std::move(task));
  }
}

void AsyncRenderObject::RunOnMainThread(Task task) {
  DCHECK(main_thread_runner_);
  if (main_thread_runner_) {
    main_thread_runner_(std::move(task));
  }
}

void AsyncRenderObject::RunOnRenderThread(Task task) {
  DCHECK(render_thread_runner_);
  if (render_thread_runner_) {
    render_thread_runner_(std::move(task));
  }
}

}  // namespace detail
}  // namespace lull
