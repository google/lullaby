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

#ifndef LULLABY_SYSTEMS_ANIMATION_ANIMATION_SYSTEM_H_
#define LULLABY_SYSTEMS_ANIMATION_ANIMATION_SYSTEM_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "lullaby/generated/animation_def_generated.h"
#include "lullaby/generated/animation_response_def_generated.h"
#include "lullaby/events/animation_events.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/animation/animation_asset.h"
#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/systems/animation/playback_parameters.h"
#include "lullaby/systems/animation/spline_modifiers.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/resource_manager.h"
#include "motive/common.h"
#include "motive/engine.h"

namespace lull {

// Uses the motive library to play animations on Entities.
//
// An animation is defined as a set of 2D curves such that, for a single curve
// there is a single y-value for any given x-value (or time value).  Animations
// are played back by advancing the x-value by the given timestep, reading the
// associated y-values from the curves, and applying those values to Entities
// (e.g. setting the TransformSystem SQT).
//
// Animation Channels are used to map curve data to component data.  Animation
// Channel instances must be registered with the AnimationSystem by calling
// |AddChannel|.  A common set of AnimationChannels are available for use (e.g.
// PositionChannel, which reads data from a 3-curve animation and sets the
// corresponding data in the TransformSystem), but must still be explicitly
// registered.  Clients are also free to implement their own channels as needed.
//
// The AnimationSystem uses the motive library (//third_party/motive) to
// process and evaluate animation curves.  Animation data is stored in
// flatbuffers as either .motiveanim files (which are converted from FBX files
// using motive's anim_pipeline) or .splienanim files (which are converted from
// JSON files using flatbuffers flatc compiler.)  Alternatively, animations
// can be driven towards arbitrary target values.  This is done by generating
// the appropriate curves at runtime.
class AnimationSystem : public System {
 public:
  explicit AnimationSystem(Registry* registry);

  ~AnimationSystem() override;

  // Sets up animation responses specified in the Def on the Entity.
  void Create(Entity entity, HashValue type, const Def* def) override;

  // Plays the animation curves specified in the Def on the Entity.
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;

  // Removes any animation data related to the Entity.
  void Destroy(Entity entity) override;

  // Stops all animations playing on the Entity.
  void CancelAllAnimations(Entity entity);

  // Registers an animation channel which determines how animation data will be
  // set on external Systems.  See AnimationChannel for more details.
  void AddChannel(HashValue id, AnimationChannelPtr channel);

  // Advances all animations by the specified |delta_time| and updates all
  // AnimationChannels, pushing the updated animation data to their
  // corresponding Systems.
  void AdvanceFrame(Clock::duration delta_time);

  // Plays the animation curves specified in the Def on the Entity.  Returns a
  // unique AnimationId, which will be included in the AnimationCompleteEvent
  // dispatched when this animation finishes or is interrupted.
  AnimationId PlayAnimation(Entity e, const AnimationDef* data);

  // Plays the specified animation on the channel with the given params. Returns
  // a unique AnimationId, which will be included in the AnimationCompleteEvent
  // dispatched when this animation finishes or is interrupted.
  AnimationId PlayAnimation(Entity e, HashValue channel,
                            const AnimationAssetPtr& anim,
                            const PlaybackParameters& params);

  // Drives the data specified by the |channel| towards the |target| values
  // over the given |time|, after |delay|. Returns a unique AnimationId, which
  // will be included in the AnimationCompleteEvent dispatched when this
  // animation finishes or is interrupted.
  AnimationId SetTarget(Entity e, HashValue channel, const float* target,
                        size_t len, Clock::duration time,
                        Clock::duration delay = Clock::duration::zero());

  // Cancels the current animation for |e| in |channel|.
  void CancelAnimation(Entity e, HashValue channel);

  bool IsAnimationPlaying(AnimationId id) const;

  // Returns the remaining time for the current animation on the
  // specified channel.  Returns 0 if there is no animation playing or
  // if the animation is complete.  Returns motive::kMotiveTimeEndless
  // if the animation is looping.
  motive::MotiveTime TimeRemaining(Entity entity, HashValue channel);

  // Returns the currently playing RigAnim of the specified channel of
  // the entity, or nullptr if not a rig channel or if not animation
  // is playing.
  const motive::RigAnim* CurrentRigAnim(Entity entity, HashValue channel);

