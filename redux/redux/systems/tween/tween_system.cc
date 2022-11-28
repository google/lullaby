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

#include "redux/systems/tween/tween_system.h"

#include <utility>

#include "redux/engines/animation/animation_engine.h"
#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/math/interpolation.h"

namespace redux {

// Utility functions that will allow us to wrap up a float as a vec1f.
template <typename T>
static T AsVec(const T& t) {
  return t;
}
static Vector<float, 1, false> AsVec(float f) {
  return Vector<float, 1, false>(f);
}

// clang-format off
static constexpr vec3 kQuadraticEaseInData[] = {
    {0.000000, 0.000000, 0.009971},
    {1.000000, 1.000000, 1.999531},
};

static constexpr vec3 kQuadraticEaseOutData[] = {
    {0.000000, 0.000000, 1.989983},
    {1.000000, 1.000000, 0.000000},
};

static constexpr vec3 kQuadraticEaseInOutData[] = {
    {0.000000, 0.000000, 0.019944},
    {0.330007, 0.217792, 1.319829},
    {0.450004, 0.404990, 1.799687},
    {0.489998, 0.480034, 2.028617},
    {0.530007, 0.558190, 1.879963},
    {0.629999, 0.726192, 1.479961},
    {1.000000, 1.000000, 0.000000},
};

static constexpr vec3 kCubicEaseInData[] = {
    {0.000000, 0.000000, 0.000096},
    {0.549676, 0.182452, 0.497842},
    {1.000000, 1.000000, 2.999982},
};

static constexpr vec3 kCubicEaseOutData[] = {
    {0.000000, 0.000000, 2.969582},
    {0.330007, 0.699229, 1.346724},
    {1.000000, 1.000000, 0.000000},
};

static constexpr vec3 kCubicEaseInOutData[] = {
    {0.000000, 0.000000, 0.000383},
    {0.181537, 0.026291, 0.217016},
    {0.330007, 0.143740, 1.307027},
    {0.450004, 0.364492, 2.430013},
    {0.489998, 0.470253, 3.072471},
    {0.530007, 0.584695, 2.650512},
    {0.629999, 0.797375, 1.642933},
    {0.796628, 0.963043, 0.272510},
    {1.000000, 1.000000, 0.000000},
};

static constexpr vec3 kFastOutSlowInData[] = {
    {0.000000, 0.000000, 0.019753},
    {0.220005, 0.165698, 1.956671},
    {0.280003, 0.309728, 2.637488},
    {0.340002, 0.466667, 2.604294},
    {0.459998, 0.714031, 1.516353},
    {0.699992, 0.934417, 0.507450},
    {1.000000, 1.000000, 0.000000},
};
// clang-format on

template <size_t N>
static CompactSplinePtr BuildSpline(const vec3 (&nodes)[N]) {
  const Interval y_range(0.f, 1.f);
  const float x_granularity = CompactSpline::RecommendXGranularity(1.f);
  auto spline = CompactSpline::Create(N);
  spline->Init(y_range, x_granularity);
  for (const vec3& node : nodes) {
    spline->AddNode(node.x, node.y, node.z, kAddWithoutModification);
  }
  return spline;
}

TweenSystem::TweenSystem(Registry* registry) : System(registry) {
  RegisterDependency<AnimationEngine>(this);
}

void TweenSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<AnimationEngine>();
  splines_[kQuadraticEaseIn] = BuildSpline(kQuadraticEaseInData);
  splines_[kQuadraticEaseOut] = BuildSpline(kQuadraticEaseOutData);
  splines_[kQuadraticEaseInOut] = BuildSpline(kQuadraticEaseInOutData);
  splines_[kCubicEaseIn] = BuildSpline(kCubicEaseInData);
  splines_[kCubicEaseOut] = BuildSpline(kCubicEaseOutData);
  splines_[kCubicEaseInOut] = BuildSpline(kCubicEaseInOutData);
  splines_[kFastOutSlowIn] = BuildSpline(kFastOutSlowInData);

  auto choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo->Add<&TweenSystem::PostAnimation>(Choreographer::Stage::kAnimation)
        .After<&AnimationEngine::AdvanceFrame>();
  }
}

