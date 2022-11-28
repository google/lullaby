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

#include "redux/systems/animation/animation_system.h"

#include <functional>
#include <utility>

#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/math/interpolation.h"
#include "redux/systems/rig/rig_system.h"

namespace redux {

AnimationSystem::AnimationSystem(Registry* registry) : System(registry) {
  RegisterDependency<AnimationEngine>(this);
}

void AnimationSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<AnimationEngine>();
  auto choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo
        ->Add<&AnimationSystem::PostAnimation>(Choreographer::Stage::kAnimation)
        .After<&AnimationEngine::AdvanceFrame>();
  }
}

void AnimationSystem::PlayAnimation(Entity entity,
                                    const AnimationClipPtr& animation,
                                    AnimationParams params) {
  CHECK(animation != nullptr) << "Must provide an animation";
  if (entity == kNullEntity) {
    return;
  }

  AnimationPlayback playback;
  playback.playback_rate = params.speed;
  playback.start_time = params.start_time;
  playback.blend_time = params.blend_time;
  playback.repeat = params.repeat;

  animation->OnReady([=]() {
    auto iter = anims_.find(entity);
    if (iter == anims_.end()) {
      return;  // Animation cancelled before load finished.
    } else if (iter->second.animation != animation) {
      return;  // Animation cancelled before load finished.
    }

    auto& c = iter->second;
    if (!c.motivator.Valid()) {
      c.motivator = engine_->AcquireMotivator<RigMotivator>();
    }
    c.motivator.BlendToAnim(animation, playback);
  });

  auto& c = anims_[entity];
  if (c.on_complete) {
    c.on_complete(kInterrupted);
  }
  c.animation = animation;
  c.on_complete = std::move(params.on_animation_completed);
  c.playback_speed = params.speed;
}

void AnimationSystem::PauseAnimation(Entity entity) { OnDisable(entity); }

void AnimationSystem::ResumeAnimation(Entity entity) { OnEnable(entity); }

void AnimationSystem::StopAnimation(Entity entity) { OnDestroy(entity); }

void AnimationSystem::OnEnable(Entity entity) {
  if (auto it = anims_.find(entity); it != anims_.end()) {
    it->second.motivator.SetPlaybackRate(it->second.playback_speed);
    it->second.paused = false;
  }
}

void AnimationSystem::OnDisable(Entity entity) {
  if (auto it = anims_.find(entity); it != anims_.end()) {
    it->second.motivator.SetPlaybackRate(0.f);
    it->second.paused = true;
  }
}

void AnimationSystem::OnDestroy(Entity entity) {
  if (auto it = anims_.find(entity); it != anims_.end()) {
    auto completion = std::move(it->second.on_complete);
    anims_.erase(it);
    if (completion) {
      completion(kCancelled);
    }
  }
}

absl::Duration AnimationSystem::GetTimeRemaining(Entity entity) const {
  auto it = anims_.find(entity);
  return it != anims_.end() ? it->second.motivator.TimeRemaining()
                            : absl::ZeroDuration();
}

void AnimationSystem::PostAnimation(absl::Duration delta_time) {
  auto* rig_system = registry_->Get<RigSystem>();
  for (auto iter = anims_.begin(); iter != anims_.end();) {
    AnimationComponent& c = iter->second;

    bool remove = false;

    if (c.motivator.Valid()) {
      rig_system->UpdatePose(iter->first, c.motivator.GlobalTransforms());

      if (c.motivator.TimeRemaining() == absl::ZeroDuration()) {
        if (c.on_complete) {
          c.on_complete(kCompleted);
        }
        remove = true;
      }
    }

    if (remove) {
      anims_.erase(iter++);
    } else {
      ++iter;
    }
  }
}

}  // namespace redux
