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

#include "lullaby/systems/animation/animation_system.h"

#include "lullaby/events/animation_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"
#include "motive/matrix_init.h"
#include "motive/rig_init.h"
#include "motive/spline_init.h"
#include "motive/sqt_init.h"
#include "motive/spline_anim_generated.h"

namespace lull {
namespace {

const HashValue kAnimationDef = ConstHash("AnimationDef");
const HashValue kAnimationResponseDef = ConstHash("AnimationResponseDef");
constexpr const char* kMotiveListExtension = "motivelist";

inline bool IsMotiveListFile(const std::string& filename, size_t start_pos,
                             size_t end_pos) {
  return (filename.compare(start_pos, end_pos - start_pos,
                           kMotiveListExtension) == 0);
}

}  // namespace

using MotiveTimeUnit = std::chrono::duration<motive::MotiveTime, std::milli>;

AnimationSystem::AnimationSystem(Registry* registry)
    : System(registry), current_id_(kNullAnimation) {
  RegisterDef<AnimationDefT>(this);
  RegisterDef<AnimationResponseDefT>(this);

  motive::RigInit::Register();
  motive::SqtInit::Register();
  motive::MatrixInit::Register();
  motive::SplineInit::Register();

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    auto set_target_fn = [this](Entity e, HashValue channel,
                                const std::vector<float>& data, int time_ms) {
      const auto duration_ms = std::chrono::milliseconds(time_ms);
      return SetTarget(e, channel, data.data(), data.size(), duration_ms);
    };
    auto play_anim_fn = [this](Entity e, HashValue channel,
                               const std::string& filename,
                               const PlaybackParameters& params) {
      auto asset = LoadAnimation(filename);
      return PlayAnimation(e, channel, asset, params);
    };

    binder->RegisterFunction("lull.Animation.SetTarget",
                             std::move(set_target_fn));
    binder->RegisterFunction("lull.Animation.Play", std::move(play_anim_fn));
    binder->RegisterMethod("lull.Animation.AdvanceFrame",
                           &lull::AnimationSystem::AdvanceFrame);
  }

  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this, [this](const CancelAllAnimationsEvent& event) {
      CancelAllAnimations(event.entity);
    });
  }
}

AnimationSystem::~AnimationSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Animation.SetTarget");
    binder->UnregisterFunction("lull.Animation.Play");
    binder->UnregisterFunction("lull.Animation.AdvanceFrame");
  }
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
}

void AnimationSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kAnimationResponseDef) {
    const AnimationResponseDef* data = ConvertDef<AnimationResponseDef>(def);
    if (!data->animation() || !data->inputs()) {
      LOG(DFATAL) << "Cannot create response with no animations or inputs.";
      return;
    }

    auto response = [this, entity, data](const EventWrapper&) {
      PlayAnimation(entity, data->animation());
    };

    ConnectEventDefs(registry_, entity, data->inputs(), response);
  } else if (type == kAnimationDef) {
    const AnimationDef* data = ConvertDef<AnimationDef>(def);
    if (data->defining_animation()) {
      LOG(INFO) << "Defining animations are no longer supported or necessary.";
    }
  }
}

void AnimationSystem::PostCreateInit(Entity e, HashValue type, const Def* def) {
  if (type == kAnimationDef) {
    const AnimationDef* data = ConvertDef<AnimationDef>(def);
    PlayAnimation(e, data);
  }
}

void AnimationSystem::Destroy(Entity entity) {
  CancelAllAnimations(entity);
}

void AnimationSystem::CancelAllAnimations(Entity entity) {
  for (auto& channel : channels_) {
    const AnimationId id = channel.second->Cancel(entity);
    UntrackAnimation(id, AnimationCompletionReason::kCancelled);
  }
}

void AnimationSystem::AddChannel(HashValue id, AnimationChannelPtr channel) {
  channels_[id] = std::move(channel);
}

AnimationAssetPtr AnimationSystem::LoadAnimation(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  return assets_.Create(key, [&]() {
    AssetLoader* asset_loader = registry_->Get<AssetLoader>();
    return asset_loader->LoadNow<AnimationAsset>(filename);
  });
}

AnimationAssetPtr AnimationSystem::LoadAnimationAsync(
    const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  return assets_.Create(key, [&]() {
    AssetLoader* asset_loader = registry_->Get<AssetLoader>();
    return asset_loader->LoadAsync<AnimationAsset>(filename);
  });
}

