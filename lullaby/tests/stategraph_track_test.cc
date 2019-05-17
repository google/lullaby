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

#include "lullaby/modules/stategraph/stategraph_track.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/time.h"

namespace lull {
namespace {

using ::testing::_;
using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;

class TestSignal : public StategraphSignal {
 public:
  TestSignal(const char* name, float start_time, float end_time)
      : StategraphSignal(Hash(name), DurationFromSeconds(start_time),
                         DurationFromSeconds(end_time)) {}

  MOCK_METHOD1(Enter, void(TypedPointer));
  MOCK_METHOD1(Exit, void(TypedPointer));
};

class TestTrack : public StategraphTrack {
 public:
  TestTrack() : StategraphTrack() {}

  void AddSignal(std::unique_ptr<TestSignal> signal) {
    signals_.emplace_back(signal.release());
  }
};

TEST(StategraphTrackTest, GetSignal) {
  TestTrack track;
  track.AddSignal(MakeUnique<TestSignal>("s1", 1.f, 3.f));

  EXPECT_THAT(track.GetSignal(Hash("s0")), IsNull());
  EXPECT_THAT(track.GetSignal(Hash("s1")), NotNull());
}

TEST(StategraphTrackTest, EnterSignals) {
  auto signal1 = MakeUnique<TestSignal>("s1", 1.f, 3.f);
  EXPECT_CALL(*signal1, Enter(_)).Times(2);
  EXPECT_CALL(*signal1, Exit(_)).Times(0);

  auto signal2 = MakeUnique<TestSignal>("s2", 2.f, 3.f);
  EXPECT_CALL(*signal2, Enter(_)).Times(1);
  EXPECT_CALL(*signal2, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal1));
  track.AddSignal(std::move(signal2));

  track.EnterActiveSignals(DurationFromSeconds(0.f));
  track.EnterActiveSignals(DurationFromSeconds(1.f));
  track.EnterActiveSignals(DurationFromSeconds(2.f));
  track.EnterActiveSignals(DurationFromSeconds(3.f));
  track.EnterActiveSignals(DurationFromSeconds(4.f));
}

TEST(StategraphTrackTest, ExitSignals) {
  auto signal1 = MakeUnique<TestSignal>("s1", 1.f, 3.f);
  EXPECT_CALL(*signal1, Enter(_)).Times(0);
  EXPECT_CALL(*signal1, Exit(_)).Times(2);

  auto signal2 = MakeUnique<TestSignal>("s2", 2.f, 3.f);
  EXPECT_CALL(*signal2, Enter(_)).Times(0);
  EXPECT_CALL(*signal2, Exit(_)).Times(1);

  TestTrack track;
  track.AddSignal(std::move(signal1));
  track.AddSignal(std::move(signal2));

  track.ExitActiveSignals(DurationFromSeconds(0.f));
  track.ExitActiveSignals(DurationFromSeconds(1.f));
  track.ExitActiveSignals(DurationFromSeconds(2.f));
  track.ExitActiveSignals(DurationFromSeconds(3.f));
  track.ExitActiveSignals(DurationFromSeconds(4.f));
}

TEST(StategraphTrackTest, ProcessSignalBeforeWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 1.f, 2.f);
  EXPECT_CALL(*signal, Enter(_)).Times(0);
  EXPECT_CALL(*signal, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalEndsDuringWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 2.f, 4.f);
  EXPECT_CALL(*signal, Enter(_)).Times(0);
  EXPECT_CALL(*signal, Exit(_)).Times(1);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalSameSizeAsWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 3.f, 6.f);
  EXPECT_CALL(*signal, Enter(_)).Times(1);
  EXPECT_CALL(*signal, Exit(_)).Times(1);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalContainedWithinWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 4.f, 5.f);
  EXPECT_CALL(*signal, Enter(_)).Times(1);
  EXPECT_CALL(*signal, Exit(_)).Times(1);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalLargerThanWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 2.f, 7.f);
  EXPECT_CALL(*signal, Enter(_)).Times(0);
  EXPECT_CALL(*signal, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalStartsDuringWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 5.f, 7.f);
  EXPECT_CALL(*signal, Enter(_)).Times(1);
  EXPECT_CALL(*signal, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalAfterWindow) {
  auto signal = MakeUnique<TestSignal>("sig", 7.f, 8.f);
  EXPECT_CALL(*signal, Enter(_)).Times(0);
  EXPECT_CALL(*signal, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalEndsWhenWindowStarts) {
  auto signal = MakeUnique<TestSignal>("sig", 2.f, 3.f);
  EXPECT_CALL(*signal, Enter(_)).Times(0);
  EXPECT_CALL(*signal, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

TEST(StategraphTrackTest, ProcessSignalStartsWhenWindowEnds) {
  auto signal = MakeUnique<TestSignal>("sig", 6.f, 7.f);
  EXPECT_CALL(*signal, Enter(_)).Times(1);
  EXPECT_CALL(*signal, Exit(_)).Times(0);

  TestTrack track;
  track.AddSignal(std::move(signal));
  track.ProcessSignals(DurationFromSeconds(3.f), DurationFromSeconds(6.f));
}

}  // namespace
}  // namespace lull
