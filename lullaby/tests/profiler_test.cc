/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include <chrono>  // std::chrono::seconds
#include <thread>

#define PROFILE_LULLABY

#include "gtest/gtest.h"
#include "lullaby/util/profiler.h"

namespace lull {
namespace {

// Helper function to retrieve the number of seconds a sample ran.
double GetSampleDurationInSeconds(const ProfilerSampleData& sample) {
  return std::chrono::duration<double>(sample.end_time_point -
                                       sample.start_time_point)
      .count();
}

class ProfilerTest : public ::testing::Test {
 protected:
  ProfilerTest() = default;
  ~ProfilerTest() override {
    // Must be called at the end of the test or other tests are polluted.
    CleanupProfiler();
  }
};

TEST_F(ProfilerTest, Cleanup) {
  const ProfilerData* profile_data = GetProfilerData();
  EXPECT_EQ(size_t(0), profile_data->next_allocated_index);

  // Generate some data.
  for (int i = 0; i < 3; ++i) {
    LULL_PROFILE_START(test_scope_one);
    void();
    LULL_PROFILE_END(test_scope_one);
  }

  profile_data = GetProfilerData();
  EXPECT_NE(size_t(0), profile_data->next_allocated_index);

  CleanupProfiler();
  profile_data = GetProfilerData();
  EXPECT_EQ(size_t(0), profile_data->next_allocated_index);
}

TEST_F(ProfilerTest, CallTimes) {
  // Test scope 3 times.
  for (int i = 0; i < 3; ++i) {
    LULL_PROFILE_START(test_scope_one);
    void();
    LULL_PROFILE_END(test_scope_one);
  }
  // Test other scope twice.
  for (int i = 0; i < 2; ++i) {
    LULL_PROFILE(test_scope_two);
    void();
  }

  const ProfilerData* profile_data = GetProfilerData();
  EXPECT_EQ(size_t(3), profile_data->samples[0].times_called);
  EXPECT_EQ(size_t(2), profile_data->samples[1].times_called);
}

TEST_F(ProfilerTest, SamplesCalled) {
  // Sample a thing.
  for (int i = 0; i < 3; ++i) {
    LULL_PROFILE_START(test_scope_one);
    LULL_PROFILE_END(test_scope_one);
  }

  const ProfilerData* profile_data = GetProfilerData();
  EXPECT_EQ(size_t(1), profile_data->next_allocated_index);

  // Call two more things.
  {
    LULL_PROFILE(test_scope_two);
    void();
  }
  {
    LULL_PROFILE(test_scope_three);
    void();
  }

  profile_data = GetProfilerData();
  EXPECT_EQ(size_t(3), profile_data->next_allocated_index);

  // Call one more thing multiple times.
  for (int i = 0; i < 3; ++i) {
    LULL_PROFILE(test_scope_four);
  }

  profile_data = GetProfilerData();
  EXPECT_EQ(size_t(4), profile_data->next_allocated_index);
}

void RecursiveFunction(int total_num_calls) {
  LULL_PROFILE(RecursiveFunction);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  if (--total_num_calls > 0) {
    RecursiveFunction(total_num_calls);
  }
}

TEST_F(ProfilerTest, Recursion) {
  RecursiveFunction(4);
  const ProfilerData* profile_data = GetProfilerData();
  EXPECT_EQ(size_t(1), profile_data->next_allocated_index);
  EXPECT_EQ(size_t(4), profile_data->samples[0].times_called);

  // Expect the recursive function to include the time of all its calls.
  EXPECT_NEAR(GetSampleDurationInSeconds(profile_data->samples[0]), 4.0, 0.5);
}

void DoSomething(int amount) {
  std::this_thread::sleep_for(std::chrono::seconds(amount));
}

TEST_F(ProfilerTest, Times) {
  // Create some scopes within scopes to validate the outer scope is bigger than
  // the inner.
  {
    LULL_PROFILE(test_scope_one);
    DoSomething(1);
    {
      LULL_PROFILE(test_scope_two);
      DoSomething(1);
      {
        LULL_PROFILE(test_scope_three);
        DoSomething(1);
      }
    }
  }

  // Create a new scope bigger than the previous largest scope to ensure we get
  // a correct result when comparing it to the first scope.
  {
    LULL_PROFILE(test_scope_four);
    DoSomething(5);
  }

  // Expect that the initial scope would be larger than the scopes included
  // inside of it.
  const ProfilerData* profile_data = GetProfilerData();
  EXPECT_GT(GetSampleDurationInSeconds(profile_data->samples[0]),
            GetSampleDurationInSeconds(profile_data->samples[1]));
  EXPECT_GT(GetSampleDurationInSeconds(profile_data->samples[0]),
            GetSampleDurationInSeconds(profile_data->samples[2]));
  EXPECT_GT(GetSampleDurationInSeconds(profile_data->samples[1]),
            GetSampleDurationInSeconds(profile_data->samples[2]));

  // Expect the last scope to be bigger than the first.
  EXPECT_GT(GetSampleDurationInSeconds(profile_data->samples[3]),
            GetSampleDurationInSeconds(profile_data->samples[0]));
}

}  // namespace
}  // namespace lull
