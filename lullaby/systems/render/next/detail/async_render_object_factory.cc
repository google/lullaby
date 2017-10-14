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

#include "lullaby/systems/render/next/detail/async_render_object_factory.h"

namespace lull {
namespace detail {

AsyncRenderObjectFactory::AsyncRenderObjectFactory(const InitParams& params)
    : params_(params) {}

void AsyncRenderObjectFactory::RunOnMainThread(Task task) {
  if (!task) {
    return;
  } else if (params_.async_render) {
    main_task_queue_.PushBack(std::move(task));
  } else {
    task();
  }
}

void AsyncRenderObjectFactory::RunOnRenderThread(Task task) {
  if (!task) {
    return;
  } else if (params_.async_render) {
    render_task_queue_.PushBack(std::move(task));
  } else {
    task();
  }
}

void AsyncRenderObjectFactory::ProcessMainThreadTasks() {
  Task task;
  while (main_task_queue_.PopFront(&task)) {
    task();
  }
}

void AsyncRenderObjectFactory::ProcessRenderThreadTasks() {
  Task task;
  while (render_task_queue_.PopFront(&task)) {
    task();
  }
}

}  // namespace detail
}  // namespace lull