AnimationAssetPtr AnimationSystem::CreateAnimation(
    HashValue key, DataContainer splines, size_t num_splines,
    std::shared_ptr<void> context) {
  return assets_.Create(key, [&]() {
    return std::make_shared<AnimationAsset>(std::move(splines), num_splines,
                                            std::move(context));
  });
}

AnimationAssetPtr AnimationSystem::CreateAnimation(
    HashValue key, std::unique_ptr<motive::RigAnim> anim,
    std::shared_ptr<void> context) {
  return assets_.Create(key, [&]() {
    return std::make_shared<AnimationAsset>(std::move(anim),
                                            std::move(context));
  });
}

AnimationAssetPtr AnimationSystem::GetAnimation(HashValue key) {
  return assets_.Find(key);
}

void AnimationSystem::UnloadAnimation(const std::string& filename) {
  assets_.Release(Hash(filename));
}

void AnimationSystem::UnloadAllAnimations() { assets_.Reset(); }

void AnimationSystem::AdvanceFrame(Clock::duration delta_time) {
  LULLABY_CPU_TRACE("AnimAdvance");
  const motive::MotiveTime timestep =
      GetMotiveTimeFromDuration(delta_time + accumulated_time_error_);
  accumulated_time_error_ += (delta_time - GetDurationFromMotiveTime(timestep));
  engine_.AdvanceFrame(timestep);

  std::vector<AnimationId> completed;
  for (auto& channel : channels_) {
    channel.second->Update(&completed);
  }

  for (const AnimationId id : completed) {
    UntrackAnimation(id, AnimationCompletionReason::kCompleted);
  }
}

void AnimationSystem::CancelAnimation(Entity e, HashValue channel) {
  auto iter = channels_.find(channel);
  if (iter != channels_.end()) {
    const AnimationId id = iter->second->Cancel(e);
    UntrackAnimation(id, AnimationCompletionReason::kCancelled);
  }
}

bool AnimationSystem::IsAnimationPlaying(AnimationId id) const {
  return external_id_to_entry_.count(id) != 0;
}

motive::MotiveTime AnimationSystem::TimeRemaining(Entity entity,
                                                  HashValue channel) {
  auto iter = channels_.find(channel);
  if (iter == channels_.end()) {
    return 0;
  }
  return iter->second->TimeRemaining(entity);
}

const motive::RigAnim* AnimationSystem::CurrentRigAnim(Entity entity,
                                                       HashValue channel) {
  auto iter = channels_.find(channel);
  if (iter == channels_.end()) {
    return nullptr;
  }
  return iter->second->CurrentRigAnim(entity);
}

AnimationId AnimationSystem::SetTarget(Entity e, HashValue channel,
                                       const float* data, size_t len,
                                       Clock::duration time,
                                       Clock::duration delay) {
  AnimationChannel* channel_ptr = FindChannel(channel);
  if (channel_ptr == nullptr) {
    LOG(DFATAL) << "Could not find channel: " << channel;
    return kNullAnimation;
  }

  AnimationId id = SetTargetInternal(e, channel_ptr, data, len, time, delay);
  if (id == kNullAnimation) {
    return kNullAnimation;
  }

  AnimationSet anims;
  anims.emplace(id);
  return TrackAnimations(e, std::move(anims), nullptr);
}

AnimationId AnimationSystem::PlayAnimation(Entity e, const AnimationDef* data) {
  AnimationSet anims;
  if (data->animations()) {
    for (unsigned int i = 0; i < data->animations()->size(); ++i) {
      const AnimationId id = PlayAnimation(e, data->animations()->Get(i));
      anims.emplace(id);
    }
  }
  if (data->targets()) {
    for (unsigned int i = 0; i < data->targets()->size(); ++i) {
      const AnimationId id = PlayAnimation(e, data->targets()->Get(i));
      anims.emplace(id);
    }
  }
  return TrackAnimations(e, std::move(anims), data);
}

AnimationId AnimationSystem::PlayAnimation(Entity e,
                                           const AnimTargetDef* target) {
  if (target->values() == nullptr || target->values()->size() == 0) {
    LOG(DFATAL) << "No actual data in AnimTargetDef.";
    return kNullAnimation;
  }

  AnimationChannel* channel_ptr = FindChannel(target->channel()->c_str());
  if (channel_ptr == nullptr) {
    LOG(DFATAL) << "Could not find channel: " << target->channel()->c_str();
    return kNullAnimation;
  }

  const float* values = target->values()->data();
  const size_t len = target->values()->size();
  const std::chrono::milliseconds time(target->time_ms());
  const std::chrono::milliseconds delay(target->start_delay_ms());
  return SetTargetInternal(e, channel_ptr, values, len, time, delay);
}

