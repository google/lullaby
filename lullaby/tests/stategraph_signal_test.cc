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

#include "lullaby/modules/stategraph/stategraph_signal.h"

#include "gtest/gtest.h"
#include "lullaby/util/time.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

class TestSignal : public StategraphSignal {
 public:
  TestSignal(const char* name, float start_time, float end_time)
      : StategraphSignal(Hash(name), DurationFromSeconds(start_time),
                         DurationFromSeconds(end_time)) {}
};

TEST(StategraphSignalTest, IsActive) {
  TestSignal signal("signal", 1.f, 3.f);
  EXPECT_FALSE(signal.IsActive(DurationFromSeconds(0.0f)));
  EXPECT_TRUE(signal.IsActive(DurationFromSeconds(1.0f)));
  EXPECT_TRUE(signal.IsActive(DurationFromSeconds(2.0f)));
  EXPECT_FALSE(signal.IsActive(DurationFromSeconds(3.0f)));
  EXPECT_FALSE(signal.IsActive(DurationFromSeconds(4.0f)));
}

TEST(StategraphSignalDeathTest, InvalidTime) {
  PORT_EXPECT_DEBUG_DEATH(TestSignal signal("signal", 3.f, 1.f), "");
}

}  // namespace
}  // namespace lull
