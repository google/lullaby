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

#ifndef LULLABY_SYSTEMS_ANIMATION_ANIMATION_CHANNEL_H_
#define LULLABY_SYSTEMS_ANIMATION_ANIMATION_CHANNEL_H_

#include <memory>
#include "lullaby/events/animation_events.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/systems/animation/playback_parameters.h"
#include "lullaby/systems/animation/spline_modifiers.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/registry.h"
#include "motive/init.h"
#include "motive/motivator.h"
#include "motive/util.h"

namespace lull {

// Responsible for mapping data between Components and a motive::Motivator.
//
// Each AnimationChannel stores a set of Animation objects which associate
// Motivators with Entities.  Each frame, the current Motivator values are
// passed to the AnimationChannel's virtual |Set| function which is overridden
// such that the data is passed to the correct Component for the associated
// Entity.
class AnimationChannel {
 public:
  static const size_t kMaxDimensions = 6;

  AnimationChannel(Registry* registry, int num_dimensions, size_t pool_size);
  virtual ~AnimationChannel() {}

  // Specifies the number of elements being animated by the motivator.  (For
  // example a 3D position animation has 3 dimensions: x, y, and z.)
  size_t GetDimensions() const { return dimensions_; }

  // Initializes the underlying motive "motivators" to play animations as
  // defined by the |init_data|.
  void Init(Entity entity, motive::MotiveEngine* engine, const void* init_data);

  // Copies all the data from the Motivator into the Component.  Updates the
  // |completed| vector with information about Animations that have completed.
  void Update(std::vector<AnimationId>* completed);

  // Plays a new animation (with the given |id|) on the |entity|.  The animation
  // sets the motivator to animate towards the specified |target_value| array
  // (of size |length|) over the given |time| duration, after a |delay|.
  // Returns the AnimationId of the previously running animation, or
  // kNullAnimation if no animation was active.
  AnimationId Play(Entity e, motive::MotiveEngine* engine, AnimationId id,
                   const float* target_value, size_t length,
                   Clock::duration time, Clock::duration delay);

  // Plays a new animation (with the given |id|) on the |entity|.  The animation
  // to be played is defined by the array of |splines| and |constants|, both
  // of which are of size |length|.  For each dimension, the associated spline
  // is used, unless it is NULL in which case the associated constant value is
  // set.  |params| and |modifiers| can be specified to provide extra control of
  // how the animation is played.  Returns the AnimationId of the previously
  // running animation, or kNullAnimation if no animation was active.
  AnimationId Play(Entity e, motive::MotiveEngine* engine, AnimationId id,
                   const motive::CompactSpline* const* splines,
                   const float* constants, size_t length,
                   const PlaybackParameters& params,
                   const SplineModifiers& modifiers);

  // Plays a new animation (with the given |id|) on the |entity|.  The animation
  // is specified by the |rig_anim|.  |params| can be specified to provide extra
  // control of how the animation is played.  Returns the AnimationId of the
  // previously running animation, or kNullAnimation if no animation was active.
  AnimationId Play(Entity e, motive::MotiveEngine* engine, AnimationId id,
                   const motive::RigAnim* rig_anim,
                   const PlaybackParameters& params);

  // Stops animation playback on this channel for the specified |entity| and
  // returns its AnimationId, or kNullAnimation if no animation was active.
  AnimationId Cancel(Entity entity);

  // Sets the rate on an active animation on |entity|'s |channel|.
  // |rate| multiplies the animation's natural timestep.
  void SetPlaybackRate(Entity entity, float rate);

  // Returns true if the channel uses the rig_motivator.
  virtual bool IsRigChannel() const { return false; }

  // Gets the array of operations (e.g. scale-x, rotate-z, translate-y, etc.)
  // to pull from the underlying animation, or NULL if no operations supported.
  // The length of the array will equal the value of GetDimensions().
  virtual const motive::MatrixOperationType* GetOperations() const {
    return nullptr;
  }

  // Returns the remaining time for the current animation.  Returns 0 if there
  // is no animation playing or if the animation is complete.  Returns
  // motive::kMotiveTimeEndless if the animation is looping.
  motive::MotiveTime TimeRemaining(lull::Entity entity) const;

  // Returns the currently playing RigAnim of the entity, or nullptr
  // if not a rig channel or if not animation is playing.
  const motive::RigAnim* CurrentRigAnim(lull::Entity entity) const;

 protected:
  // Associates a motivator with an Entity.
  struct Animation : Component {
    explicit Animation(Entity entity) : Component(entity) {}

    motive::MotivatorNf motivator;
    motive::RigMotivator rig_motivator;
    float base_offset[kMaxDimensions] = {0.f};
    float multiplier[kMaxDimensions] = {0.f};
    motive::MotiveTime total_time = 0;
    AnimationId id = kNullAnimation;
  };

  // Updates the |anim| with the new |id|, returning the previously set
  // AnimationId.
  AnimationId UpdateId(Animation* anim, AnimationId id);

  // Determines whether or not the specified |anim| is complete.
  bool IsComplete(const Animation& anim) const;

  // Prepares an Animation to be played on the |entity| using the provided
  // motive |engine| and |init_data|.  Returns the instantiated Animation
  // for that |entity|, or the previously instantiated Animation for the
  // |entity|.
  virtual Animation* DoInitialize(Entity entity, motive::MotiveEngine* engine,
                                  const void* init_data);

  // Gets Component data for this channel (an array of floats) associated with
  // the Entity.
  virtual bool Get(Entity entity, float* values, size_t len) const;

  // Sets Component data for this channel (an array of floats) associated with
  // the Entity.
  virtual void Set(Entity entity, const float* values, size_t len) = 0;

  // Sets the rig data associated with the Entity.  Valid only for rig channels.
  virtual void SetRig(Entity entity, const mathfu::AffineTransform* values,
                      size_t len);

  Registry* registry_;
  ComponentPool<Animation> anims_;
  int dimensions_;
};

using AnimationChannelPtr = std::unique_ptr<AnimationChannel>;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_ANIMATION_ANIMATION_CHANNEL_H_
