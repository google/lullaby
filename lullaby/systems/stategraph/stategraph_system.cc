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

#include "lullaby/systems/stategraph/stategraph_system.h"

#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/generated/stategraph_def_generated.h"

namespace lull {

static constexpr HashValue kStategraphDef = ConstHash("StategraphDef");

StategraphSystem::StategraphSystem(Registry* registry)
    : System(registry), components_(32) {
  RegisterDef(this, kStategraphDef);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Stategraph.SetSelectionArgs",
                           &lull::StategraphSystem::SetSelectionArgs);
    binder->RegisterMethod("lull.Stategraph.SetDesiredState",
                           &lull::StategraphSystem::SetDesiredState);
    binder->RegisterMethod("lull.Stategraph.SnapToState",
                           &lull::StategraphSystem::SnapToState);
    binder->RegisterMethod("lull.Stategraph.SnapToStateAtSignal",
                           &lull::StategraphSystem::SnapToStateAtSignal);
    binder->RegisterMethod("lull.Stategraph.SnapToStateAtTime",
                           &lull::StategraphSystem::SnapToStateAtTime);
  }
}

StategraphSystem::~StategraphSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Stategraph.SetSelectionArgs");
    binder->UnregisterFunction("lull.Stategraph.SetDesiredState");
    binder->UnregisterFunction("lull.Stategraph.SnapToState");
    binder->UnregisterFunction("lull.Stategraph.SnapToStateAtSignal");
    binder->UnregisterFunction("lull.Stategraph.SnapToStateAtTime");
  }
}

void StategraphSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kStategraphDef) {
    LOG(DFATAL) << "Invalid blueprint type: " << type;
    return;
  }

  StategraphComponent* component = components_.Emplace(entity);
  if (component == nullptr) {
    LOG(DFATAL) << "Could not create Stategraphcomponent->";
    return;
  }

  const StategraphDef* data = ConvertDef<StategraphDef>(def);
  if (data->animation_stategraph()) {
    component->stategraph =
        LoadStategraph(data->animation_stategraph()->c_str());
    component->current_state = data->initial_state();
  }
  component->env = MakeUnique<ScriptEnv>();

  auto* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    component->env->SetFunctionCallHandler(
        [binder](FunctionCall* call) { binder->Call(call); });
    component->env->SetValue(Symbol("entity"), ScriptValue::Create(entity));
  }
}

void StategraphSystem::Destroy(Entity entity) { components_.Destroy(entity); }

void StategraphSystem::AdvanceFrame(Clock::duration delta_time) {
  components_.ForEach([this, delta_time](StategraphComponent& component) {
    AdvanceFrame(&component, delta_time);
  });
}

void StategraphSystem::SetSelectionArgs(Entity entity, const VariantMap& args) {
  StategraphComponent* component = components_.Get(entity);
  if (component) {
    component->selection_args = args;
  }
}

void StategraphSystem::SetDesiredState(Entity entity, HashValue state) {
  StategraphComponent* component = components_.Get(entity);
  if (component) {
    UpdatePathToTargetState(component, state);
  }
}

void StategraphSystem::SnapToState(Entity entity, HashValue state) {
  StategraphComponent* component = components_.Get(entity);
  if (component) {
    component->path.clear();
    EnterState(component, state, 0, Clock::duration(0), Clock::duration(0));
  }
}

void StategraphSystem::SnapToStateAtSignal(Entity entity, HashValue state,
                                           HashValue signal) {
  StategraphComponent* component = components_.Get(entity);
  if (component) {
    component->path.clear();
    EnterState(component, state, signal, Clock::duration(0),
               Clock::duration(0));
  }
}

void StategraphSystem::SnapToStateAtTime(Entity entity, HashValue state,
                                         Clock::duration timestamp) {
  StategraphComponent* component = components_.Get(entity);
  if (component) {
    component->path.clear();
    EnterState(component, state, 0, timestamp, Clock::duration(0));
  }
}

