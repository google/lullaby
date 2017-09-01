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

#ifndef LULLABY_SYSTEMS_STATEGRAPH_STATEGRAPH_SYSTEM_H_
#define LULLABY_SYSTEMS_STATEGRAPH_STATEGRAPH_SYSTEM_H_

#include <memory>
#include <unordered_map>
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/entity.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/systems/stategraph/stategraph_asset.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

/// The StategraphSystem is responsible for advancing and tracking the progress
/// of an Entity within a stategraph.
///
/// A stategraph is a collection of states and transitions that allows entity
/// animations to be played and transitioned at a higher level.  Instead of
/// focusing on the specific animation to play at a given time, the logic of
/// playing and transitioning animations is encoded in a graph.  Users of the
/// StategraphSystem can then issue requests to transition to a desired state
/// (eg. idle, walk, jump, etc.), and all the logic of how to reach that desired
/// state will be handled by the StategraphSystem.
class StategraphSystem : public System {
 public:
  explicit StategraphSystem(Registry* registry);

  ~StategraphSystem() override;

  /// Associates the entity with a Stategraph as specified by the blueprint.
  void Create(Entity entity, HashValue type, const Def* def) override;

  /// Disassociates entity from all stategraphs.
  void Destroy(Entity entity) override;

  /// Updates the progress of all associated Entities through their individual
  /// stategraphs.
  void AdvanceFrame(Clock::duration delta_time);

  /// Sets the arguments that will be used to select a track to play when
  /// entering a new animation state.
  void SetSelectionArgs(Entity entity, const VariantMap& args);

  /// Sets a new destination/target state for an Entity in its stategraph.
  void SetDesiredState(Entity entity, HashValue state);

  /// Forces an Entity to enter the specified state, bypassing all other logic
  /// and transitions.
  void SnapToState(Entity entity, HashValue state);

  /// Forces an Entity to enter the specified state, skipping to the beginning
  /// of the given signal, bypassing all other logic and transitions.
  void SnapToStateAtSignal(Entity entity, HashValue state, HashValue signal);

  /// Forces an Entity to enter the specified state, skipping to the specified
  /// time, bypassing all other logic and transitions.
  void SnapToStateAtTime(Entity entity, HashValue state,
                         Clock::duration timestamp);

  StategraphSystem(const StategraphSystem&) = delete;
  StategraphSystem& operator=(const StategraphSystem&) = delete;

 private:
  /// Associates a stategraph with an Entity and tracks the progress of the
  /// Entity through the stategraph.
  struct StategraphComponent : Component {
    explicit StategraphComponent(Entity entity) : Component(entity) {}

    /// The stategraph associated with the Entity.
    std::shared_ptr<StategraphAsset> stategraph;

    /// The script context in which to run signals in the stategraph.
    std::unique_ptr<ScriptEnv> env;

    /// The current state in the stategraph.
    HashValue current_state = 0;

    /// The track within the current state being played.
    const StategraphTrack* track = nullptr;

    /// The current time within the track playback.
    Clock::duration time = Clock::duration(0);

    /// The current path within the stategraph that gets us to the desired
    /// state.
    Stategraph::Path path;

    /// The arguments that will be used to select the next track.
    VariantMap selection_args;
  };

  // Sets the target state for the Stategraph |component|.
  void UpdatePathToTargetState(StategraphComponent* component, HashValue state);

  // Updates the Stategraph |component| playback by the specified |delta_time|.
  void AdvanceFrame(StategraphComponent* component, Clock::duration delta_time);

  // Updates the Stategraph |component| by exiting the current state and
  // entering the specified |state| at either the target signal or target
  // timestamp.
  void EnterState(StategraphComponent* component, HashValue state,
                  HashValue signal, Clock::duration timestamp,
                  Clock::duration blend_time);

  // Returns the next transition for the component.
  const StategraphTransition* GetNextTransition(
      const StategraphComponent* component) const;

  // Returns the state at the end of the path or the current state if there is
  // no path.
  HashValue GetTargetState(const StategraphComponent* component) const;

  /// Returns the StategraphAsset for the given filename.
  std::shared_ptr<StategraphAsset> LoadStategraph(string_view filename);

  /// The active pool of StategraphComponents being managed by this System.
  ComponentPool<StategraphComponent> components_;

  /// The stategraph objects loaded off disk.
  ResourceManager<StategraphAsset> assets_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StategraphSystem);

#endif  // LULLABY_SYSTEMS_STATEGRAPH_STATEGRAPH_SYSTEM_H_