template <typename T>
TweenSystem::TweenId TweenSystem::Start(GenericTweenParams<T> params) {
  CHECK(params.type != TweenType::kMaxNumTweenTypes);
  const CompactSpline* spline = splines_[params.type].get();

  CHECK(params.duration > absl::ZeroDuration())
      << "Must specify a positive duration.";
  // Our predefined splines are all 1ms in duration, so we need to slow-down or
  // speed-up the playback to match the desired duration.
  const float playback_rate = 1.f / absl::ToDoubleMilliseconds(params.duration);

  auto target_value = AsVec(params.target_value);
  using VecType = decltype(target_value);

  VecType init_value(0.f);
  if (params.init_value.has_value()) {
    init_value = AsVec(params.init_value.value());
  }

  const TweenId tween_id = ++next_tween_id_;
  Tween& tween = tweens_[tween_id];
  tween.type = params.type;
  tween.total_duration = params.duration;
  tween.dimensions = VecType::kDims;
  tween.on_update_callback = std::move(params.on_update_callback);
  tween.on_completed_callback = std::move(params.on_completed_callback);

  for (std::size_t i = 0; i < tween.dimensions; ++i) {
    float init_value = 0.f;
    if (params.init_value.has_value()) {
      init_value = AsVec(params.init_value.value())[i];
    }
    float target_value = AsVec(params.target_value)[i];

    AnimationPlayback& playback = tween.playback_params[i];
    playback.playback_rate = playback_rate;
    playback.value_offset = init_value;
    playback.value_scale = target_value - init_value;

    tween.value.data[i] = init_value;
    tween.motivators[i] = engine_->AcquireMotivator<SplineMotivator>();
    tween.motivators[i].SetSpline(*spline, playback);
  }
  return tween_id;
}

template <typename T>
TweenSystem::TweenId TweenSystem::Start(Entity entity, HashValue channel,
                                        GenericTweenParams<T> params) {
  Tween* prev_tween = GetTween(GetTweenId(entity, channel));
  if (prev_tween) {
    // Interrupt the previous tween.
    if (prev_tween->on_completed_callback) {
      prev_tween->on_completed_callback(kInterrupted);
    }

    // Use the previous tweens current value as the initial vaule of the new
    // tween (if not provided by the caller).
    if (!params.init_value.has_value()) {
      if constexpr (std::is_same_v<T, float>) {
        params.init_value = *prev_tween->value.data;
      } else {
        params.init_value = T(prev_tween->value.data);
      }
    }
  }

  // Start a new tween.
  const TweenId tween_id = Start(std::move(params));

  // Associate the tween with the entity and channel.
  Tween& tween = tweens_[tween_id];
  tween.entity = entity;
  tween.channel = channel;
  tween_components_[entity].tweens[channel] = tween_id;
  return tween_id;
}

void TweenSystem::Pause(TweenId tween_id) {
  Tween* tween = GetTween(tween_id);
  if (tween == nullptr) {
    return;  // Invalid tween.
  } else if (!tween->motivators[0].Valid()) {
    return;  // Already paused.
  }

  // Our predefined splines all have a length of 1ms. We use that length in
  // order to determine how far along we are in the current spline playback and
  // use that value for when we want to resume the spline.
  const absl::Duration spline_elapsed_time =
      absl::Milliseconds(1) - tween->motivators[0].TimeRemaining();
  for (std::size_t i = 0; i < tween->dimensions; ++i) {
    tween->playback_params[i].start_time = spline_elapsed_time;
    tween->motivators[i].Invalidate();
  }
}

void TweenSystem::Pause(Entity entity) {
  if (auto it = tween_components_.find(entity); it != tween_components_.end()) {
    for (const auto& [_, tween_id] : it->second.tweens) {
      Pause(tween_id);
    }
  }
}

void TweenSystem::Unpause(TweenId tween_id) {
  Tween* tween = GetTween(tween_id);
  if (tween == nullptr) {
    return;  // Invalild tween.
  } else if (tween->type == kMaxNumTweenTypes) {
    return;  // Already ended.
  } else if (tween->motivators[0].Valid()) {
    return;  // Already paused.
  }

  const CompactSpline* spline = splines_[tween->type].get();
  for (std::size_t i = 0; i < tween->dimensions; ++i) {
    tween->motivators[i] = engine_->AcquireMotivator<SplineMotivator>();
    tween->motivators[i].SetSpline(*spline, tween->playback_params[i]);
  }
}

void TweenSystem::Unpause(Entity entity) {
  if (auto it = tween_components_.find(entity); it != tween_components_.end()) {
    for (const auto& [_, tween_id] : it->second.tweens) {
      Unpause(tween_id);
    }
  }
}

void TweenSystem::Stop(TweenId tween_id) { EndTween(tween_id, kCancelled); }

void TweenSystem::Stop(Entity entity) {
  if (auto it = tween_components_.find(entity); it != tween_components_.end()) {
    for (const auto& [_, tween_id] : it->second.tweens) {
      Stop(tween_id);
    }
  }
}

