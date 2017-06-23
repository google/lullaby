/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "motive/init.h"
#include "motive/spline_anim_generated.h"
#include "lullaby/base/asset_loader.h"
#include "lullaby/base/dispatcher.h"
#include "lullaby/events/animation_events.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace {

const HashValue kAnimationDef = Hash("AnimationDef");
const HashValue kAnimationResponseDef = Hash("AnimationResponseDef");
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
  RegisterDef(this, kAnimationDef);
  RegisterDef(this, kAnimationResponseDef);

  motive::RigInit::Register();
  motive::MatrixInit::Register();
  motive::SplineInit::Register();

  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this, [this](const CancelAllAnimationsEvent& event) {
      CancelAllAnimations(event.entity);
    });
  }
}

AnimationSystem::~AnimationSystem() {
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

void AnimationSystem::AdvanceFrame(Clock::duration delta_time) {
  LULLABY_CPU_TRACE_CALL();
  const motive::MotiveTime timestep = GetMotiveTime(delta_time);
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

AnimationId AnimationSystem::SetTarget(Entity e, HashValue channel,
                                       const float* data, size_t len,
                                       Clock::duration time) {
  AnimationId id = SetTargetInternal(e, channel, data, len, time);
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

  const HashValue channel = Hash(target->channel()->c_str());
  const float* values = target->values()->data();
  const size_t len = target->values()->size();
  const std::chrono::milliseconds time(target->time_ms());
  return SetTargetInternal(e, channel, values, len, time);
}

AnimationId AnimationSystem::PlayAnimation(Entity e,
                                           const AnimInstanceDef* anim) {
  const HashValue channel_id = Hash(anim->channel()->c_str());
  auto iter = channels_.find(channel_id);
  if (iter == channels_.end()) {
    LOG(DFATAL) << "Could not find channel: " << anim->channel()->c_str();
    return kNullAnimation;
  }

  if (iter->second->IsRigChannel()) {
    return PlayRigAnimation(e, anim, iter->second);
  } else {
    return PlaySplineAnimation(e, anim, iter->second);
  }
}

AnimationId AnimationSystem::PlayRigAnimation(
    Entity e, const AnimInstanceDef* anim, const AnimationChannelPtr& channel) {
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

  const motive::RigAnim* rig_anim =
      asset ? asset->GetRigAnim(list_index) : nullptr;
  if (!rig_anim) {
    LOG(DFATAL) << "Animation is not a rig animation: " << filename;
    return kNullAnimation;
  }

  const AnimationId id = GenerateAnimationId();
  const PlaybackParameters params = GetPlaybackParameters(anim);
  const AnimationId prev_id = channel->Play(e, &engine_, id, rig_anim, params);
  UntrackAnimation(prev_id, AnimationCompletionReason::kInterrupted);
  return id;
}

AnimationId AnimationSystem::PlaySplineAnimation(
    Entity e, const AnimInstanceDef* anim, const AnimationChannelPtr& channel) {
  float constants[AnimationChannel::kMaxDimensions] = {0.0f};
  const motive::CompactSpline* splines[AnimationChannel::kMaxDimensions] = {
      nullptr};

  const int dimensions = static_cast<int>(channel->GetDimensions());
  const int num_filenames = static_cast<int>(anim->filenames()->size());
  if (num_filenames > dimensions) {
    LOG(DFATAL) << "Cannot have more filenames than channel dimensions!";
    return kNullAnimation;
  }

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
      // TODO(b/30696097): Correctly merge multiple animation assets so that we
      // don't overwrite assets with multiple splines here. Here we allow the
      // ith spline to populate dimensions - i channels. Subsequent files will
      // overwrite all but the first spline in each file.
      asset->GetSplinesAndConstants(list_index, dimensions - i,
                                    channel->GetOperations(), splines + i,
                                    constants + i);
    }
  }

  const AnimationId id = GenerateAnimationId();
  const PlaybackParameters params = GetPlaybackParameters(anim);
  const AnimationId prev_id = channel->Play(e, &engine_, id, splines, constants,
                                            channel->GetDimensions(), params);
  UntrackAnimation(prev_id, AnimationCompletionReason::kInterrupted);
  return id;
}

AnimationId AnimationSystem::SetTargetInternal(Entity e, HashValue channel,
                                               const float* data, size_t len,
                                               Clock::duration time) {
  auto iter = channels_.find(channel);
  if (iter == channels_.end()) {
    LOG(DFATAL) << "Could not find channel: " << channel;
    return kNullAnimation;
  }
  if (iter->second->GetDimensions() != len) {
    LOG(DFATAL) << "Target data size does not match channel dimensions.  "
                   "Skipping playback.";
    return kNullAnimation;
  }

  const AnimationId id = GenerateAnimationId();
  const AnimationId prev_id =
      iter->second->Play(e, &engine_, id, data, len, time);
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

    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (dispatcher_system) {
      const AnimationCompleteEvent event(entity, external_id, reason);
      dispatcher_system->Send(entity, event);
    }

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

PlaybackParameters AnimationSystem::GetPlaybackParameters(
    const AnimInstanceDef* anim) {
  PlaybackParameters params;
  params.looping = anim->looping() != 0;
  params.speed = anim->speed();
  params.start_delay_s = anim->start_delay();
  if (anim->offset()) {
    params.offsets = anim->offset()->data();
    params.num_offsets = anim->offset()->size();
  }
  if (anim->multiplier()) {
    params.multipliers = anim->multiplier()->data();
    params.num_multipliers = anim->multiplier()->size();
  }
  return params;
}

motive::MotiveTime AnimationSystem::GetMotiveTime(
    const Clock::duration& timestep) {
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
