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

#ifndef REDUX_SYSTEMS_ANIMATION_ANIMATION_SYSTEM_H_
#define REDUX_SYSTEMS_ANIMATION_ANIMATION_SYSTEM_H_

#include <functional>

#include "absl/container/flat_hash_map.h"
#include "redux/engines/animation/animation_engine.h"
#include "redux/engines/animation/motivator/rig_motivator.h"
#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/modules/ecs/system.h"

namespace redux {

// Plays animations on Entities used to generate poses.
class AnimationSystem : public System {
 public:
  // The various reasons an animation can finish.
  enum CompletionReason {
    kCompleted,    // Animation reached its end successfully.
    kCancelled,    // TweAnimationen was explicitly cancelled.
    kInterrupted,  // Animation was interrupted by starting another animation.
  };

  struct AnimationParams {
    // The function to call when an animation is completed.
    std::function<void(CompletionReason)> on_animation_completed;

    // The amount of time to blend between a previously running animation and
    // the requested one.
    absl::Duration blend_time = absl::ZeroDuration();

    // Offset into the requested animation at which to start playback.
    absl::Duration start_time = absl::ZeroDuration();

    // The speed at which to playback the animation.
    float speed = 1.f;

    // If true, start back at the beginning after we reach the end.
    bool repeat = false;
  };

  explicit AnimationSystem(Registry* registry);

  void OnRegistryInitialize();

  // Plays an animation on the Entity used to generate poses.
  void PlayAnimation(Entity entity, const AnimationClipPtr& animation,
                     AnimationParams params);

  // Stops the animation playing on the Entity.
  void StopAnimation(Entity entity);

  // Pauses the animation playing on the Entity.
  void PauseAnimation(Entity entity);

  // Resumes the animation playing on the Entity.
  void ResumeAnimation(Entity entity);

  // Returns the remaining time for the current animation. Returns 0 if there is
  // no animation playing or if the animation is complete. Returns infinity if
  // the animation is looping.
  absl::Duration GetTimeRemaining(Entity entity) const;

  // Updates all the animations, invoking their `on_pose_update` callbacks. This
  // function should be called after updating the AnimationEngine. Note: this
  // function is automatically bound to the Choreographer if it is available.
  void PostAnimation(absl::Duration delta_time);

 private:
  void OnEnable(Entity entity) override;
  void OnDisable(Entity entity) override;
  void OnDestroy(Entity entity) override;

  struct AnimationComponent {
    RigMotivator motivator;
    AnimationClipPtr animation;
    std::function<void(CompletionReason)> on_complete;
    float playback_speed = 0.f;
    bool paused = false;
  };

  AnimationEngine* engine_ = nullptr;
  absl::flat_hash_map<Entity, AnimationComponent> anims_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::AnimationSystem);

#endif  // REDUX_SYSTEMS_ANIMATION_ANIMATION_SYSTEM_H_
