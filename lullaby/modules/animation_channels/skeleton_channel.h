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

#ifndef LULLABY_MODULES_ANIMATION_CHANNELS_SKELETON_CHANNEL_H_
#define LULLABY_MODULES_ANIMATION_CHANNELS_SKELETON_CHANNEL_H_

#include <vector>

#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/util/registry.h"

namespace lull {

class AnimationSystem;
class BlendShapeSystem;
class TransformSystem;

/// Animates a group of Entities that represent a single Entity's
/// AnimationSystem::Skeleton.
///
/// This channel can be used to drive the properties of a group of Entities by
/// animating a single Entity that represents the group as a "skeleton". Each
/// sub-Entity has its own "target" identified by the index of that sub-Entity
/// in the owning Entity's skeleton.
///
/// Animations on this channel must have an associated animation context that
/// describes how to interpret incoming data. The channel interprets data by
/// scanning through it and applying the following rules:
/// - For each AnimationContext::Target:
///   - If it has a translation flag, consume the next 3 floats and set as this
///     Target Entity's translation. Components are assumed in order XYZ.
///   - If it has a rotation flag, consume the next 4 floats and set as this
///     Target Entity's rotation. Components are assumed in order WXYZ.
///   - If it has a scale flag, consume the next 3 floats and set a.s this
///     Target Entity's scale. Components are assumed in order XYZ
///   - If it has a non-zero blend shape count, consume the specified number of
///     floats and set as this Target Entity's blend weights.
class SkeletonChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  SkeletonChannel(Registry* registry, size_t pool_size);

  bool UsesAnimationContext() const override { return true; }

  /// Required to process incoming arrays of values.
  class AnimationContext {
   public:
    explicit AnimationContext(size_t num_targets);

    /// Allocates a new target for the animation associated with this context.
    /// |skeleton_index| is the index into the targeted Entity's
    /// AnimationSystem::Skeleton to apply properties to. The remaining flags
    /// indicate which properties to modify in calls to SkeletonChannel::Set().
    void CreateTarget(uint32_t skeleton_index, bool has_translation,
                      bool has_rotation, bool has_scale,
                      size_t blend_shape_count);

   private:
    friend class SkeletonChannel;

    /// Identifies an Entity by its index in the owning Entity's skeleton and
    /// indicates which properties to animate.
    struct Target {
      uint32_t skeleton_index;

      /// Flags indicating which transform-related properties should be set for
      /// the target Entity.
      uint8_t transform_properties = 0;
      bool HasTransform() const { return transform_properties != 0; }
      void SetTranslation() { transform_properties |= 0x1; }
      bool HasTranslation() const { return transform_properties & 0x1; }
      void SetRotation() { transform_properties |= 0x2; }
      bool HasRotation() const { return transform_properties & 0x2; }
      void SetScale() { transform_properties |= 0x4; }
      bool HasScale() const { return transform_properties & 0x4; }

      /// The number of values to consume for blend weights.
      size_t blend_shape_count = 0;

      explicit Target(uint32_t skeleton_index)
          : skeleton_index(skeleton_index) {}
    };
    std::vector<Target> targets;
    size_t expected_value_count = 0;
  };

 private:
  bool Get(Entity entity, float* values, size_t len,
           const void* context) const override;
  void Set(Entity entity, const float* values, size_t len) override;
  void Set(Entity entity, const float* values, size_t len,
           const void* context) override;
  AnimationSystem* animation_system_;
  BlendShapeSystem* blend_shape_system_;
  TransformSystem* transform_system_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_ANIMATION_CHANNELS_SKELETON_CHANNEL_H_
