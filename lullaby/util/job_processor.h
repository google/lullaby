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

#ifndef LULLABY_UTIL_JOB_PROCESSOR_H_
#define LULLABY_UTIL_JOB_PROCESSOR_H_

#include <future>
#include <utility>

#include "lullaby/util/async_processor.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/typeid.h"

namespace lull {

// AsyncProcessor that can be used to execute functions asynchronously. This
// type of processor has an associated lullaby typeid, which allows it to be
// used in the lullaby Registry.
using JobProcessor = AsyncProcessor<std::packaged_task<void()>>;

// Queues the specified function for execution and returns a future which can
// be used to query the status. Execution will begin as soon as worker thread
// is available.
template <typename Func>
std::future<void> RunJob(JobProcessor* processor, Func&& func) {
  CHECK(processor != nullptr);

  std::packaged_task<void()> task(std::forward<Func>(func));
  auto job = task.get_future();
  processor->Execute(std::move(task),
                     [](std::packaged_task<void()>* task) { (*task)(); });
  return job;
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::JobProcessor);

#endif  // LULLABY_UTIL_JOB_PROCESSOR_H_
