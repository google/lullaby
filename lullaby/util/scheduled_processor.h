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

#ifndef LULLABY_UTIL_SCHEDULED_PROCESSOR_H_
#define LULLABY_UTIL_SCHEDULED_PROCESSOR_H_

#include <deque>
#include <functional>

#include "lullaby/util/typeid.h"
#include "lullaby/util/clock.h"

namespace lull {

// Scheduled processor handles tasks that need to be delayed before being
// processed.
//
// Tasks are defined as std::function<void()>, allowing the user to encapsulate
// functionality via a lambda. Tasks are added with a delay via the Add
// function:
// task_queue.Add(task, delay);
//
// The Tick function will tick the queue and process all tasks whose
// delay had passed and should be processed. The order in which tasks are
// processed is determined first by their delay and then by the order in which
// they were added.
class ScheduledProcessor {
 public:
  using Task = std::function<void()>;
  using TaskId = unsigned int;

  static const TaskId kInvalidTaskId = 0;

  ScheduledProcessor() {}

  // Tick the queue and process all tasks whose delay had passed.
  void Tick(Clock::duration delta_time);

  // Add a task to the queue to be processed with a delay. Returns the task ID.
  TaskId Add(Task task, Clock::duration delay_ms);

  // Add a task to be processed the next time AdvanceFrame is called. Returns
  // the task ID.
  TaskId Add(Task task);

  // Cancels a task identified by |id|. If the given task is not pending (it
  // is invalid or has already executed or been cancelled), this will result in
  // a DFATAL.
  void Cancel(TaskId id);

  // Returns true iff there are no pending tasks associated with the processor.
  // Note that if you are executing the last task in the queue and call this
  // method within it, then this will return false because it will include the
  // current task.
  bool Empty() const;

  // Returns the number of pending tasks, including the current task if this is
  // called within a task.
  size_t Size() const;

 private:
  // The TaskId to use for the next QueueItem that is created.
  TaskId next_task_id_ = 1;

  // Timer used to keep track on when tasks should be processed.
  Clock::duration timer_ = Clock::duration::zero();

  // A queue item which includes a task and a time (relative to the
  // ScheduledProcessor) to be processed at.
  struct QueueItem {
    QueueItem(Task task, TaskId task_id, Clock::duration trigger_time);

    // The time for this item to be processed at.
    Clock::duration trigger_time;

    // The task to be called when processed.
    mutable Task task;

    // A monotonically increasing ID for the task; this can be used to determine
    // the order in which tasks were added.
    TaskId task_id;

    // Compares two QueueItems by trigger time, or task ID if those are equal.
    bool operator<(const QueueItem& rhs) const {
      if (trigger_time != rhs.trigger_time) {
        return trigger_time < rhs.trigger_time;
      }
      return task_id < rhs.task_id;
    }
  };

  using TaskQueue = std::deque<QueueItem>;

  // A queue to hold all the tasks sorted manually by time.
  TaskQueue queue_;

  ScheduledProcessor(const ScheduledProcessor&) = delete;
  ScheduledProcessor& operator=(const ScheduledProcessor&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScheduledProcessor);

#endif  // LULLABY_UTIL_SCHEDULED_PROCESSOR_H_
