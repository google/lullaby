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

#include "lullaby/systems/animation/animation_channel.h"

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/util/logging.h"

namespace lull {

const size_t AnimationChannel::kDynamicDimensions = 0;

AnimationChannel::AnimationChannel(Registry* registry, size_t num_dimensions,
                                   size_t pool_size)
    : registry_(registry), anims_(pool_size), dimensions_(num_dimensions) {}

AnimationChannel::Animation* AnimationChannel::DoInitialize(
    Entity entity, motive::MotiveEngine* engine, size_t dimensions,
    const void* context) {
  auto anim = anims_.Get(entity);
  if (anim) {
    return anim;
  }

  anim = anims_.Emplace(entity);
  anim->scratch.resize(dimensions, 0.f);

  // Get the current values for the channel.
  if (!UsesAnimationContext()) {
    Get(entity, anim->scratch.data(), dimensions);
  } else if (context != nullptr) {
    Get(entity, anim->scratch.data(), dimensions, context);
  } else {
    LOG(DFATAL) << "Cannot initialize with a null context.";
    return anim;
  }

  // Set the motive targets to the current values.
  std::vector<motive::MotiveTarget1f> targets(dimensions);
  for (int i = 0; i < dimensions; ++i) {
    targets[i] = motive::Current1f(anim->scratch[i]);
  }

  // Initialize the motivator with the current values.
  const motive::SplineInit init;
  anim->motivator.InitializeWithTargets(
      init, engine, static_cast<int>(dimensions), targets.data());
  return anim;
}

AnimationChannel::Animation* AnimationChannel::DoInitialize(
    Entity entity, motive::MotiveEngine* engine,
    const motive::RigAnim* rig_anim) {
  auto anim = anims_.Get(entity);
  if (anim) {
    return anim;
  }

  const motive::RigInit init(*rig_anim, rig_anim->bone_parents(),
                             rig_anim->NumBones());
  anim = anims_.Emplace(entity);
  anim->rig_motivator.Initialize(init, engine);
  return anim;
}

void AnimationChannel::Update(std::vector<AnimationId>* completed) {
  // Track which animations need to be cancelled since we do not want to remove
  // them during iteration.
  std::vector<Entity> anims_to_cancel;

  anims_.ForEach([&](Animation& anim) {
    const Entity entity = anim.GetEntity();

    if (anim.rig_motivator.Valid()) {
      // Update the Component data to match the motivator's transforms.
      const int num_bones = anim.rig_motivator.DefiningAnim()->NumBones();
      if (!UsesAnimationContext()) {
        SetRig(entity, anim.rig_motivator.GlobalTransforms(), num_bones);
      } else {
        SetRig(entity, anim.rig_motivator.GlobalTransforms(), num_bones,
               anim.context);
      }
    } else if (anim.motivator.Valid()) {
      // Update the Component data to match the motivator's current values.
      const size_t dimensions = anim.motivator.Dimensions();
      const size_t num_offsets = anim.base_offset.size();
      const size_t num_multiplier = anim.multiplier.size();
      const float* anim_values = anim.motivator.Values();
      for (int i = 0; i < dimensions; ++i) {
        anim.scratch[i] =
            (i < num_offsets ? anim.base_offset[i] : 0.f) +
            (anim_values[i] * (i < num_multiplier ? anim.multiplier[i] : 1.f));
      }
      if (!UsesAnimationContext()) {
        Set(entity, anim.scratch.data(), dimensions);
      } else {
        Set(entity, anim.scratch.data(), dimensions, anim.context);
      }
    } else {
      LOG(ERROR) << "Invalid motivator detected during playback!";
      anim.total_time = 0;
    }

    if (IsComplete(anim)) {
      anims_to_cancel.push_back(anim.GetEntity());
      completed->push_back(anim.id);
    }
  });

  for (const Entity e : anims_to_cancel) {
    Cancel(e);
  }
}

AnimationId AnimationChannel::Cancel(Entity entity) {
  Animation* anim = anims_.Get(entity);
  if (anim) {
    const AnimationId id = anim->id;
    anims_.Destroy(entity);
    return id;
  }
  return kNullAnimation;
}

AnimationId AnimationChannel::Play(Entity entity, motive::MotiveEngine* engine,
                                   AnimationId id, const float* target_values,
                                   size_t length, Clock::duration time,
                                   Clock::duration delay) {
  if (!IsDimensionSupported(length)) {
    LOG(DFATAL) << "Channel does not support enough dimensions.";
    return kNullAnimation;
  }

  if (UsesAnimationContext()) {
    LOG(DFATAL) << "Cannot set targets for a context-dependent channel.";
    return kNullAnimation;
  }

  auto anim = DoInitialize(entity, engine, length, nullptr);
  if (!anim->motivator.Valid()) {
    LOG(DFATAL) << "Invalid motivator!";
    return kNullAnimation;
  }

  if (target_values == nullptr) {
    LOG(DFATAL) << "target_values must be a non-null float pointer!";
    return kNullAnimation;
  }

  const motive::MotiveTime min_time = AnimationSystem::GetMinimalStep();
  motive::MotiveTime anim_time =
      AnimationSystem::GetMotiveTimeFromDuration(time);
  motive::MotiveTime delay_time =
      AnimationSystem::GetMotiveTimeFromDuration(delay);

  anim_time = std::max(anim_time, min_time);
  if (delay_time > 0) {
    delay_time = std::max(delay_time, min_time);
  } else {
    delay_time = 0;  // Don't allow negative times.
  }
  anim->total_time = anim_time + delay_time;

  Get(entity, anim->scratch.data(), length);
  std::vector<motive::MotiveTarget1f> targets(length);
  for (int i = 0; i < length; ++i) {
    if (delay_time > 0) {
      targets[i] =
          motive::TargetToTarget1f(anim->scratch[i], 0.f, delay_time,
                                   target_values[i], 0.f, anim->total_time);
    } else {
      targets[i] = motive::Target1f(target_values[i], 0.f, anim->total_time);
    }
  }

  anim->motivator.SetTargets(targets.data());
  return UpdateId(anim, id);
}

static motive::MotiveTime MaxSplineTime(
    const motive::CompactSpline* const* splines, size_t num_splines) {
  motive::MotiveTime max_time = 0;
  for (size_t i = 0; i < num_splines; ++i) {
    if (splines[i] != nullptr) {
      max_time = std::max(max_time,
                          static_cast<motive::MotiveTime>(splines[i]->EndX()));
    }
  }
  return max_time;
}

AnimationId AnimationChannel::Play(Entity entity, motive::MotiveEngine* engine,
                                   AnimationId id,
                                   const motive::CompactSpline* const* splines,
                                   const float* constants, size_t length,
                                   const PlaybackParameters& params,
                                   const SplineModifiers& modifiers,
                                   const void* context) {
  if (!IsDimensionSupported(length)) {
    LOG(DFATAL) << "Channel does not support enough dimensions.";
    return kNullAnimation;
  }

  if (UsesAnimationContext() && context == nullptr) {
    LOG(DFATAL) << "Channel requires an animation context.";
    return kNullAnimation;
  }

  auto anim = DoInitialize(entity, engine, length, context);
  if (!anim->motivator.Valid()) {
    LOG(DFATAL) << "Invalid motivator!";
    return kNullAnimation;
  }

  const motive::MotiveTime blend_time =
      AnimationSystem::GetMotiveTimeFromSeconds(params.blend_time_s);

  // Initialize targets for any dimensions that are constant
  // (i.e. don't have splines).
  std::vector<motive::MotiveTarget1f> targets(length);
  for (size_t i = 0; i < length; ++i) {
    if (splines[i] == nullptr) {
      targets[i] = motive::Target1f(constants[i], 0.0f, blend_time);
    }
  }

  // Initialize the overall curve offsets and multipliers.
  anim->base_offset.resize(modifiers.num_offsets, 0.f);
  for (size_t i = 0; i < static_cast<size_t>(modifiers.num_offsets); ++i) {
    anim->base_offset[i] = modifiers.offsets[i];
  }
  anim->multiplier.resize(modifiers.num_multipliers, 1.f);
  for (size_t i = 0; i < static_cast<size_t>(modifiers.num_multipliers); ++i) {
    anim->multiplier[i] = modifiers.multipliers[i];
  }

  // Blend motivator to the new splines and constant values.
  motive::SplinePlayback playback;
  playback.repeat = params.looping;
  playback.playback_rate = params.speed;
  playback.blend_x = static_cast<float>(blend_time);
  playback.start_x = -static_cast<float>(
      AnimationSystem::GetMotiveTimeFromSeconds(params.start_delay_s));
  playback.y_offset = params.y_offset;
  playback.y_scale = params.y_scale;

  anim->total_time = params.looping
                         ? motive::kMotiveTimeEndless
                         : std::max(blend_time, MaxSplineTime(splines, length));
  anim->motivator.SetSplinesAndTargets(splines, playback, targets.data());
  anim->context = context;
  return UpdateId(anim, id);
}

AnimationId AnimationChannel::Play(Entity entity, motive::MotiveEngine* engine,
                                   AnimationId id,
                                   const motive::RigAnim* rig_anim,
                                   const PlaybackParameters& params,
                                   const void* context) {
  auto anim = DoInitialize(entity, engine, rig_anim);
  if (!anim->rig_motivator.Valid()) {
    LOG(DFATAL) << "Invalid motivator!";
    return kNullAnimation;
  }
  if (UsesAnimationContext() && context == nullptr) {
    LOG(DFATAL) << "Channel requires an animation context.";
    return kNullAnimation;
  }

  motive::SplinePlayback playback;
  playback.repeat = params.looping;
  playback.playback_rate = params.speed;
  playback.blend_x = static_cast<float>(
      AnimationSystem::GetMotiveTimeFromSeconds(params.blend_time_s));
  playback.start_x = -static_cast<float>(
      AnimationSystem::GetMotiveTimeFromSeconds(params.start_delay_s));
  playback.y_offset = params.y_offset;
  playback.y_scale = params.y_scale;

  anim->total_time =
      params.looping ? motive::kMotiveTimeEndless : rig_anim->end_time();

  anim->rig_motivator.BlendToAnim(*rig_anim, playback);
  anim->context = context;
  return UpdateId(anim, id);
}

void AnimationChannel::SetPlaybackRate(Entity entity, float rate) {
  Animation* anim = anims_.Get(entity);
  if (!anim) {
    return;
  }

  if (IsRigChannel()) {
    anim->rig_motivator.SetPlaybackRate(rate);
  } else {
    anim->motivator.SetSplinePlaybackRate(rate);
  }
}

void AnimationChannel::SetLooping(Entity entity, bool looping) {
  Animation* anim = anims_.Get(entity);
  if (!anim) {
    return;
  }

  if (IsRigChannel()) {
    anim->rig_motivator.SetRepeating(looping);
  } else {
    anim->motivator.SetSplineRepeating(looping);
  }
}

AnimationId AnimationChannel::UpdateId(Animation* anim, AnimationId id) {
  const AnimationId previous_id = anim->id;
  anim->id = id;
  return previous_id;
}

bool AnimationChannel::IsComplete(const Animation& anim) const {
  if (anim.total_time == motive::kMotiveTimeEndless) {
    return false;
  } else if (IsRigChannel()) {
    return anim.rig_motivator.TimeRemaining() <= 0;
  } else {
    return anim.motivator.SplineTime() > anim.total_time;
  }
}

motive::MotiveTime AnimationChannel::TimeRemaining(Entity entity) const {
  const auto* anim = anims_.Get(entity);
  if (anim == nullptr) {
    return 0;
  }
  if (anim->total_time == motive::kMotiveTimeEndless) {
    return motive::kMotiveTimeEndless;
  }
  if (IsRigChannel()) {
    return anim->rig_motivator.TimeRemaining();
  }
  return anim->total_time - anim->motivator.SplineTime();
}

const motive::RigAnim* AnimationChannel::CurrentRigAnim(Entity entity) const {
  if (!IsRigChannel()) {
    return nullptr;
  }
  const auto* anim = anims_.Get(entity);
  if (anim == nullptr) {
    return nullptr;
  }
  return anim->rig_motivator.CurrentAnim();
}

bool AnimationChannel::Get(Entity entity, float* values, size_t len) const {
  LOG(DFATAL) << "This channel does not support getting data.";
  for (size_t i = 0; i < len; ++i) {
    values[i] = 0.f;
  }
  return false;
}

bool AnimationChannel::Get(Entity entity, float* values, size_t len,
                           const void* context) const {
  LOG(DFATAL) << "This channel does not support getting data (with context).";
  for (size_t i = 0; i < len; ++i) {
    values[i] = 0.f;
  }
  return false;
}

void AnimationChannel::Set(Entity entity, const float* values, size_t len,
                           const void* context) {
  LOG(DFATAL) << "Set (with context) called on an unsupported channel.";
}

void AnimationChannel::SetRig(Entity entity,
                              const mathfu::AffineTransform* values,
                              size_t len) {
  LOG(DFATAL) << "SetRig called on unsupported channel.";
}

void AnimationChannel::SetRig(Entity entity,
                              const mathfu::AffineTransform* values, size_t len,
                              const void* context) {
  LOG(DFATAL) << "SetRig (with context) called on unsupported channel.";
}

}  // namespace lull
