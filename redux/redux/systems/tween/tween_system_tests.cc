/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/systems/tween/tween_system.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::Gt;
using ::testing::Lt;

class TweenSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    ScriptEngine::Create(&registry_);
    AnimationEngine::Create(&registry_);
    tween_system_ = registry_.Create<TweenSystem>(&registry_);
    registry_.Initialize();
    script_engine_ = registry_.Get<ScriptEngine>();
    anim_engine_ = registry_.Get<AnimationEngine>();
  }

  void AdvanceFrame(absl::Duration delta_time) {
    anim_engine_->AdvanceFrame(delta_time);
    tween_system_->PostAnimation(delta_time);
  }

  Registry registry_;
  TweenSystem* tween_system_;
  ScriptEngine* script_engine_;
  AnimationEngine* anim_engine_;
};

TEST_F(TweenSystemTest, Tween1f) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);
  float value = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value, Eq(0.0f));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Gt(value));
    value = curr_value;
  }
  EXPECT_THAT(value, Eq(1.0f));
}

TEST_F(TweenSystemTest, Tween4f) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams4f params;
  params.init_value = vec4(1, 2, 3, 4);
  params.target_value = vec4(2, 4, 6, 8);
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);
  vec4 value(tween_system_->GetCurrentValue(tween_id).data());
  EXPECT_THAT(value[0], Eq(1.0f));
  EXPECT_THAT(value[1], Eq(2.0f));
  EXPECT_THAT(value[2], Eq(3.0f));
  EXPECT_THAT(value[3], Eq(4.0f));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const vec4 curr_value(result.data());
    EXPECT_THAT(curr_value[0], Gt(value[0]));
    EXPECT_THAT(curr_value[1], Gt(value[1]));
    EXPECT_THAT(curr_value[2], Gt(value[2]));
    EXPECT_THAT(curr_value[3], Gt(value[3]));
    value = curr_value;
  }
  EXPECT_THAT(value[0], FloatEq(2.0f));
  EXPECT_THAT(value[1], FloatEq(4.0f));
  EXPECT_THAT(value[2], FloatEq(6.0f));
  EXPECT_THAT(value[3], FloatEq(8.0f));
}

TEST_F(TweenSystemTest, TweenLongTime) {
  static constexpr int kNumSteps = 100;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(25);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);
  float value = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value, Eq(0.0f));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Gt(value));
    value = curr_value;
  }
  EXPECT_THAT(value, Eq(1.0f));
}

TEST_F(TweenSystemTest, TweenLargeToSmall) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams1f params;
  params.init_value = 1.0f;
  params.target_value = -1.0f;
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);
  float value = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value, Eq(1.0f));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Lt(value));
    value = curr_value;
  }
  EXPECT_THAT(value, Eq(-1.0f));
}

TEST_F(TweenSystemTest, Start) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);
  float value = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value, Eq(0.0f));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Gt(value));
    value = curr_value;
  }
  EXPECT_THAT(value, Eq(1.0f));
}

TEST_F(TweenSystemTest, StartEntity) {
  static constexpr int kNumSteps = 10;
  const Entity entity(123);
  const HashValue channel = ConstHash("test");

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id =
      tween_system_->Start(entity, channel, params);
  float value = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value, Eq(0.0f));

  EXPECT_THAT(tween_system_->GetTweenId(entity, channel), Eq(tween_id));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Gt(value));
    value = curr_value;
  }
  EXPECT_THAT(value, Eq(1.0f));
}

TEST_F(TweenSystemTest, StartEntityResumesFromPreviousValue) {
  static constexpr int kNumSteps = 10;
  const Entity entity(123);
  const HashValue channel = ConstHash("test");

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  auto tween_id = tween_system_->Start(entity, channel, params);

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    AdvanceFrame(delta_time);
  }

  auto value1 = tween_system_->GetCurrentValue(tween_id)[0];

  params.init_value.reset();
  params.target_value = 0.0f;
  tween_id = tween_system_->Start(entity, channel, params);

  auto value2 = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value1, Eq(value2));

  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
  }
  auto value3 = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value3, FloatNear(0.0f, 1e-07));
}

TEST_F(TweenSystemTest, Pause) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);
  float value = tween_system_->GetCurrentValue(tween_id)[0];
  EXPECT_THAT(value, Eq(0.0f));

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Gt(value));
    value = curr_value;
  }

  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id));
  tween_system_->Pause(tween_id);
  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id));

  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Eq(value));
    value = curr_value;
  }

  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id));
  tween_system_->Unpause(tween_id);
  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id));

  for (int i = kNumSteps / 2; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    const float curr_value = result[0];
    EXPECT_THAT(curr_value, Gt(value));
    value = curr_value;
  }

  EXPECT_THAT(value, Eq(1.0f));
}

TEST_F(TweenSystemTest, PauseEntity) {
  static constexpr int kNumSteps = 10;
  const Entity entity(123);
  const HashValue channel1 = ConstHash("test1");
  const HashValue channel2 = ConstHash("test2");

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const auto tween_id1 = tween_system_->Start(entity, channel1, params);
  const auto tween_id2 = tween_system_->Start(entity, channel2, params);

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    AdvanceFrame(delta_time);
  }

  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id1));
  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id2));
  tween_system_->Pause(entity);
  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id1));
  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id2));
  tween_system_->Unpause(entity);
  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id1));
  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id2));
}

