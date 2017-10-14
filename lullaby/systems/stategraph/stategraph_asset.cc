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

#include "lullaby/systems/stategraph/stategraph_asset.h"

#include "lullaby/generated/animation_stategraph_generated.h"
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/random_number_generator.h"

namespace lull {
namespace {

constexpr HashValue kWeightKey = lull::ConstHash("weight");

// TrackSelector that assumes a single track and returns its index.
class FirstAnimationSelector : public StategraphState::TrackSelector {
  Optional<size_t> Select(const VariantMap& params,
                          Span<Type> choices) override {
    DCHECK_EQ(choices.size(), size_t(1));
    return choices.empty() ? Optional<size_t>() : size_t(0);
  }
};

// TrackSelector that randomly chooses among tracks.  If the track
// has selection data with a float 'weight' key, the value will be used
// to weight the random selection.
class RandomAnimationSelector : public StategraphState::TrackSelector {
 public:
  explicit RandomAnimationSelector(RandomNumberGenerator* rng) : rng_(rng) {
    DCHECK(rng_);
  }

  Optional<size_t> Select(const VariantMap& params,
                          Span<Type> choices) override {
    Optional<size_t> ret;
    float total_weight = 0.0f;
    for (size_t choice = 0; choice < choices.size(); ++choice) {
      float weight = 1.0f;
      const auto& choice_params = choices[choice]->GetSelectionParams();
      const auto it = choice_params.find(kWeightKey);
      if (it != choice_params.end()) {
        weight = it->second.ValueOr(1.0f);
      }
      total_weight += weight;
      if (rng_->GenerateUniform(0.0f, total_weight) <= weight) {
        ret = choice;
      }
    }
    return ret;
  }

 private:
  RandomNumberGenerator* rng_;
};

// TrackSelector that runs a script to choose among tracks.
class ScriptedAnimationSelector : public StategraphState::TrackSelector {
 public:
  explicit ScriptedAnimationSelector(const ScriptedAnimationSelectorDef* def) {
    ScriptEnv compiler;
    if (def->code()) {
      code_ = compiler.Read(def->code()->c_str());
    }
  }

  Optional<size_t> Select(const VariantMap& params,
                          Span<Type> choices) override {
    Optional<size_t> ret;
    auto env_it = params.find(kLullscriptEnvHash);
    if (env_it != params.end()) {
      auto* env =
          reinterpret_cast<ScriptEnv*>(env_it->second.ValueOr(uint64_t(0)));
      if (env) {
        VariantArray tracks;
        tracks.reserve(choices.size());
        for (const auto& choice : choices) {
          tracks.emplace_back(choice->GetSelectionParams());
        }
        ScriptEnvScope scope(env);
        env->SetValue(Symbol("tracks"), env->Create(tracks));
        ret = env->Eval(code_).NumericCast<size_t>();
      }
    }
    return ret;
  }

