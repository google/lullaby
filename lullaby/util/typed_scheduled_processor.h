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

#ifndef LULLABY_UTIL_TYPED_SCHEDULED_PROCESSOR_H_
#define LULLABY_UTIL_TYPED_SCHEDULED_PROCESSOR_H_

#include <unordered_map>
#include <vector>

#include "lullaby/util/scheduled_processor.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/clock.h"

namespace lull {

// A series of schedule processors which group together tasks by a TypeId and
// allows manipulation of those tasks as a group.
//
// Tasks are defined as they are in ScheduleProcessor. Tasks are added with a
// delay and a TypeId via the Add function:
//
// typed_schedule_processor.Add(type_id, task, delay);
//
// The Tick function will tick the queue and process all tasks whose delay has
// passed and should be processed.
//
// This should typically be used instead of the ScheduledProcessor directly if
// you want to have tighter control over a group of posted Tasks including the
// ability to remove tasks.
class TypedScheduledProcessor {
 public:
  using Task = ScheduledProcessor::Task;
  TypedScheduledProcessor() {}

  // Ticks all the queues for every TypeId and and process all tasks whose delay
  // has passed.
  void Tick(Clock::duration delta_time);

  // Add a task of as specified type to the queue to be processed with a delay.
  void Add(TypeId type, Task task, Clock::duration delay_ms);

  // Add a task of as specified type to the queue to be processed on the next
  // tick.
  void Add(TypeId type, Task task);

  // Clears all tasks of a specified type.
  void ClearTasksOfType(TypeId type);

  // Returns true iff there are no tasks scheduled of the specified type. Note
  // that if you are executing the last task in the queue and call this method
  // within it, then this will return false because it will include the current
  // task.
  bool Empty(TypeId type) const;

  // Returns the number of pending tasks, including the current task if this is
  // called within a task.
  size_t Size(TypeId type) const;

 private:
  // A map of scheduled processors associated with different type IDs.
  std::unordered_map<TypeId, ScheduledProcessor> typed_scheduled_processor_map_;

  TypedScheduledProcessor(const TypedScheduledProcessor&) = delete;
  TypedScheduledProcessor& operator=(const TypedScheduledProcessor&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TypedScheduledProcessor);

#endif  // LULLABY_UTIL_TYPED_SCHEDULED_PROCESSOR_H_
