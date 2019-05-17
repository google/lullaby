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

#include "lullaby/util/clock.h"
#include "lullaby/util/time.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

static const Clock::rep kNanosecondsPerMicrosecond = 1000;
static const float kMicrosecondsPerSecond = 1000000;

static const Clock::rep kNanosecondsPerMillisecond = 1000000;
static const float kMillisecondsPerSecond = 1000;

static const Clock::rep kNanosecondsPerSecond = 1000000000;

TEST(TimeTest, MicrosecondsFromDuration) {
  Clock::duration duration(kNanosecondsPerMicrosecond);
  ASSERT_EQ(1, lull::MicrosecondsFromDuration(duration));
}

TEST(TimeTest, MicrosecondsFromSeconds) {
  ASSERT_EQ(kMicrosecondsPerSecond, lull::MicrosecondsFromSeconds(1));
}

TEST(TimeTest, MillisecondsFromDuration) {
  Clock::duration duration(kNanosecondsPerMillisecond);
  ASSERT_EQ(1, lull::MillisecondsFromDuration(duration));
}

TEST(TimeTest, MillisecondsFromNanoseconds) {
  ASSERT_EQ(1, lull::MillisecondsFromNanoseconds(kNanosecondsPerMillisecond));
}

TEST(TimeTest, SecondsFromDuration) {
  Clock::duration duration(kNanosecondsPerSecond);
  ASSERT_EQ(1, lull::SecondsFromDuration(duration));
}

TEST(TimeTest, SecondsFromMilliseconds) {
  ASSERT_EQ(1, lull::SecondsFromMilliseconds(kMillisecondsPerSecond));
}

}  // namespace
}  // namespace lull