AnimationId AnimationSystem::PlayAnimation(Entity e,
                                           const AnimInstanceDef* anim) {
  AnimationChannel* channel_ptr = FindChannel(anim->channel()->c_str());
  if (channel_ptr == nullptr) {
    LOG(DFATAL) << "Could not find channel: " << anim->channel()->c_str();
    return kNullAnimation;
  }

  if (channel_ptr->IsRigChannel()) {
    return PlayRigAnimation(e, channel_ptr, anim);
  } else {
    return PlaySplineAnimation(e, channel_ptr, anim);
  }
}

AnimationId AnimationSystem::PlayAnimation(Entity e, HashValue channel,
                                           const AnimationAssetPtr& anim,
                                           const PlaybackParameters& params) {
  AnimationChannel* channel_ptr = FindChannel(channel);
  if (channel_ptr == nullptr) {
    LOG(DFATAL) << "Could not find channel: " << channel;
    return kNullAnimation;
  }

  AnimationId id = kNullAnimation;
  if (channel_ptr->IsRigChannel()) {
    id = PlayRigAnimationInternal(e, channel_ptr, anim, params);
  } else {
    id = PlaySplineAnimationInternal(e, channel_ptr, anim, params);
  }

  if (id == kNullAnimation) {
    return kNullAnimation;
  } else {
    AnimationSet anims;
    anims.emplace(id);
    return TrackAnimations(e, std::move(anims), nullptr);
  }
}

AnimationId AnimationSystem::PlayRigAnimation(Entity e,
                                              AnimationChannel* channel,
                                              const AnimInstanceDef* anim) {
  const int num_names = anim->filenames()->size();
  if (num_names != 1) {
    LOG(DFATAL) << "Expecting exactly 1 animation in def.";
    return kNullAnimation;
  }

  std::string filename = anim->filenames()->Get(0)->str();
  int list_index = 0;
  SplitListFilenameAndIndex(&filename, &list_index);
  if (filename.empty()) {
    LOG(DFATAL) << "No filename specified.";
    return kNullAnimation;
  }

  AnimationAssetPtr asset = LoadAnimation(filename);
  if (!asset) {
    LOG(DFATAL) << "Could not load animation: " << filename;
    return kNullAnimation;
  }

  const PlaybackParameters params = GetPlaybackParameters(anim);
  return PlayRigAnimationInternal(e, channel, asset, params, list_index);
}

AnimationId AnimationSystem::PlaySplineAnimation(Entity e,
                                                 AnimationChannel* channel,
                                                 const AnimInstanceDef* anim) {
  const size_t num_filenames = anim->filenames()->size();
  if (!channel->IsDimensionSupported(num_filenames)) {
    LOG(DFATAL) << "Cannot have more filenames than channel dimensions!";
    return kNullAnimation;
  }

  // Some spline animations actually extract the splines from rig animations. We
  // assume that the dimensions of the channel are the dimensions desired for
  // the animation (rather than basing the number of channels on the number of
  // files).
  const size_t dimensions = channel->GetDimensions();
  std::vector<float> constants(dimensions, 0.f);
  std::vector<const motive::CompactSpline*> splines(dimensions, nullptr);

  for (int i = 0; i < num_filenames; ++i) {
    AnimationAssetPtr asset;
    std::string filename = anim->filenames()->Get(i)->str();
    int list_index = 0;
    SplitListFilenameAndIndex(&filename, &list_index);
    if (!filename.empty()) {
      asset = LoadAnimation(filename);
      if (!asset) {
        LOG(DFATAL) << "Could not load animation: " << filename;
      }
    }
    if (asset) {
      // TODO: Correctly merge multiple animation assets so that we
      // don't overwrite assets with multiple splines here. Here we allow the
      // ith spline to populate dimensions - i channels. Subsequent files will
      // overwrite all but the first spline in each file.
      asset->GetSplinesAndConstants(list_index,
                                    static_cast<int>(dimensions - i),
                                    channel->GetOperations(), &splines[i],
                                    &constants[i]);
    }
  }

  const AnimationId id = GenerateAnimationId();
  const PlaybackParameters params = GetPlaybackParameters(anim);
  const SplineModifiers modifiers = GetSplineModifiers(anim);
  const AnimationId prev_id =
      channel->Play(e, &engine_, id, splines.data(), constants.data(),
                    dimensions, params, modifiers, nullptr);
  UntrackAnimation(prev_id, AnimationCompletionReason::kInterrupted);
  return id;
}