void StategraphSystem::EnterState(StategraphComponent* component,
                                  HashValue state, HashValue signal,
                                  Clock::duration timestamp,
                                  Clock::duration blend_time) {
  CHECK_NOTNULL(component);

  // Exit the current track.
  if (component->track) {
    component->track->ExitActiveSignals(component->time,
                                        TypedPointer(component->env.get()));
  }

  // Select a new track.
  if (component->env) {
    component->selection_args[kLullscriptEnvHash] =
        reinterpret_cast<uint64_t>(component->env.get());
  } else {
    component->selection_args.erase(kLullscriptEnvHash);
  }
  component->track =
      component->stategraph->SelectTrack(state, component->selection_args);
  if (component->track == nullptr) {
    // The stategraph may return a nullptr if the stategraph isn't fully loaded
    // yet, so just early out instead of erroring.  We'll keep hitting this
    // case until we've got a stategraph we can play, at which point we'll
    // start at the current_state.
    return;
  }

  // Determine the time to start the track playback.
  Clock::duration enter_time = timestamp;
  if (signal) {
    const StategraphSignal* signal_ptr = component->track->GetSignal(signal);
    if (signal_ptr) {
      enter_time = signal_ptr->GetStartTime();
    }
  }
  component->time =
      component->stategraph->AdjustTime(enter_time, component->track);

  // Enter the new track and start playing the animation at the correct time.
  component->current_state = state;
  component->track->EnterActiveSignals(component->time,
                                       TypedPointer(component->env.get()));
  component->stategraph->PlayTrack(component->GetEntity(), component->track,
                                   component->time, blend_time);
}

void StategraphSystem::UpdatePathToTargetState(StategraphComponent* component,
                                               HashValue state) {
  CHECK_NOTNULL(component);
  if (GetTargetState(component) == state) {
    return;
  }

  component->path.clear();
  if (component->current_state != state) {
    component->path =
        component->stategraph->FindPath(component->current_state, state);
    if (component->path.empty()) {
      LOG(DFATAL) << "No path to target state: " << state;
    }
  }
}

void StategraphSystem::AdvanceFrame(StategraphComponent* component,
                                    Clock::duration delta_time) {
  CHECK_NOTNULL(component);
  if (!component->stategraph->IsReady()) {
    return;
  }
  if (component->track == nullptr) {
    // If there is no current animation playing, snap to the current state.
    // This usually occurs the first time we add a stategraph to this Entity
    // (and until the stategraph data is loaded).
    SnapToState(component->GetEntity(), component->current_state);
  }

  const StategraphTransition* transition = GetNextTransition(component);
  if (transition == nullptr) {
    LOG(DFATAL) << "No more transitions. Will get stuck at current state?";
    return;
  }

  // Enter the next state specified by the transition if it's valid to "take"
  // the transition now.
  const Optional<HashValue> valid_transition =
      component->stategraph->IsTransitionValid(*transition, component->track,
                                               component->time);
  if (valid_transition) {
    if (!component->path.empty()) {
      component->path.pop_front();
    }
    EnterState(component, transition->to_state, *valid_transition,
               Clock::duration(0), transition->transition_time);
  }

  // Advance the track playback by the given time, adjusting for any track
  // specific attributes.
  const Clock::duration adjusted_delta_time =
      component->stategraph->AdjustTime(delta_time, component->track);
  const Clock::duration next_time = component->time + adjusted_delta_time;
  component->track->ProcessSignals(component->time, next_time,
                                   TypedPointer(component->env.get()));
  component->time = next_time;
}

const StategraphTransition* StategraphSystem::GetNextTransition(
    const StategraphComponent* component) const {
  CHECK_NOTNULL(component);
  if (component->path.empty()) {
    return component->stategraph->GetDefaultTransition(
        component->current_state);
  } else {
    return &component->path.front();
  }
}

HashValue StategraphSystem::GetTargetState(
    const StategraphComponent* component) const {
  CHECK_NOTNULL(component);
  return component->path.empty() ? component->current_state
                                 : component->path.back().to_state;
}

std::shared_ptr<StategraphAsset> StategraphSystem::LoadStategraph(
    string_view filename) {
  const HashValue key = Hash(filename);
  return assets_.Create(key, [this, filename]() {
    AssetLoader* asset_loader = registry_->Get<AssetLoader>();
    return asset_loader->LoadAsync<StategraphAsset>(filename.to_string(),
                                                    registry_);
  });
}

}  // namespace lull
