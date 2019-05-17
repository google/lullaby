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

#ifndef LULLABY_MODULES_ANIMATION_CHANNELS_TRANSFORM_CHANNELS_H_
#define LULLABY_MODULES_ANIMATION_CHANNELS_TRANSFORM_CHANNELS_H_

#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/util/registry.h"

namespace lull {

class TransformSystem;

// Channel for animating Transform position.
class PositionChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  PositionChannel(Registry* registry, size_t pool_size);

  const motive::MatrixOperationType* GetOperations() const override;

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

// Channel for animating only the x-position of an entity.
class PositionXChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  PositionXChannel(Registry* registry, size_t pool_size);

  const motive::MatrixOperationType* GetOperations() const override;

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

// Channel for animating only the y-position of an entity.
class PositionYChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  PositionYChannel(Registry* registry, size_t pool_size);

  const motive::MatrixOperationType* GetOperations() const override;

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

// Channel for animating only the z-position of an entity.
class PositionZChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  PositionZChannel(Registry* registry, size_t pool_size);

  const motive::MatrixOperationType* GetOperations() const override;

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

// Channel for animating Transform rotation.
class RotationChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  RotationChannel(Registry* registry, size_t pool_size);

  const motive::MatrixOperationType* GetOperations() const override;

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

// Channel for animating Transform scale.
class ScaleChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  ScaleChannel(Registry* registry, size_t pool_size);

  const motive::MatrixOperationType* GetOperations() const override;

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

// Channel for animating Transform scale from an fbx.
class ScaleFromRigChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  ScaleFromRigChannel(Registry* registry, size_t pool_size);

  bool IsRigChannel() const override { return true; }

  const motive::MatrixOperationType* GetOperations() const override;

  static void Setup(Registry* registry, size_t pool_size);

 private:
  void Set(Entity e, const float* values, size_t len) override;
  void SetRig(Entity entity, const mathfu::AffineTransform* values,
              size_t len) override;
  TransformSystem* transform_system_;
};

class AabbMinChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  AabbMinChannel(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

class AabbMaxChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  AabbMaxChannel(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;
  TransformSystem* transform_system_;
};

struct AnimatePositionEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&position, ConstHash("position"));
    archive(&time_ms, ConstHash("time_ms"));
  }

  Entity entity = kNullEntity;
  mathfu::vec3 position = mathfu::kZeros3f;
  float time_ms = 0.f;
};

struct AnimateRotationEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&rotation, ConstHash("rotation"));
    archive(&time_ms, ConstHash("time_ms"));
  }

  Entity entity = kNullEntity;
  mathfu::quat rotation = mathfu::quat::identity;
  float time_ms = 0.f;
};

struct AnimateScaleEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&scale, ConstHash("scale"));
    archive(&time_ms, ConstHash("time_ms"));
  }

  Entity entity = kNullEntity;
  mathfu::vec3 scale = mathfu::kZeros3f;
  float time_ms = 0.f;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AnimatePositionEvent);
LULLABY_SETUP_TYPEID(lull::AnimateRotationEvent);
LULLABY_SETUP_TYPEID(lull::AnimateScaleEvent);

#endif  // LULLABY_MODULES_ANIMATION_CHANNELS_TRANSFORM_CHANNELS_H_
