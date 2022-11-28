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

#ifndef REDUX_SYSTEMS_TWEEN_TWEEN_SYSTEM_H_
#define REDUX_SYSTEMS_TWEEN_TWEEN_SYSTEM_H_

#include <functional>

#include "absl/container/flat_hash_map.h"
#include "absl/time/time.h"
#include "redux/engines/animation/animation_engine.h"
#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/engines/script/function_binder.h"
#include "redux/modules/ecs/system.h"

namespace redux {

// Interpolates values between two points over time using common algorithms.
//
// The TweenSystem using the animation engine to drive the values.
class TweenSystem : public System {
 public:
  using TweenId = uint32_t;
  static constexpr TweenId kInvalidTweenId = 0;

  // The various tweening algorithms that can be used.
  enum TweenType {
    kQuadraticEaseIn,
    kQuadraticEaseOut,
    kQuadraticEaseInOut,
    kCubicEaseIn,
    kCubicEaseOut,
    kCubicEaseInOut,
    kFastOutSlowIn,
    kMaxNumTweenTypes,
  };

  // The various reasons a tween can finish.
  enum CompletionReason {
    kCompleted,    // Tween reached its target value successfully.
    kCancelled,    // Tween was explicitly cancelled.
    kInterrupted,  // Tween was interrupted by starting another tween on the
                   // same data channel.
  };

 private:
  // The arguments for tweening. Note that this generic class is not intended
  // to be used directly; instead use one of the explicit type aliased defined
  // below (e.g. TweenParams1f).
  template <typename T>
  struct GenericTweenParams {
    // The required target value at which the tween will end.
    T target_value;

    // The length of time the tween will last.
    absl::Duration duration = absl::ZeroDuration();

    // The algorithm that will be used to drive the tween animation.
    TweenType type = kQuadraticEaseInOut;

    // Optional starting value from which to begin the tween. If no value is
    // specified, uses the value of the tween in progress or zero if no such
    // tween exists.
    std::optional<T> init_value;

    // The function to be called on every frame after the value has been
    // tweened. We use a float span here reduce one layer of indirection as a
    // minor optimization.
    std::function<void(absl::Span<const float>)> on_update_callback;

    // The function to be called when a tween finishes.
    std::function<void(CompletionReason)> on_completed_callback;
  };

 public:
  explicit TweenSystem(Registry* registry);

  void OnRegistryInitialize();

  // Parameters used for defining tweens. Tweens can be performed on upto 4
  // floating point values.
  using TweenParams1f = GenericTweenParams<float>;
  using TweenParams2f = GenericTweenParams<vec2>;
  using TweenParams3f = GenericTweenParams<vec3>;
  using TweenParams4f = GenericTweenParams<vec4>;

  // Plays a tween from the init_value to the target_value over the given
  // duration. Invokes the callback on every frame while the tween is active.
  template <typename T>
  TweenId Start(GenericTweenParams<T> params);

  // Like above, but associates the tween with a specific Entity and a channel.
  // The channel is a useful way to uniquely identify a tween on an Entity (e.g.
  // `Transform.Position`). Only one tween can be played on a given channel
  // for an Entity. Starting a new tween will interrupt the active tween.
  template <typename T>
  TweenId Start(Entity entity, HashValue channel, GenericTweenParams<T> params);

  // Returns the TweenId for the tween playing on the channel for the given
  // Entity. Returns `kInvalidTweenId` if no such tween exists.
  TweenId GetTweenId(Entity entity, HashValue channel) const;

  // Pauses the tween such that no new values will be calculated and the
  // `on_update_callback` will no longer be called.
  void Pause(TweenId tween_id);

  // Pauses all tweens on the Entity such that no new values will be calculated
  // and the `on_update_callback` will no longer be called.
  void Pause(Entity entity);

  // Unpauses the tween.
  void Unpause(TweenId tween_id);

  // Unpauses all the tweens on the Entity.
  void Unpause(Entity entity);

  // Stops the specified tween from continuing. Will invoke the tween's
  // `on_completed_callback` with a `kCancelled` reason.
  void Stop(TweenId tween_id);

  // Stops all the tweens associated with the Entity from continuing. Will
  // invoke each tween's `on_completed_callback` with a `kCancelled` reason.
  void Stop(Entity entity);

  // Returns true if the given tween is active, false otherwise.
  bool IsTweenPlaying(TweenId tween_id) const;

  // Returns the current value of the tween in progress. Returns an empty span
  // if no such tween exists.
  absl::Span<const float> GetCurrentValue(TweenId tween_id) const;

  // Updates all the tweens, storing their current value. This function should
  // be called after updating the AnimationEngine. Note: this function is
  // automatically bound to the Choreographer if it is available.
  void PostAnimation(absl::Duration delta_time);

 private:
  // Internally, we do everything with a span of floats.
  using TweenParamsSpan = GenericTweenParams<absl::Span<const float>>;

  struct Tween {
    vec4 value = vec4::Zero();
    Entity entity = kNullEntity;
    HashValue channel = HashValue(0);
    TweenType type = kQuadraticEaseInOut;
    std::size_t dimensions = 0;
    absl::Duration total_duration = absl::ZeroDuration();
    std::function<void(absl::Span<const float>)> on_update_callback;
    std::function<void(CompletionReason)> on_completed_callback;
    AnimationPlayback playback_params[4];
    SplineMotivator motivators[4];
  };

  struct TweenComponent {
    // Each Tween on an Entity is associated with a channel.
    absl::flat_hash_map<HashValue, TweenId> tweens;
  };

  template <typename T>
  static TweenParamsSpan ToTweenParamsSpan(GenericTweenParams<T> params);

  Tween* GetTween(TweenId tween_id);
  const Tween* GetTween(TweenId tween_id) const;

  void OnEnable(Entity entity) override;
  void OnDisable(Entity entity) override;
  void OnDestroy(Entity entity) override;

  TweenId Start(TweenParamsSpan params);
  TweenId Start(Entity entity, HashValue channel, TweenParamsSpan params);
  void RemoveChannel(Entity entity, HashValue channel);
  vec4 InterruptTween(TweenId tween_id);
  void EndTween(TweenId tween_id, CompletionReason reason);
  void EraseCompletedTweens();

  AnimationEngine* engine_ = nullptr;
  absl::flat_hash_map<TweenId, Tween> tweens_;
  absl::flat_hash_map<Entity, TweenComponent> tween_components_;
  CompactSplinePtr splines_[kMaxNumTweenTypes];
  std::vector<TweenId> completed_tweens_;
  TweenId next_tween_id_ = 1;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::TweenSystem);

#endif  // REDUX_SYSTEMS_TWEEN_TWEEN_SYSTEM_H_