  // Sets the rate on an active animation on |entity|'s |channel|.
  // |rate| multiplies the animation's natural timestep.
  void SetPlaybackRate(Entity entity, HashValue channel, float speed);

  // Loads and returns the animation asset associated with the filename. The
  // asset is loaded synchronously and will remain in the AnimationSystem's
  // internal cache indefinitely.
  AnimationAssetPtr LoadAnimation(const std::string& filename);

  // Converts MotiveTime |time| units to Clock::duration units.
  static Clock::duration GetDurationFromMotiveTime(motive::MotiveTime time);

  // Converts Clock::duration |timestamp| to MotiveTime units.
  static motive::MotiveTime GetMotiveTimeFromDuration(Clock::duration timestep);

  // Converts |seconds| to MotiveTime units.
  static motive::MotiveTime GetMotiveTimeFromSeconds(float seconds);

  // The smallest timestep by which the AnimationSystem can be advanced.
  static motive::MotiveTime GetMinimalStep();

  // Converts |seconds| to MotiveTime units.
  static float GetMotiveDerivativeFromSeconds(float seconds);

  // Splits an animation list filename and its index.  Indices are specified at
  // the end of the filename, delimited by ':', eg "foo.motivelist:1".
  // If |filename| is not a recognized list file or has no index, it is not
  // modified and index is untouched.
  static void SplitListFilenameAndIndex(std::string* filename, int* index);

 private:
  // A set of AnimationIds.
  using AnimationSet = std::unordered_set<AnimationId>;

  // Associates an Entity with an AnimationSet and an array of events to trigger
  // when the animations in the AnimationSet have completed.
  struct AnimationSetEntry {
    Entity entity = kNullEntity;
    AnimationSet animations;
    const AnimationDef* data = nullptr;
  };

  static PlaybackParameters GetPlaybackParameters(const AnimInstanceDef* anim);
  static SplineModifiers GetSplineModifiers(const AnimInstanceDef* anim);

  AnimationChannel* FindChannel(string_view channel_name);
  AnimationChannel* FindChannel(HashValue channel_id);

  AnimationId GenerateAnimationId();
  AnimationId TrackAnimations(Entity e, AnimationSet anims,
                              const AnimationDef* data);
  void UntrackAnimation(AnimationId internal_id,
                        AnimationCompletionReason status);

  // Initializes the |channel| with the defining animation for the Entity |e|
  // if one has been specified. This function should be called before playing
  // any rig animations for an Entity on the specified channel.
  void PrepareDefiningAnimation(Entity e, AnimationChannel* channel);

  AnimationId PlayAnimation(Entity e, const AnimTargetDef* target);
  AnimationId PlayAnimation(Entity e, const AnimInstanceDef* anim);
  AnimationId PlayRigAnimation(Entity e, AnimationChannel* channel,
                               const AnimInstanceDef* anim);
  AnimationId PlaySplineAnimation(Entity e, AnimationChannel* channel,
                                  const AnimInstanceDef* anim);
  AnimationId PlayRigAnimationInternal(Entity e, AnimationChannel* channel,
                                       const AnimationAssetPtr& anim,
                                       const PlaybackParameters& params,
                                       int rig_index = 0);
  AnimationId SetTargetInternal(Entity e, AnimationChannel* channel,
                                const float* data, size_t len,
                                Clock::duration time, Clock::duration delay);

  struct DefiningAnimation {
    AnimationAssetPtr asset;
    AnimationChannel* channel = nullptr;
  };

  AnimationId current_id_;
  motive::MotiveEngine engine_;
  ResourceManager<AnimationAsset> assets_;
  std::unordered_map<Entity, DefiningAnimation> defining_animations_;
  std::unordered_map<HashValue, AnimationChannelPtr> channels_;
  std::unordered_map<AnimationId, AnimationSetEntry> external_id_to_entry_;
  std::unordered_map<AnimationId, AnimationId> internal_to_external_ids_;

  AnimationSystem(const AnimationSystem&) = delete;
  AnimationSystem& operator=(const AnimationSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AnimationSystem);

#endif  // LULLABY_SYSTEMS_ANIMATION_ANIMATION_SYSTEM_H_
