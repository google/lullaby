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

#ifndef LULLABY_SYSTEMS_STATEGRAPH_STATEGRAPH_ASSET_H_
#define LULLABY_SYSTEMS_STATEGRAPH_STATEGRAPH_ASSET_H_

#include <memory>
#include "lullaby/generated/animation_stategraph_generated.h"
#include "lullaby/events/animation_events.h"
#include "lullaby/modules/ecs/entity.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/stategraph/stategraph.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/selector.h"

namespace lull {

/// Creates a Stategraph instance from an AnimationStategraphDef and provides
/// animation-specific operations on top of the underlying Stategraph.
class StategraphAsset : public Asset {
 public:
  explicit StategraphAsset(Registry* registry) : registry_(registry) {}

  /// Creates the stategraph from the specified AnimationStategraphDef data.
  void OnFinalize(const std::string& filename, std::string* data) override;

  /// Returns the path in the stategraph between the two states.
  Stategraph::Path FindPath(HashValue from_state, HashValue to_state) const;

  /// Returns the default (0-th) transition out of the given state.
  const StategraphTransition* GetDefaultTransition(HashValue state) const;

  /// Returns a track from state based on the data in the selection args.
  const StategraphTrack* SelectTrack(HashValue state,
                                     const VariantMap& args) const;

  /// Adjusts the time using the playback speed from the track.
  Clock::duration AdjustTime(Clock::duration time,
                             const StategraphTrack* track) const;

  /// Plays the animation associated with the track starting at the timestamp.
  AnimationId PlayTrack(Entity entity, const StategraphTrack* track,
                        Clock::duration timestamp,
                        Clock::duration blend_time) const;

  /// Returns the target signal in the transition if the transition is valid
  /// for the given track at the specified timestamp.
  Optional<HashValue> IsTransitionValid(const StategraphTransition& transition,
                                        const StategraphTrack* track,
                                        Clock::duration timestamp) const;

  StategraphAsset(const StategraphAsset&) = delete;
  StategraphAsset& operator=(const StategraphAsset&) = delete;

 private:
  using TrackSelector = Selector<std::unique_ptr<StategraphTrack>>;

  // Creates the appropriate Stategraph objects from flatbuffers.
  StategraphTransition CreateTransition(
      HashValue from_state, const AnimationTransitionDef* def) const;
  std::unique_ptr<StategraphSignal> CreateSignal(
      const AnimationSignalDef* def) const;
  std::unique_ptr<StategraphTrack> CreateTrack(
      const AnimationTrackDef* def) const;
  std::unique_ptr<StategraphState> CreateState(
      const AnimationStateDef* def) const;
  std::unique_ptr<TrackSelector> CreateSelector(AnimationSelectorDef type,
                                                const void* def) const;

  Registry* registry_;
  std::unique_ptr<Stategraph> stategraph_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_STATEGRAPH_STATEGRAPH_ASSET_H_
