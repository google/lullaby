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

#include "lullaby/modules/animation_channels/transform_channels.h"

#include "mathfu/constants.h"
#include "motive/init.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"

namespace lull {

const HashValue PositionChannel::kChannelName = Hash("transform-position");
const HashValue PositionXChannel::kChannelName = Hash("transform-position-x");
const HashValue PositionZChannel::kChannelName = Hash("transform-position-z");
const HashValue RotationChannel::kChannelName = Hash("transform-rotation");
const HashValue ScaleChannel::kChannelName = Hash("transform-scale");
const HashValue ScaleFromRigChannel::kChannelName = Hash("transform-scale-rig");
const HashValue AabbMinChannel::kChannelName = Hash("transform-aabb-min");
const HashValue AabbMaxChannel::kChannelName = Hash("transform-aabb-max");

namespace {

const motive::MatrixOperationType kTranslateOps[] = {
    motive::kTranslateX, motive::kTranslateY, motive::kTranslateZ};
const motive::MatrixOperationType kTranslateXOps[] = {motive::kTranslateX};
const motive::MatrixOperationType kTranslateZOps[] = {motive::kTranslateZ};
const motive::MatrixOperationType kRotateOps[] = {
    motive::kRotateAboutX, motive::kRotateAboutY, motive::kRotateAboutZ};
const motive::MatrixOperationType kScaleOps[] = {
    motive::kScaleX, motive::kScaleY, motive::kScaleZ};

}  // namespace

PositionChannel::PositionChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3, pool_size),
      transform_system_(registry->Get<TransformSystem>()) {}

void PositionChannel::Setup(Registry* registry, size_t pool_size) {
  auto animation_system = registry->Get<AnimationSystem>();
  auto transform_system = registry->Get<TransformSystem>();
  if (animation_system == nullptr || transform_system == nullptr) {
    LOG(DFATAL) << "Failed to setup PositionChannel.";
  }
  AnimationChannelPtr ptr(new PositionChannel(registry, pool_size));
  animation_system->AddChannel(kChannelName, std::move(ptr));
}

const motive::MatrixOperationType* PositionChannel::GetOperations() const {
  return kTranslateOps;
}

bool PositionChannel::Get(Entity e, float* values, size_t len) const {
  const Sqt* sqt = transform_system_->GetSqt(e);
  if (sqt == nullptr) {
    return false;
  }
  values[0] = sqt->translation.x;
  values[1] = sqt->translation.y;
  values[2] = sqt->translation.z;
  return true;
}

void PositionChannel::Set(Entity e, const float* values, size_t len) {
  const Sqt* sqt = transform_system_->GetSqt(e);
  if (sqt == nullptr) {
    return;
  }
  Sqt updated_sqt = *sqt;
  updated_sqt.translation.x = values[0];
  updated_sqt.translation.y = values[1];
  updated_sqt.translation.z = values[2];
  transform_system_->SetSqt(e, updated_sqt);
}

PositionXChannel::PositionXChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 1 /* num_dimensions */, pool_size),
      transform_system_(registry->Get<TransformSystem>()) {}

void PositionXChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* transform_system = registry->Get<TransformSystem>();
  if (animation_system == nullptr || transform_system == nullptr) {
    LOG(DFATAL) << "Failed to setup PositionXChannel.";
  }
  AnimationChannelPtr ptr(new PositionXChannel(registry, pool_size));
  animation_system->AddChannel(kChannelName, std::move(ptr));
}

const motive::MatrixOperationType* PositionXChannel::GetOperations() const {
  return kTranslateZOps;
}

bool PositionXChannel::Get(Entity entity, float* values, size_t len) const {
  const Sqt* sqt = transform_system_->GetSqt(entity);
  if (sqt == nullptr) {
    return false;
  }
  values[0] = sqt->translation.x;
  return true;
}