 private:
  ScriptValue code_;
};

// A Stategraph track that also has a pointer to an animation to play and
// information about how to play it.
class AnimationTrack : public StategraphTrack {
 public:
  AnimationTrack() : StategraphTrack() {}
  void SetSelectionParams(VariantMap params) {
    selection_params_ = std::move(params);
  }
  void AddSignal(std::unique_ptr<StategraphSignal> signal) {
    signals_.emplace_back(std::move(signal));
  }
  AnimationAssetPtr asset_ = nullptr;
  HashValue channel_ = ConstHash("render-rig");
  Clock::duration total_time_ = Clock::duration(0);
  float playback_speed_ = 0.f;
};

// A Stategraph state.
class AnimationState : public StategraphState {
 public:
  explicit AnimationState(HashValue id) : StategraphState(id) {}
  void SetSelector(std::unique_ptr<TrackSelector> selector) {
    StategraphState::SetSelector(std::move(selector));
  }
  void AddTrack(std::unique_ptr<StategraphTrack> track) {
    StategraphState::AddTrack(std::move(track));
  }
  void AddTransition(StategraphTransition transition) {
    StategraphState::AddTransition(std::move(transition));
  }
  size_t default_transition_index_ = 0;
};

// A Stategraph signal that uses the ScriptEngine for triggering Enter/Exit
// callbacks.
class AnimationSignal : public StategraphSignal {
 public:
  AnimationSignal(HashValue id, Clock::duration start_time,
                  Clock::duration end_time)
      : StategraphSignal(id, start_time, end_time) {}
  void Enter(TypedPointer userdata) override;
  void Exit(TypedPointer userdata) override;
  ScriptValue on_enter_;
  ScriptValue on_exit_;
};

void AnimationSignal::Enter(TypedPointer userdata) {
  ScriptEnv* env = userdata.Get<ScriptEnv>();
  if (env) {
    env->Eval(on_enter_);
  }
}

void AnimationSignal::Exit(TypedPointer userdata) {
  ScriptEnv* env = userdata.Get<ScriptEnv>();
  if (env) {
    env->Eval(on_exit_);
  }
}

}  // namespace

void StategraphAsset::OnFinalize(const std::string& filename,
                                 std::string* data) {
  if (!data || data->empty()) {
    LOG(DFATAL) << "Could not load stategraph.";
    return;
  }

  const AnimationStategraphDef* stategraph_def =
      GetAnimationStategraphDef(data->data());
  if (stategraph_def->states() == nullptr) {
    LOG(DFATAL) << "No states in graph.";
    return;
  }

  stategraph_ = MakeUnique<Stategraph>();
  for (const AnimationStateDef* state_def : *stategraph_def->states()) {
    stategraph_->AddState(CreateState(state_def));
  }
}

StategraphTransition StategraphAsset::CreateTransition(
    HashValue from_state, const AnimationTransitionDef* def) const {
  StategraphTransition transition;
  if (def != nullptr) {
    transition.from_state = from_state;
    transition.to_state = def->to_state();
    if (def->active_for_entire_time() || def->active_time_from_end_s() < 0) {
      transition.active_time_from_end = Clock::duration::max();
    } else {
      transition.active_time_from_end =
          DurationFromSeconds(def->active_time_from_end_s());
    }
    transition.transition_time = DurationFromSeconds(def->blend_time_s());
    const auto* signals = def->signals();
    if (signals) {
      transition.signals.reserve(signals->size());
      for (auto iter : *signals) {
        const HashValue from_signal = iter->from_signal();
        const HashValue to_signal = iter->to_signal();
        transition.signals.emplace_back(std::make_pair(from_signal, to_signal));
      }
    }
  }
  return transition;
}

std::unique_ptr<StategraphSignal> StategraphAsset::CreateSignal(
    const AnimationSignalDef* def) const {
  if (def == nullptr) {
    return nullptr;
  }

  const HashValue id = def->id();
  const Clock::duration start_time = DurationFromSeconds(def->start_time_s());
  const Clock::duration end_time = DurationFromSeconds(def->end_time_s());
  AnimationSignal* signal = new AnimationSignal(id, start_time, end_time);

  ScriptEnv compiler;
  if (def->on_enter()) {
    signal->on_enter_ = compiler.Read(def->on_enter()->c_str());
  }
  if (def->on_exit()) {
    signal->on_exit_ = compiler.Read(def->on_exit()->c_str());
  }

  return std::unique_ptr<StategraphSignal>(signal);
}

std::unique_ptr<StategraphTrack> StategraphAsset::CreateTrack(
    const AnimationTrackDef* def) const {
  if (def == nullptr) {
    return nullptr;
  }
  if (!def->animation()) {
    LOG(DFATAL) << "Animation must be specified in track.";
    return nullptr;
  }

  auto* animation_system = registry_->Get<AnimationSystem>();
  AnimationAssetPtr asset =
      animation_system->LoadAnimation(def->animation()->str());
  if (!asset) {
    LOG(DFATAL) << "Could not load animation.";
    return nullptr;
  }
  if (asset->GetNumRigAnims() != 1) {
    LOG(DFATAL) << "Animation asset should contain a single rig animation.";
  }
  AnimationTrack* track = new AnimationTrack();
  track->asset_ = asset;
  track->total_time_ = AnimationSystem::GetDurationFromMotiveTime(
      asset->GetRigAnim(0)->end_time());
  track->playback_speed_ = def->playback_speed();
  if (def->animation_channel() != 0) {
    track->channel_ = def->animation_channel();
  }

  VariantMap selection_params;
  VariantMapFromFbVariantMap(def->selection_params(), &selection_params);
  track->SetSelectionParams(std::move(selection_params));

  if (def->signals()) {
    for (const AnimationSignalDef* signal_def : *def->signals()) {
      track->AddSignal(CreateSignal(signal_def));
    }
  }
  return std::unique_ptr<StategraphTrack>(track);
}

std::unique_ptr<StategraphState> StategraphAsset::CreateState(
    const AnimationStateDef* def) const {
  if (def == nullptr) {
    return nullptr;
  }

  const HashValue id = def->id();

  AnimationState* state = new AnimationState(id);
  state->SetSelector(CreateSelector(def->selector_type(), def->selector()));

  if (def->tracks()) {
    for (const AnimationTrackDef* track_def : *def->tracks()) {
      state->AddTrack(CreateTrack(track_def));
    }
  }
  if (def->transitions()) {
    for (const AnimationTransitionDef* transition_def : *def->transitions()) {
      state->AddTransition(CreateTransition(id, transition_def));
    }
  }
  state->default_transition_index_ = def->default_transition_index();

  return std::unique_ptr<StategraphState>(state);
}

std::unique_ptr<StategraphAsset::TrackSelector> StategraphAsset::CreateSelector(
    AnimationSelectorDef type, const void* def) const {
  StategraphAsset::TrackSelector* ptr = nullptr;
  switch (type) {
    case AnimationSelectorDef_FirstAnimationSelectorDef:
      ptr = new FirstAnimationSelector();
      break;
    case AnimationSelectorDef_RandomAnimationSelectorDef:
      ptr =
          new RandomAnimationSelector(registry_->Get<RandomNumberGenerator>());
      break;
    case AnimationSelectorDef_ScriptedAnimationSelectorDef:
      ptr = new ScriptedAnimationSelector(
          reinterpret_cast<const ScriptedAnimationSelectorDef*>(def));
      break;
    default:
      LOG(ERROR) << "Unknown selector type: " << type;
      break;
  }
  return std::unique_ptr<StategraphAsset::TrackSelector>(ptr);
}

const StategraphTransition* StategraphAsset::GetDefaultTransition(
    HashValue state) const {
  if (stategraph_ == nullptr) {
    return nullptr;
  }
  const StategraphState* state_ptr = stategraph_->GetState(state);
  if (state_ptr == nullptr) {
    return nullptr;
  }
  const std::vector<StategraphTransition>& transitions =
      state_ptr->GetTransitions();
  const size_t default_index =
      static_cast<const AnimationState*>(state_ptr)->default_transition_index_;
  if (default_index >= transitions.size()) {
    return nullptr;
  }
  return &transitions[default_index];
}

Stategraph::Path StategraphAsset::FindPath(HashValue from_state,
                                           HashValue to_state) const {
  return stategraph_ ? stategraph_->FindPath(from_state, to_state)
                     : Stategraph::Path();
}

const StategraphTrack* StategraphAsset::SelectTrack(
    HashValue state, const VariantMap& args) const {
  if (stategraph_ == nullptr) {
    return nullptr;
  }
  const StategraphState* state_ptr = stategraph_->GetState(state);
  if (state_ptr == nullptr) {
    return nullptr;
  }
  return state_ptr->SelectTrack(args);
}

Clock::duration StategraphAsset::AdjustTime(
    Clock::duration time, const StategraphTrack* track) const {
  const AnimationTrack* anim_track = static_cast<const AnimationTrack*>(track);
  if (anim_track == nullptr) {
    return time;
  }
  const Clock::rep adjusted_count =
      static_cast<Clock::rep>(time.count() * anim_track->playback_speed_);
  return Clock::duration(adjusted_count);
}

AnimationId StategraphAsset::PlayTrack(Entity entity,
                                       const StategraphTrack* track,
                                       Clock::duration timestamp,
                                       Clock::duration blend_time) const {
  const AnimationTrack* anim_track = static_cast<const AnimationTrack*>(track);
  if (anim_track == nullptr) {
    return kNullAnimation;
  }

  auto* animation_system = registry_->Get<AnimationSystem>();
  if (animation_system == nullptr) {
    return kNullAnimation;
  }

  PlaybackParameters params;
  params.speed = anim_track->playback_speed_;
  params.blend_time_s = SecondsFromDuration(blend_time);
  params.start_delay_s = -1.f * SecondsFromDuration(timestamp);
  return animation_system->PlayAnimation(entity, anim_track->channel_,
                                         anim_track->asset_, params);
}

Optional<HashValue> StategraphAsset::IsTransitionValid(
    const StategraphTransition& transition, const StategraphTrack* track,
    Clock::duration timestamp) const {
  const AnimationTrack* anim_track = static_cast<const AnimationTrack*>(track);
  if (anim_track == nullptr) {
    return kNullAnimation;
  }

  const Clock::duration total_time = anim_track->total_time_;
  if (total_time - timestamp <= transition.active_time_from_end) {
    return transition.to_signal;
  }
  for (const auto& pair : transition.signals) {
    const StategraphSignal* signal = anim_track->GetSignal(pair.first);
    if (signal && signal->IsActive(timestamp)) {
      return pair.second;
    }
  }
  return Optional<HashValue>();
}

}  // namespace lull
