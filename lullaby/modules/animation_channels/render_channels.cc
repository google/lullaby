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

#include "lullaby/modules/animation_channels/render_channels.h"

#include "motive/init.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/render/render_helpers.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

const HashValue RigChannel::kChannelName = Hash("render-rig");
const HashValue UniformChannel::kColorChannelName = Hash("render-color");
const HashValue RgbChannel::kChannelName = Hash("render-color-rgb");
const HashValue AlphaChannel::kChannelName = Hash("render-color-alpha");
const HashValue AlphaDescendantsChannel::kChannelName =
    Hash("render-color-alpha-descendants");
const HashValue RgbMultiplierDescendantsChannel::kChannelName =
    Hash("render-color-rgb-multiplier-descendants");
const HashValue AlphaMultiplierDescendantsChannel::kChannelName =
    Hash("render-color-alpha-multiplier-descendants");

UniformChannel::UniformChannel(Registry* registry, size_t pool_size,
                               const std::string& uniform_name,
                               int uniform_dimensions)
    : AnimationChannel(registry, uniform_dimensions, pool_size),
      render_system_(registry->Get<RenderSystem>()),
      uniform_name_(uniform_name) {}

void UniformChannel::Setup(Registry* registry, size_t pool_size,
                           HashValue channel_id,
                           const std::string& uniform_name,
                           int uniform_dimension) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(new UniformChannel(
        registry, pool_size, uniform_name, uniform_dimension));
    animation_system->AddChannel(channel_id, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup UniformChannel.";
  }
}

bool UniformChannel::Get(Entity e, float* values, size_t len) const {
  return render_system_->GetUniform(e, uniform_name_.c_str(), len, values);
}

void UniformChannel::Set(Entity e, const float* values, size_t len) {
  render_system_->SetUniform(e, uniform_name_.c_str(), values,
                             static_cast<int>(len));
}

RigChannel::RigChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 0, pool_size) {
  render_system_ = registry->Get<RenderSystem>();
}

void RigChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(new RigChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup RigChannel.";
  }
}

void RigChannel::Set(Entity e, const float* values, size_t len) {
  LOG(DFATAL) << "SetRig should be called for rig channels.";
}

void RigChannel::SetRig(Entity entity, const mathfu::AffineTransform* values,
                        size_t len) {
  render_system_->SetBoneTransforms(entity, values, static_cast<int>(len));
}

RgbChannel::RgbChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3, pool_size),
      render_system_(registry->Get<RenderSystem>()) {}

void RgbChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(new RgbChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup RgbChannel.";
  }
}

bool RgbChannel::Get(Entity e, float* values, size_t len) const {
  mathfu::vec4 color;
  if (render_system_->GetColor(e, &color)) {
    values[0] = color[0];
    values[1] = color[1];
    values[2] = color[2];
    return true;
  }
  return false;
}

void RgbChannel::Set(Entity e, const float* values, size_t len) {
  mathfu::vec4 color;
  if (render_system_->GetColor(e, &color)) {
    color[0] = values[0];
    color[1] = values[1];
    color[2] = values[2];
    render_system_->SetColor(e, color);
  }
}

AlphaChannel::AlphaChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 1, pool_size),
      render_system_(registry->Get<RenderSystem>()) {}

void AlphaChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(new AlphaChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup AlphaChannel.";
  }
}

bool AlphaChannel::Get(Entity e, float* values, size_t len) const {
  mathfu::vec4 color;
  if (render_system_->GetColor(e, &color)) {
    values[0] = color[3];
    return true;
  }
  return false;
}

void AlphaChannel::Set(Entity e, const float* values, size_t len) {
  mathfu::vec4 color;
  if (render_system_->GetColor(e, &color)) {
    color[3] = values[0];
    render_system_->SetColor(e, color);
  }
}

