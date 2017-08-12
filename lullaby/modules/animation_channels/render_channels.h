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

#ifndef LULLABY_MODULES_ANIMATION_CHANNELS_RENDER_CHANNELS_H_
#define LULLABY_MODULES_ANIMATION_CHANNELS_RENDER_CHANNELS_H_

#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/util/registry.h"
#include "motive/motivator.h"

namespace lull {

class RenderSystem;

// Channel for animating Render uniforms.
class UniformChannel : public AnimationChannel {
 public:
  static const HashValue kColorChannelName;

  UniformChannel(Registry* registry, size_t pool_size,
                 const std::string& uniform_name, int uniform_dimensions);

  static void Setup(Registry* registry, size_t pool_size, HashValue channel_id,
                    const std::string& uniform_name, int uniform_dimension);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  RenderSystem* render_system_;
  std::string uniform_name_;
};

// Channel for animating Render matrix palette for rigged skeletal animation.
class RigChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  RigChannel(Registry* registry, size_t pool_size);

  bool IsRigChannel() const override { return true; }

  static void Setup(Registry* registry, size_t pool_size);

 private:
  void Set(Entity e, const float* values, size_t len) override;
  void SetRig(Entity entity, const mathfu::AffineTransform* values,
              size_t len) override;

  RenderSystem* render_system_;
};

// Channel for animating only RGB and leaving alpha alone.
class RgbChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  RgbChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  RenderSystem* render_system_;
};

// Channel for animating alpha values.
class AlphaChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  AlphaChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  RenderSystem* render_system_;
};

// Channel for animating alpha values for an entity and its descendants.
class AlphaDescendantsChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  AlphaDescendantsChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  Registry* registry_;
  RenderSystem* render_system_;
};

// Channel for animating rgb values for an entity and its descendants.
class RgbMultiplierDescendantsChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  RgbMultiplierDescendantsChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  Registry* registry_;
  RenderSystem* render_system_;
};

// Channel for animating alpha values for an entity and its descendants.
class AlphaMultiplierDescendantsChannel : public AnimationChannel {
 public:
  static const HashValue kChannelName;

  AlphaMultiplierDescendantsChannel(Registry* registry, size_t pool_size);

  static void Setup(Registry* registry, size_t pool_size);

 private:
  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  Registry* registry_;
  RenderSystem* render_system_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_ANIMATION_CHANNELS_RENDER_CHANNELS_H_