TEST_F(TweenSystemTest, Stop) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    AdvanceFrame(delta_time);
  }

  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id));

  auto res1 = tween_system_->GetCurrentValue(tween_id);
  EXPECT_FALSE(res1.empty());
  EXPECT_THAT(res1[0], Gt(0.0f));
  EXPECT_THAT(res1[0], Lt(1.0f));

  tween_system_->Stop(tween_id);
  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id));

  auto res2 = tween_system_->GetCurrentValue(tween_id);
  EXPECT_FALSE(res2.empty());
  EXPECT_THAT(res1[0], Gt(0.0f));
  EXPECT_THAT(res1[0], Lt(1.0f));

  AdvanceFrame(delta_time);
  auto res3 = tween_system_->GetCurrentValue(tween_id);
  EXPECT_TRUE(res3.empty());
}

TEST_F(TweenSystemTest, StopEntity) {
  static constexpr int kNumSteps = 10;
  const Entity entity(123);
  const HashValue channel1 = ConstHash("test1");
  const HashValue channel2 = ConstHash("test2");

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  const auto tween_id1 = tween_system_->Start(entity, channel1, params);
  const auto tween_id2 = tween_system_->Start(entity, channel2, params);

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    AdvanceFrame(delta_time);
  }

  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id1));
  EXPECT_TRUE(tween_system_->IsTweenPlaying(tween_id2));
  tween_system_->Stop(entity);
  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id1));
  EXPECT_FALSE(tween_system_->IsTweenPlaying(tween_id2));
}

TEST_F(TweenSystemTest, OnUpdate) {
  static constexpr int kNumSteps = 10;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);

  float cb_value = 0.f;
  params.on_update_callback = [&](absl::Span<const float> data) {
    const float value = data[0];
    EXPECT_THAT(value, Gt(cb_value));
    cb_value = value;
  };

  const TweenSystem::TweenId tween_id = tween_system_->Start(params);

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    AdvanceFrame(delta_time);
    auto result = tween_system_->GetCurrentValue(tween_id);
    EXPECT_THAT(result[0], Eq(cb_value));
  }
}

TEST_F(TweenSystemTest, OnCompleted) {
  static constexpr int kNumSteps = 10;

  std::optional<TweenSystem::CompletionReason> reason;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);
  params.on_completed_callback = [&](TweenSystem::CompletionReason r) {
    reason = r;
  };

  const auto tween_id = tween_system_->Start(params);
  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps; ++i) {
    EXPECT_FALSE(reason.has_value());
    AdvanceFrame(delta_time);
  }
  EXPECT_THAT(reason.value(), Eq(TweenSystem::CompletionReason::kCompleted));

  auto result = tween_system_->GetCurrentValue(tween_id);
  EXPECT_THAT(result[0], Eq(1.0f));
}

TEST_F(TweenSystemTest, OnCancelled) {
  static constexpr int kNumSteps = 10;

  std::optional<TweenSystem::CompletionReason> reason;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);
  params.on_completed_callback = [&](TweenSystem::CompletionReason r) {
    reason = r;
  };

  const auto tween_id = tween_system_->Start(params);
  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    EXPECT_FALSE(reason.has_value());
    AdvanceFrame(delta_time);
  }

  EXPECT_FALSE(reason.has_value());
  tween_system_->Stop(tween_id);
  EXPECT_THAT(reason.value(), Eq(TweenSystem::CompletionReason::kCancelled));

  auto result = tween_system_->GetCurrentValue(tween_id);
  EXPECT_THAT(result[0], Gt(0.0f));
  EXPECT_THAT(result[0], Lt(1.0f));
}

TEST_F(TweenSystemTest, OnInterrupted) {
  static constexpr int kNumSteps = 10;
  const Entity entity(123);
  const HashValue channel = ConstHash("test");

  std::optional<TweenSystem::CompletionReason> reason;

  TweenSystem::TweenParams1f params;
  params.init_value = 0.0f;
  params.target_value = 1.0f;
  params.duration = absl::Seconds(1);
  params.on_completed_callback = [&](TweenSystem::CompletionReason r) {
    reason = r;
  };

  tween_system_->Start(entity, channel, params);

  const auto delta_time = params.duration / static_cast<float>(kNumSteps);
  for (int i = 0; i < kNumSteps / 2; ++i) {
    EXPECT_FALSE(reason.has_value());
    AdvanceFrame(delta_time);
  }

  EXPECT_FALSE(reason.has_value());
  auto tween_id = tween_system_->Start(entity, channel, params);
  EXPECT_THAT(reason.value(), Eq(TweenSystem::CompletionReason::kInterrupted));
  auto result = tween_system_->GetCurrentValue(tween_id);
  EXPECT_THAT(result[0], Eq(0.0f));
}

}  // namespace
}  // namespace redux
