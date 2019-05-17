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

#include "lullaby/modules/animation_channels/skeleton_channel.h"

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/blend_shape/blend_shape_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"

namespace lull {

const HashValue SkeletonChannel::kChannelName = ConstHash("skeleton");

SkeletonChannel::SkeletonChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, AnimationChannel::kDynamicDimensions,
                       pool_size) {
  animation_system_ = registry->Get<AnimationSystem>();
  blend_shape_system_ = registry->Get<BlendShapeSystem>();
  transform_system_ = registry->Get<TransformSystem>();
}

void SkeletonChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* transform_system = registry->Get<AnimationSystem>();
  if (animation_system && transform_system) {
    AnimationChannelPtr ptr(new SkeletonChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup SkeletonChannel.";
  }
}

SkeletonChannel::AnimationContext::AnimationContext(size_t num_targets) {
  targets.reserve(num_targets);
}

void SkeletonChannel::AnimationContext::CreateTarget(
    uint32_t skeleton_index, bool has_translation, bool has_rotation,
    bool has_scale, size_t blend_shape_count) {
  targets.emplace_back(skeleton_index);
  auto& target = targets.back();
  if (has_translation) {
    target.SetTranslation();
    expected_value_count += 3;
  }
  if (has_rotation) {
    target.SetRotation();
    expected_value_count += 4;
  }
  if (has_scale) {
    target.SetScale();
    expected_value_count += 3;
  }
  if (blend_shape_count > 0) {
    target.blend_shape_count = blend_shape_count;
    expected_value_count += static_cast<size_t>(blend_shape_count);
  }
}

bool SkeletonChannel::Get(Entity entity, float* values, size_t len,
                          const void* context) const {
  auto skeleton = animation_system_->GetSkeleton(entity);
  if (skeleton.empty()) {
    LOG(ERROR) << "Entity has no skeleton.";
    return false;
  }

  const AnimationContext* skeleton_context =
      reinterpret_cast<const AnimationContext*>(context);
  if (len != skeleton_context->expected_value_count) {
    LOG(ERROR) << "Value count does not match expectation.";
    return false;
  }

  // Produce values for the array in the following order: translation, rotation,
  // scale. Each value produce increases the offset for the next property.
  size_t offset = 0;
  for (const auto& target : skeleton_context->targets) {
    const Entity entity = skeleton[target.skeleton_index];
    if (target.HasTransform()) {
      const Sqt* sqt = transform_system_->GetSqt(entity);
      const Sqt valid_sqt = sqt != nullptr ? *sqt : Sqt();
      if (target.HasTranslation()) {
        memcpy(values + offset, &valid_sqt.translation[0], sizeof(float) * 3);
        offset += 3;
      }
      if (target.HasRotation()) {
        const mathfu::vec3& vector = valid_sqt.rotation.vector();
        const mathfu::vec4 packed(valid_sqt.rotation.scalar(), vector[0],
                                  vector[1], vector[2]);
        memcpy(values + offset, &packed[0], sizeof(float) * 4);
        offset += 4;
      }
      if (target.HasScale()) {
        memcpy(values + offset, &valid_sqt.scale[0], sizeof(float) * 3);
        offset += 3;
      }
    }
    if (target.blend_shape_count > 0) {
      const size_t count = static_cast<size_t>(target.blend_shape_count);
      if (blend_shape_system_) {
        Span<float> weights = blend_shape_system_->GetWeights(entity);
        for (size_t i = 0; i < count; ++i) {
          values[offset + i] = i < weights.size() ? weights[i] : 0.f;
        }
      } else {
        LOG(ERROR) << "Animation includes blend weights, but BlendShapeSystem "
                      "is missing. Defaulting weights to 0.";
        for (size_t i = 0; i < count; ++i) {
          values[offset + i] = 0.f;
        }
      }
      offset += count;
    }
  }
  return true;
}

void SkeletonChannel::Set(Entity entity, const float* values, size_t len) {
  LOG(DFATAL) << "Context is required for the skeleton channel.";
}

void SkeletonChannel::Set(Entity entity, const float* values, size_t len,
                          const void* context) {
  auto skeleton = animation_system_->GetSkeleton(entity);
  if (skeleton.empty()) {
    LOG(ERROR) << "Entity has no skeleton.";
    return;
  }

  const AnimationContext* skeleton_context =
      reinterpret_cast<const AnimationContext*>(context);
  if (len != skeleton_context->expected_value_count) {
    LOG(ERROR) << "Value count does not match expectation.";
    return;
  }

  // Scan through the value array and consume values in the following order:
  // translation, rotation, scale. Consuming values increases the offset into
  // the value array for the next property.
  size_t offset = 0;
  for (const auto& target : skeleton_context->targets) {
    const Entity entity = skeleton[target.skeleton_index];
    if (target.HasTransform()) {
      const Sqt* sqt = transform_system_->GetSqt(entity);
      Sqt new_sqt = sqt != nullptr ? *sqt : Sqt();
      if (target.HasTranslation()) {
        new_sqt.translation = mathfu::vec3(values + offset);
        offset += 3;
      }
      if (target.HasRotation()) {
        const float* quat = values + offset;
        new_sqt.rotation =
            mathfu::quat(quat[0], quat[1], quat[2], quat[3]).Normalized();
        offset += 4;
      }
      if (target.HasScale()) {
        new_sqt.scale = mathfu::vec3(values + offset);
        offset += 3;
      }
      transform_system_->SetSqt(entity, new_sqt);
    }
    if (target.blend_shape_count > 0) {
      const size_t count = static_cast<size_t>(target.blend_shape_count);
      if (blend_shape_system_) {
        Span<float> weights(values + offset, count);
        blend_shape_system_->UpdateWeights(entity, weights);
      } else {
        LOG(ERROR) << "Animation includes blend weights, but BlendShapeSystem "
                      "is missing. No weights changed.";
      }
      offset += count;
    }
  }
}

}  // namespace lull