void PositionXChannel::Set(Entity entity, const float* values, size_t len) {
  const Sqt* sqt = transform_system_->GetSqt(entity);
  if (sqt == nullptr) {
    return;
  }
  Sqt updated_sqt = *sqt;
  updated_sqt.translation.x = values[0];
  transform_system_->SetSqt(entity, updated_sqt);
}

PositionZChannel::PositionZChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 1 /* num_dimensions */, pool_size),
      transform_system_(registry->Get<TransformSystem>()) {}

void PositionZChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* transform_system = registry->Get<TransformSystem>();
  if (animation_system == nullptr || transform_system == nullptr) {
    LOG(DFATAL) << "Failed to setup PositionZChannel.";
  }
  AnimationChannelPtr ptr(new PositionZChannel(registry, pool_size));
  animation_system->AddChannel(kChannelName, std::move(ptr));
}

const motive::MatrixOperationType* PositionZChannel::GetOperations() const {
  return kTranslateZOps;
}

bool PositionZChannel::Get(Entity entity, float* values, size_t len) const {
  const Sqt* sqt = transform_system_->GetSqt(entity);
  if (sqt == nullptr) {
    return false;
  }
  values[0] = sqt->translation.z;
  return true;
}

void PositionZChannel::Set(Entity entity, const float* values, size_t len) {
  const Sqt* sqt = transform_system_->GetSqt(entity);
  if (sqt == nullptr) {
    return;
  }
  Sqt updated_sqt = *sqt;
  updated_sqt.translation.z = values[0];
  transform_system_->SetSqt(entity, updated_sqt);
}

RotationChannel::RotationChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3, pool_size) {
  transform_system_ = registry->Get<TransformSystem>();
}

void RotationChannel::Setup(Registry* registry, size_t pool_size) {
  auto animation_system = registry->Get<AnimationSystem>();
  auto transform_system = registry->Get<TransformSystem>();
  if (animation_system && transform_system) {
    AnimationChannelPtr ptr(new RotationChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup RotationChannel.";
  }
}

const motive::MatrixOperationType* RotationChannel::GetOperations() const {
  return kRotateOps;
}

bool RotationChannel::Get(Entity e, float* values, size_t len) const {
  const Sqt* sqt = transform_system_->GetSqt(e);
  if (sqt) {
    const mathfu::vec3 angles = sqt->rotation.ToEulerAngles();
    values[0] = angles.x;
    values[1] = angles.y;
    values[2] = angles.z;
    return true;
  }
  return false;
}

void RotationChannel::Set(Entity e, const float* values, size_t len) {
  const Sqt* sqt = transform_system_->GetSqt(e);
  if (sqt) {
    const mathfu::vec3 angles(values[0], values[1], values[2]);

    Sqt updated_sqt = *sqt;
    updated_sqt.rotation = mathfu::quat::FromEulerAngles(angles);
    transform_system_->SetSqt(e, updated_sqt);
  }
}

ScaleChannel::ScaleChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3, pool_size) {
  transform_system_ = registry->Get<TransformSystem>();
}

void ScaleChannel::Setup(Registry* registry, size_t pool_size) {
  auto animation_system = registry->Get<AnimationSystem>();
  auto transform_system = registry->Get<TransformSystem>();
  if (animation_system && transform_system) {
    AnimationChannelPtr ptr(new ScaleChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup ScaleChannel.";
  }
}

const motive::MatrixOperationType* ScaleChannel::GetOperations() const {
  return kScaleOps;
}

bool ScaleChannel::Get(Entity e, float* values, size_t len) const {
  const Sqt* sqt = transform_system_->GetSqt(e);
  if (sqt) {
    values[0] = sqt->scale.x;
    values[1] = sqt->scale.y;
    values[2] = sqt->scale.z;
    return true;
  }
  return false;
}

void ScaleChannel::Set(Entity e, const float* values, size_t len) {
  const Sqt* sqt = transform_system_->GetSqt(e);
  if (sqt) {
    Sqt updated_sqt = *sqt;
    updated_sqt.scale.x = values[0];
    updated_sqt.scale.y = values[1];
    updated_sqt.scale.z = values[2];
    transform_system_->SetSqt(e, updated_sqt);
  }
}

ScaleFromRigChannel::ScaleFromRigChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 0, pool_size) {
  transform_system_ = registry->Get<TransformSystem>();
}