AnimationId AnimationSystem::PlaySplineAnimationInternal(
    Entity e, AnimationChannel* channel, const AnimationAssetPtr& anim,
    const PlaybackParameters& params) {
  if (channel == nullptr || channel->IsRigChannel()) {
    LOG(DFATAL) << "Invalid channel.";
    return kNullAnimation;
  } else if (anim == nullptr) {
    LOG(DFATAL) << "No animation specified!";
    return kNullAnimation;
  } else if (!channel->IsDimensionSupported(anim->GetNumCompactSplines())) {
    LOG(DFATAL) << "Cannot have more splines than channel dimensions!";
    return kNullAnimation;
  }

  // Animate as many dimensions as the channel allows. If it is a dynamic
  // dimension channel, animate as many compact splines as the asset has.
  size_t dimensions = channel->GetDimensions();
  if (dimensions == AnimationChannel::kDynamicDimensions) {
    dimensions = anim->GetNumCompactSplines();
  }
  std::vector<float> constants(dimensions, 0.f);
  std::vector<const motive::CompactSpline*> splines(dimensions, nullptr);

  anim->GetSplinesAndConstants(0, static_cast<int>(dimensions),
                               channel->GetOperations(), splines.data(),
                               constants.data());

  const AnimationId id = GenerateAnimationId();
  const AnimationId prev_id =
      channel->Play(e, &engine_, id, splines.data(), constants.data(),
                    dimensions, params, SplineModifiers(), anim->GetContext());
  UntrackAnimation(prev_id, AnimationCompletionReason::kInterrupted);
  return id;
}

AnimationId AnimationSystem::PlayRigAnimationInternal(
    Entity e, AnimationChannel* channel, const AnimationAssetPtr& anim,
    const PlaybackParameters& params, int rig_index) {
  if (channel == nullptr || !channel->IsRigChannel()) {
    LOG(DFATAL) << "Invalid channel.";
    return kNullAnimation;
  } else if (anim == nullptr) {
    LOG(DFATAL) << "No animation specified!";
    return kNullAnimation;
  }

  const motive::RigAnim* rig_anim = anim->GetRigAnim(rig_index);
  if (!rig_anim) {
    LOG(DFATAL) << "Animation is not a rig animation.";
    return kNullAnimation;
  }

  const AnimationId id = GenerateAnimationId();
  const AnimationId prev_id =
      channel->Play(e, &engine_, id, rig_anim, params, anim->GetContext());
  UntrackAnimation(prev_id, AnimationCompletionReason::kInterrupted);
  return id;
}

AnimationId AnimationSystem::SetTargetInternal(Entity e,
                                               AnimationChannel* channel,
                                               const float* data, size_t len,
                                               Clock::duration time,
                                               Clock::duration delay) {
  if (!channel->IsDimensionSupported(len)) {
    LOG(DFATAL) << "Target data size does not match channel dimensions.  "
                   "Skipping playback.";
    return kNullAnimation;
  }

  const AnimationId id = GenerateAnimationId();
  const AnimationId prev_id =
      channel->Play(e, &engine_, id, data, len, time, delay);
  UntrackAnimation(prev_id, AnimationCompletionReason::kInterrupted);
  return id;
}

AnimationId AnimationSystem::GenerateAnimationId() {
  AnimationId id = ++current_id_;
  if (id == kNullAnimation) {
    // In case of int overflow, just wrap around and skip kNullAnimation.
    id = ++current_id_;
  }
  return id;
}

AnimationId AnimationSystem::TrackAnimations(Entity entity, AnimationSet anims,
                                             const AnimationDef* data) {
  if (anims.empty()) {
    return kNullAnimation;
  }

  const AnimationId external_id = GenerateAnimationId();
  for (auto& internal_id : anims) {
    internal_to_external_ids_[internal_id] = external_id;
  }

  AnimationSetEntry& entry = external_id_to_entry_[external_id];
  DCHECK_EQ(entry.entity, kNullEntity);
  DCHECK(entry.animations.empty());

  entry.entity = entity;
  entry.animations = std::move(anims);
  entry.data = data;
  return external_id;
}

