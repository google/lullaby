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

#include "lullaby/util/job_processor.h"

#include <future>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(JobProcessorTest, OneJob) {
  JobProcessor job_processor(/* num_worker_threads = */ 1);

  int value = 0;
  std::future<void> job = RunJob(&job_processor, [&value]() { value = 1; });

  job.wait();
  EXPECT_THAT(value, Eq(1));
}

TEST(JobProcessorTest, ManyJobs) {
  static const int kNumJobs = 100;

  JobProcessor job_processor(/* num_worker_threads = */ 10);

  std::future<void> jobs[kNumJobs];
  int values[kNumJobs];
  for (int i = 0; i < kNumJobs; ++i) {
    jobs[i] = RunJob(&job_processor, [&values, i]() { values[i] = i; });
  }

  for (int i = 0; i < kNumJobs; ++i) {
    jobs[i].wait();
    EXPECT_THAT(values[i], Eq(i));
  }
}

}  // namespace
}  // namespace lull