void ScaleFromRigChannel::Setup(Registry* registry, size_t pool_size) {
  auto animation_system = registry->Get<AnimationSystem>();
  auto transform_system = registry->Get<TransformSystem>();
  if (animation_system && transform_system) {
    AnimationChannelPtr ptr(new ScaleFromRigChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup ScaleFromRigChannel.";
  }
}

const motive::MatrixOperationType* ScaleFromRigChannel::GetOperations() const {
  return kScaleOps;
}

void ScaleFromRigChannel::Set(Entity e, const float* values, size_t len) {
  LOG(DFATAL) << "SetRig should be called for rig channels.";
}

void ScaleFromRigChannel::SetRig(Entity entity,
                                 const mathfu::AffineTransform* values,
                                 size_t len) {
  if (len != 1) {
    LOG(DFATAL) << "Too many transforms; can only collect scale from one.";
    return;
  }

  const Sqt* old_sqt = transform_system_->GetSqt(entity);
  if (old_sqt == nullptr) {
    LOG(DFATAL) << "Entity does not have a SQT.";
    return;
  }

  Sqt sqt = CalculateSqtFromAffineTransform(values[0]);
  sqt.rotation = old_sqt->rotation;
  sqt.translation = old_sqt->translation;
  transform_system_->SetSqt(entity, sqt);
}

AabbMinChannel::AabbMinChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3 /* num_dimensions */, pool_size),
      transform_system_(registry->Get<TransformSystem>()) {}

void AabbMinChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* transform_system = registry->Get<TransformSystem>();
  if (animation_system && transform_system) {
    animation_system->AddChannel(
        kChannelName,
        AnimationChannelPtr(new AabbMinChannel(registry, pool_size)));
  } else {
    LOG(DFATAL) << "Failed to setup AabbMinChannel.";
  }
}

bool AabbMinChannel::Get(Entity entity, float* values, size_t len) const {
  const Aabb* aabb = transform_system_->GetAabb(entity);
  if (aabb) {
    values[0] = aabb->min.x;
    values[1] = aabb->min.y;
    values[2] = aabb->min.z;
    return true;
  }
  return false;
}

void AabbMinChannel::Set(Entity entity, const float* values, size_t len) {
  const Aabb* aabb = transform_system_->GetAabb(entity);
  if (aabb) {
    const mathfu::vec3 min(values[0], values[1], values[2]);
    transform_system_->SetAabb(entity, Aabb(min, aabb->max));
  }
}

AabbMaxChannel::AabbMaxChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3 /* num_dimensions */, pool_size),
      transform_system_(registry->Get<TransformSystem>()) {}

void AabbMaxChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* transform_system = registry->Get<TransformSystem>();
  if (animation_system && transform_system) {
    animation_system->AddChannel(
        kChannelName,
        AnimationChannelPtr(new AabbMaxChannel(registry, pool_size)));
  } else {
    LOG(DFATAL) << "Failed to setup AabbMaxChannel.";
  }
}

bool AabbMaxChannel::Get(Entity entity, float* values, size_t len) const {
  const Aabb* aabb = transform_system_->GetAabb(entity);
  if (aabb) {
    values[0] = aabb->max.x;
    values[1] = aabb->max.y;
    values[2] = aabb->max.z;
    return true;
  }
  return false;
}

void AabbMaxChannel::Set(Entity entity, const float* values, size_t len) {
  const Aabb* aabb = transform_system_->GetAabb(entity);
  if (aabb) {
    const mathfu::vec3 max(values[0], values[1], values[2]);
    transform_system_->SetAabb(entity, Aabb(aabb->min, max));
  }
}

}  // namespace lull
