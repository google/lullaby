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

#include "lullaby/util/scheduled_processor.h"

#include <algorithm>

#include "lullaby/util/logging.h"

namespace lull {

const ScheduledProcessor::TaskId ScheduledProcessor::kInvalidTaskId;

ScheduledProcessor::QueueItem::QueueItem(Task task, TaskId task_id,
                                         Clock::duration trigger_time)
    : trigger_time(trigger_time), task(std::move(task)), task_id(task_id) {}

bool ScheduledProcessor::Empty() const { return queue_.empty(); }

size_t ScheduledProcessor::Size() const { return queue_.size(); }

void ScheduledProcessor::Tick(Clock::duration delta_time) {
  const TaskId first_invalid_task_id = next_task_id_;
  timer_ += delta_time;

  while (!queue_.empty()) {
    if (timer_ < queue_.front().trigger_time) {
      break;
    }

    // Handle the case where a task is added during Tick() with a timeout of 0.
    // Because the queue is sorted by task ID, this check will ensure that only
    // Tasks which were added prior to this Tick() are processed.
    if (first_invalid_task_id <= queue_.front().task_id) {
      break;
    }

    Task task = std::move(queue_.front().task);
    queue_.pop_front();
    task();
  }
}

ScheduledProcessor::TaskId ScheduledProcessor::Add(
    Task task, Clock::duration delay_ms) {
  const TaskId task_id = next_task_id_;
  if (++next_task_id_ == kInvalidTaskId) {
    ++next_task_id_;
  }
  QueueItem item(std::move(task), task_id, timer_ + delay_ms);
  auto pos = std::lower_bound(queue_.begin(), queue_.end(), item);
  queue_.insert(pos, std::move(item));
  return task_id;
}

ScheduledProcessor::TaskId ScheduledProcessor::Add(Task task) {
  return Add(std::move(task), std::chrono::milliseconds(0));
}

void ScheduledProcessor::Cancel(TaskId id) {
  for (auto iter = queue_.begin(); iter != queue_.end(); ++iter) {
    if (iter->task_id == id) {
      queue_.erase(iter);
      return;
    }
  }

  DCHECK(false) << "Tried to cancel unknown task " << id;
}

}  // namespace lull
