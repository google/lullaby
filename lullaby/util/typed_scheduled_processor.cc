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

#include "lullaby/util/typed_scheduled_processor.h"

namespace lull {

void TypedScheduledProcessor::Tick(Clock::duration delta_time) {
  for (auto iter = typed_scheduled_processor_map_.begin();
       iter != typed_scheduled_processor_map_.end();) {
    iter->second.Tick(delta_time);
    // Remove any ScheduleProcessors from the map which are no longer in use.
    if (iter->second.Empty()) {
      iter = typed_scheduled_processor_map_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void TypedScheduledProcessor::Add(TypeId type, Task task,
                                  Clock::duration delay_ms) {
  // Lazily get or create a ScheduleProcessor for the type.
  auto& scheduled_processor = typed_scheduled_processor_map_[type];
  scheduled_processor.Add(task, delay_ms);
}

void TypedScheduledProcessor::Add(TypeId type, Task task) {
  Add(type, task, std::chrono::milliseconds(0));
}

void TypedScheduledProcessor::ClearTasksOfType(TypeId type) {
  typed_scheduled_processor_map_.erase(type);
}

bool TypedScheduledProcessor::Empty(TypeId type) const {
  auto iterator = typed_scheduled_processor_map_.find(type);
  return iterator == typed_scheduled_processor_map_.end() ||
         iterator->second.Empty();
}

size_t TypedScheduledProcessor::Size(TypeId type) const {
  auto iterator = typed_scheduled_processor_map_.find(type);
  return iterator == typed_scheduled_processor_map_.end()
             ? 0
             : iterator->second.Size();
}

}  // namespace lull