AlphaDescendantsChannel::AlphaDescendantsChannel(Registry* registry,
                                                 size_t pool_size)
    : AnimationChannel(registry, 1, pool_size),
      registry_(registry),
      render_system_(registry->Get<RenderSystem>()) {}

void AlphaDescendantsChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(new AlphaDescendantsChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup AlphaDescendantsChannel.";
  }
}

bool AlphaDescendantsChannel::Get(Entity e, float* values, size_t len) const {
  mathfu::vec4 color;
  if (render_system_->GetColor(e, &color)) {
    values[0] = color[3];
    return true;
  }
  return false;
}

void AlphaDescendantsChannel::Set(Entity e, const float* values, size_t len) {
  auto fn = [this, values](Entity child) {
    mathfu::vec4 color = mathfu::kOnes4f;
    if (!render_system_->GetColor(child, &color)) {
      color = mathfu::kOnes4f;
    }
    color[3] = values[0];
    render_system_->SetColor(child, color);
  };

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->ForAllDescendants(e, fn);
}

RgbMultiplierDescendantsChannel::RgbMultiplierDescendantsChannel(
    Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 3, pool_size),
      registry_(registry),
      render_system_(registry->Get<RenderSystem>()) {}

void RgbMultiplierDescendantsChannel::Setup(Registry* registry,
                                            size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(
        new RgbMultiplierDescendantsChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup RgbMultiplierDescendantsChannel.";
  }
}

bool RgbMultiplierDescendantsChannel::Get(Entity e, float* values,
                                          size_t len) const {
  mathfu::vec4 color = mathfu::kOnes4f;
  render_system_->GetColor(e, &color);
  mathfu::vec4 default_color = render_system_->GetDefaultColor(e);

  values[0] = default_color[0] > 0.f ? (color[0] / default_color[0]) : 0.f;
  values[1] = default_color[1] > 0.f ? (color[1] / default_color[1]) : 0.f;
  values[2] = default_color[2] > 0.f ? (color[2] / default_color[2]) : 0.f;
  return true;
}

void RgbMultiplierDescendantsChannel::Set(Entity e, const float* values,
                                          size_t len) {
  auto fn = [this, values](Entity child) {
    mathfu::vec4 color;
    const bool use_default = !render_system_->GetColor(child, &color);
    const mathfu::vec4 default_color = render_system_->GetDefaultColor(child);
    color[0] = default_color[0] * values[0];
    color[1] = default_color[1] * values[1];
    color[2] = default_color[2] * values[2];
    color[3] = use_default ? default_color[3] : color[3];
    render_system_->SetColor(child, color);
  };

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->ForAllDescendants(e, fn);
}

AlphaMultiplierDescendantsChannel::AlphaMultiplierDescendantsChannel(
    Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 1, pool_size),
      registry_(registry),
      render_system_(registry->Get<RenderSystem>()) {}

void AlphaMultiplierDescendantsChannel::Setup(Registry* registry,
                                              size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* render_system = registry->Get<RenderSystem>();
  if (animation_system && render_system) {
    AnimationChannelPtr ptr(
        new AlphaMultiplierDescendantsChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup AlphaMultiplierDescendantsChannel.";
  }
}

bool AlphaMultiplierDescendantsChannel::Get(Entity e, float* values,
                                            size_t len) const {
  mathfu::vec4 color = mathfu::kOnes4f;
  render_system_->GetColor(e, &color);
  mathfu::vec4 default_color = render_system_->GetDefaultColor(e);
  values[0] = default_color[3] > 0.f ? (color[3] / default_color[3]) : 0.f;
  return true;
}

void AlphaMultiplierDescendantsChannel::Set(Entity e, const float* values,
                                            size_t len) {
  if (len != 1) {
    LOG(DFATAL) << "Must have 1 value for AlphaMultiplierDescendantsChannel!";
    return;
  }
  auto* transform_system = registry_->Get<TransformSystem>();
  SetAlphaMultiplierDescendants(e, values[0], transform_system, render_system_);
}

}  // namespace lull
