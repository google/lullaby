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

#include "gtest/gtest.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

TEST(ScheduledProcessorTest, Scheduling) {
  ScheduledProcessor scheduled_processor;

  bool array[] = {false, false, false, false};

  EXPECT_TRUE(scheduled_processor.Empty());
  EXPECT_EQ(0ul, scheduled_processor.Size());
  scheduled_processor.Add([&]() { array[1] = true; },
                          std::chrono::milliseconds(200));
  EXPECT_FALSE(scheduled_processor.Empty());
  EXPECT_EQ(1ul, scheduled_processor.Size());

  scheduled_processor.Add([&]() { array[0] = true; },
                          std::chrono::milliseconds(100));

  scheduled_processor.Add([&]() { array[2] = true; },
                          std::chrono::milliseconds(300));

  scheduled_processor.Add([&]() { array[3] = true; });

  scheduled_processor.Tick(std::chrono::milliseconds(100));

  EXPECT_EQ(array[0], true);
  EXPECT_EQ(array[1], false);
  EXPECT_EQ(array[2], false);
  EXPECT_EQ(array[3], true);

  scheduled_processor.Tick(std::chrono::milliseconds(100));

  EXPECT_EQ(array[0], true);
  EXPECT_EQ(array[1], true);
  EXPECT_EQ(array[2], false);
  EXPECT_EQ(array[3], true);

  scheduled_processor.Tick(std::chrono::milliseconds(100));

  EXPECT_EQ(array[0], true);
  EXPECT_EQ(array[1], true);
  EXPECT_EQ(array[2], true);
  EXPECT_EQ(array[3], true);

  EXPECT_TRUE(scheduled_processor.Empty());

  // Verify that the order in which things are added is respected.
  int value = 0;
  scheduled_processor.Add([&]() {
    EXPECT_EQ(value, 0);
    value = 1;
  });
  scheduled_processor.Add([&]() {
    EXPECT_EQ(value, 1);
    value = 2;
  });
  scheduled_processor.Add([&]() {
    EXPECT_EQ(value, 2);
    value = 3;
  });
  scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(value, 3);

  EXPECT_TRUE(scheduled_processor.Empty());

  // Verify that re-entrant tasks are ticked on the next frame, not the
  // current frame.
  value = 0;
  scheduled_processor.Add([&]() {
    EXPECT_EQ(value, 0);
    value = 1;
    scheduled_processor.Add([&]() {
      EXPECT_EQ(value, 1);
      value = 2;
    });
  });
  scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(value, 1);
  EXPECT_FALSE(scheduled_processor.Empty());
  scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(value, 2);
  EXPECT_TRUE(scheduled_processor.Empty());
}

TEST(ScheduledProcessorTest, Cancel) {
  using TaskId = ScheduledProcessor::TaskId;

  ScheduledProcessor scheduled_processor;

  // First test that it works in the simple case.
  EXPECT_TRUE(scheduled_processor.Empty());
  TaskId id_to_cancel = scheduled_processor.Add([]() {});
  EXPECT_EQ(scheduled_processor.Size(), 1UL);
  scheduled_processor.Cancel(id_to_cancel);
  EXPECT_TRUE(scheduled_processor.Empty());

  // Next test that it doesn't cancel or reorder other tasks.
  int value = 0;
  scheduled_processor.Add([&]() {
    EXPECT_EQ(value, 0);
    value = 1;
  });

  id_to_cancel = scheduled_processor.Add([&]() { EXPECT_TRUE(false); });

  scheduled_processor.Add([&]() {
    EXPECT_EQ(value, 1);
    value = 2;
  });

  scheduled_processor.Cancel(id_to_cancel);
  scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(value, 2);

  // Finally, test that cancelling an unknown, executed or cancelled task
  // generates a DFATAL.
  const char kErrorMessage[] = "Tried to cancel unknown task";

  EXPECT_TRUE(scheduled_processor.Empty());
  PORT_EXPECT_DEBUG_DEATH(
      scheduled_processor.Cancel(ScheduledProcessor::kInvalidTaskId),
      kErrorMessage);

  id_to_cancel = scheduled_processor.Add([]() {});
  scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_TRUE(scheduled_processor.Empty());
  PORT_EXPECT_DEBUG_DEATH(scheduled_processor.Cancel(id_to_cancel),
                          kErrorMessage);

  id_to_cancel = scheduled_processor.Add([]() {});
  scheduled_processor.Cancel(id_to_cancel);
  PORT_EXPECT_DEBUG_DEATH(scheduled_processor.Cancel(id_to_cancel),
                          kErrorMessage);
}

}  // namespace
}  // namespace lull
