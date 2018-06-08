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

// Tests for the TypedScheduleProcessor.
#include "lullaby/util/typed_scheduled_processor.h"

#include "gtest/gtest.h"

namespace lull {
namespace {

TEST(TypedScheduledProcessorTest, All) {
  TypedScheduledProcessor typed_scheduled_processor;

  int value = 0;

  const TypeId one = Hash("one");
  const TypeId two = Hash("two");
  const TypeId three = Hash("three");
  const TypeId four = Hash("four");

  // Check intial state for each type.
  EXPECT_TRUE(typed_scheduled_processor.Empty(one));
  EXPECT_EQ(0ul, typed_scheduled_processor.Size(one));
  EXPECT_TRUE(typed_scheduled_processor.Empty(two));
  typed_scheduled_processor.Add(one, [&]() { value = 1; },
                                std::chrono::milliseconds(100));
  // Verify adding to tasks for one type doesn't effect the count for other
  // types.
  EXPECT_FALSE(typed_scheduled_processor.Empty(one));
  EXPECT_EQ(1ul, typed_scheduled_processor.Size(one));
  EXPECT_TRUE(typed_scheduled_processor.Empty(two));
  typed_scheduled_processor.ClearTasksOfType(one);

  // Verify the task removed correctly and isn't run.
  EXPECT_TRUE(typed_scheduled_processor.Empty(one));
  typed_scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(0, value);

  // Ensure tasks added to multiple ScheduledProcessors happen in the correct
  // order.
  typed_scheduled_processor.Add(one, [&]() { value = 1; },
                                std::chrono::milliseconds(100));
  typed_scheduled_processor.Add(two, [&]() { value = 2; },
                                std::chrono::milliseconds(200));
  typed_scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(1, value);
  typed_scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(2, value);
  EXPECT_TRUE(typed_scheduled_processor.Empty(one));
  EXPECT_TRUE(typed_scheduled_processor.Empty(two));

  // Ensure tasks added with no time happen "immediately".
  typed_scheduled_processor.Add(three, [&]() { value = 3; },
                                std::chrono::milliseconds(150));
  typed_scheduled_processor.Add(four, [&]() { value = 4; });
  typed_scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(4, value);
  EXPECT_TRUE(typed_scheduled_processor.Empty(four));
  typed_scheduled_processor.Tick(std::chrono::milliseconds(100));
  EXPECT_EQ(3, value);
  EXPECT_TRUE(typed_scheduled_processor.Empty(three));
}

}  // namespace
}  // namespace lull