void AnimationSystem::UntrackAnimation(AnimationId internal_id,
                                       AnimationCompletionReason reason) {
  if (internal_id == kNullAnimation) {
    return;
  }

  auto iter = internal_to_external_ids_.find(internal_id);
  if (iter == internal_to_external_ids_.end()) {
    LOG(DFATAL) << "Stopping animation that never started?";
    return;
  }

  const AnimationId external_id = iter->second;
  internal_to_external_ids_.erase(iter);

  AnimationSetEntry& entry = external_id_to_entry_[external_id];
  entry.animations.erase(internal_id);
  if (entry.animations.empty()) {
    const Entity entity = entry.entity;
    const AnimationDef* data = entry.data;
    external_id_to_entry_.erase(external_id);

    const AnimationCompleteEvent event(entity, external_id, reason);
    SendEvent(registry_, entity, event);

    if (data) {
      SendEventDefs(registry_, entity, data->on_complete_events());
      if (reason == AnimationCompletionReason::kCompleted) {
        SendEventDefs(registry_, entity, data->on_success_events());
      } else {
        SendEventDefs(registry_, entity, data->on_cancelled_events());
      }
    }
  }
}

AnimationChannel* AnimationSystem::FindChannel(string_view channel_name) {
  return FindChannel(Hash(channel_name));
}

AnimationChannel* AnimationSystem::FindChannel(HashValue channel_id) {
  auto iter = channels_.find(channel_id);
  return iter != channels_.end() ? iter->second.get() : nullptr;
}

SplineModifiers AnimationSystem::GetSplineModifiers(
    const AnimInstanceDef* anim) {
  SplineModifiers modifiers;
  if (anim->offset()) {
    modifiers.offsets = anim->offset()->data();
    modifiers.num_offsets = anim->offset()->size();
  }
  if (anim->multiplier()) {
    modifiers.multipliers = anim->multiplier()->data();
    modifiers.num_multipliers = anim->multiplier()->size();
  }
  return modifiers;
}

PlaybackParameters AnimationSystem::GetPlaybackParameters(
    const AnimInstanceDef* anim) {
  PlaybackParameters params;
  params.looping = anim->looping() != 0;
  params.speed = anim->speed();
  params.start_delay_s = anim->start_delay();
  params.blend_time_s = anim->blend_time();
  return params;
}

Clock::duration AnimationSystem::GetDurationFromMotiveTime(
    motive::MotiveTime time) {
  return std::chrono::duration_cast<Clock::duration>(MotiveTimeUnit(time));
}

motive::MotiveTime AnimationSystem::GetMotiveTimeFromDuration(
    Clock::duration timestep) {
  return std::chrono::duration_cast<MotiveTimeUnit>(timestep).count();
}

motive::MotiveTime AnimationSystem::GetMotiveTimeFromSeconds(float seconds) {
  return std::chrono::duration_cast<MotiveTimeUnit>(Secondsf(seconds)).count();
}

motive::MotiveTime AnimationSystem::GetMinimalStep() { return 1; }

float AnimationSystem::GetMotiveDerivativeFromSeconds(float derivative) {
  return derivative /
         std::chrono::duration_cast<MotiveTimeUnit>(Secondsf(1)).count();
}

void AnimationSystem::SetPlaybackRate(Entity entity, HashValue channel,
                                      float rate) {
  auto iter = channels_.find(channel);
  if (iter == channels_.end()) {
    LOG(WARNING) << "Could not find channel " << channel;
    return;
  }

  iter->second->SetPlaybackRate(entity, rate);
}

void AnimationSystem::SetLooping(Entity e, HashValue channel, bool loop) {
  auto iter = channels_.find(channel);
  if (iter == channels_.end()) {
    LOG(WARNING) << "Could not find channel " << channel;
    return;
  }

  iter->second->SetLooping(e, loop);
}

void AnimationSystem::SetSkeleton(Entity entity, Skeleton skeleton) {
  auto res = skeletons_.emplace(entity, SkeletonComponent());
  if (!res.second) {
    LOG(ERROR) << "Cannot replace an Entity's skeleton.";
    return;
  }

  res.first->second.entities.assign(skeleton.begin(), skeleton.end());
}

AnimationSystem::Skeleton AnimationSystem::GetSkeleton(Entity entity) {
  auto iter = skeletons_.find(entity);
  if (iter != skeletons_.end()) {
    return iter->second.entities;
  }
  return {};
}

void AnimationSystem::SplitListFilenameAndIndex(std::string* filename,
                                                int* index) {
  if (!filename) {
    LOG(DFATAL) << "Could not find file " << filename;
    return;
  }
  if (!index) {
    LOG(DFATAL) << "Cannot have null index!";
    return;
  }
  const size_t ext_pos = filename->find_last_of('.');
  const size_t index_pos = filename->find_last_of(':');
  if (ext_pos != std::string::npos && index_pos != std::string::npos &&
      index_pos > ext_pos &&
      IsMotiveListFile(*filename, ext_pos + 1, index_pos)) {
    *index = std::stoi(filename->substr(index_pos + 1));
    filename->resize(index_pos);
  }
}

}  // namespace lull