void TweenSystem::EndTween(TweenId tween_id, CompletionReason reason) {
  Tween* tween = GetTween(tween_id);
  if (tween == nullptr) {
    return;  // Invalid tween.
  }

  if (tween->on_completed_callback) {
    tween->on_completed_callback(reason);
  }
  for (std::size_t i = 0; i < tween->dimensions; ++i) {
    tween->motivators[i].Invalidate();
  }

  // Just invalidating the motivator is not enough (as that is also what
  // happens when a tween is paused), so also invalidate the tween type to
  // indicate that the tween has ended.
  tween->type = kMaxNumTweenTypes;

  // We delay the actual destruction of the tween by 1 frame so that users
  // can still call GetCurrentValue one last time. (This also helps prevent
  // issues with deleting a tween inside a for-loop).
  completed_tweens_.push_back(tween_id);
}

void TweenSystem::OnEnable(Entity entity) { Pause(entity); }

void TweenSystem::OnDisable(Entity entity) { Unpause(entity); }

void TweenSystem::OnDestroy(Entity entity) { Stop(entity); }

TweenSystem::TweenId TweenSystem::GetTweenId(Entity entity,
                                             HashValue channel) const {
  auto it1 = tween_components_.find(entity);
  if (it1 == tween_components_.end()) {
    return kInvalidTweenId;
  }
  auto it2 = it1->second.tweens.find(channel);
  if (it2 == it1->second.tweens.end()) {
    return kInvalidTweenId;
  }
  return it2->second;
}

TweenSystem::Tween* TweenSystem::GetTween(TweenId tween_id) {
  auto it = tweens_.find(tween_id);
  return it != tweens_.end() ? &it->second : nullptr;
}

const TweenSystem::Tween* TweenSystem::GetTween(TweenId tween_id) const {
  auto it = tweens_.find(tween_id);
  return it != tweens_.end() ? &it->second : nullptr;
}

bool TweenSystem::IsTweenPlaying(TweenId tween_id) const {
  const Tween* tween = GetTween(tween_id);
  return tween ? tween->motivators[0].Valid() : false;
}

absl::Span<const float> TweenSystem::GetCurrentValue(TweenId tween_id) const {
  const Tween* tween = GetTween(tween_id);
  return tween ? tween->value.data : absl::Span<const float>();
}

void TweenSystem::PostAnimation(absl::Duration delta_time) {
  EraseCompletedTweens();

  for (auto& [tween_id, tween] : tweens_) {
    if (!tween.motivators[0].Valid()) {
      continue;  // Tween paused.
    }

    for (std::size_t i = 0; i < tween.dimensions; ++i) {
      tween.value.data[i] = tween.motivators[i].Value();
    }

    if (tween.on_update_callback) {
      tween.on_update_callback(tween.value.data);
    }

    if (tween.motivators[0].TimeRemaining() == absl::ZeroDuration()) {
      EndTween(tween_id, kCompleted);
    }
  }
}

void TweenSystem::EraseCompletedTweens() {
  for (const TweenId tween_id : completed_tweens_) {
    auto it = tweens_.find(tween_id);
    CHECK(it != tweens_.end()) << "Tween already destroyed?";

    Tween& tween = it->second;
    if (tween.entity != kNullEntity && tween.channel != HashValue(0)) {
      RemoveChannel(tween.entity, tween.channel);
    }
    tweens_.erase(it);
  }
  completed_tweens_.clear();
}

void TweenSystem::RemoveChannel(Entity entity, HashValue channel) {
  if (auto it = tween_components_.find(entity); it != tween_components_.end()) {
    it->second.tweens.erase(channel);
    if (it->second.tweens.empty()) {
      tween_components_.erase(it);
    }
  }
}

// Explicitly create the template member functions for only the specializations
// we want to support.
template TweenSystem::TweenId TweenSystem::Start(TweenParams1f);
template TweenSystem::TweenId TweenSystem::Start(TweenParams2f);
template TweenSystem::TweenId TweenSystem::Start(TweenParams3f);
template TweenSystem::TweenId TweenSystem::Start(TweenParams4f);
template TweenSystem::TweenId TweenSystem::Start(Entity, HashValue,
                                                 TweenParams1f);
template TweenSystem::TweenId TweenSystem::Start(Entity, HashValue,
                                                 TweenParams2f);
template TweenSystem::TweenId TweenSystem::Start(Entity, HashValue,
                                                 TweenParams3f);
template TweenSystem::TweenId TweenSystem::Start(Entity, HashValue,
                                                 TweenParams4f);

}  // namespace redux
