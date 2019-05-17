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

#include "lullaby/contrib/fade/fade_system.h"

#include <chrono>
#include "mathfu/constants.h"
#include "lullaby/events/fade_events.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/modules/animation_channels/render_channels.h"

namespace lull {

namespace {
const HashValue kFadeResponseHash = ConstHash("FadeDef");
const Clock::duration kDefaultFadeTime = std::chrono::milliseconds(250);
}

FadeSystem::FadeComponent::FadeComponent(Entity entity)
    : Component(entity), data(nullptr) {}

FadeSystem::FadeSystem(Registry* registry) : System(registry), fades_(16) {
  RegisterDef<FadeDefT>(this);
  RegisterDependency<AnimationSystem>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<RenderSystem>(this);
}

void FadeSystem::Initialize() {
  auto* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const FadeInEvent& e) {
    const auto duration = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float, std::milli>(e.time_ms));
    FadeIn(e.entity, duration);
  });
  dispatcher->Connect(this, [this](const FadeOutEvent& e) {
    const auto duration = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float, std::milli>(e.time_ms));
    FadeOut(e.entity, duration);
  });
}

void FadeSystem::Destroy(Entity e) { fades_.Destroy(e); }

void FadeSystem::PostCreateInit(Entity e, HashValue type, const Def* def) {
  if (type == kFadeResponseHash) {
    auto* data = ConvertDef<FadeDef>(def);
    FadeComponent* fade = fades_.Emplace(e);
    if (!fade) {
      fade = fades_.Get(e);
    }
    fade->data = data;
    if (data->enable_input_events()) {
      auto response = [this, e](const EventWrapper&) { FadeIn(e); };
      ConnectEventDefs(registry_, e, data->enable_input_events(), response);
    }

    if (data->disable_input_events()) {
      auto response = [this, e](const EventWrapper&) { FadeOut(e); };
      ConnectEventDefs(registry_, e, data->disable_input_events(), response);
    }

    fade->disable_animation_id = kNullAnimation;
    if (data->start_disabled()) {
      FadeOut(e, std::chrono::milliseconds(0));
    } else if (data->animate_on_create()) {
      FadeIn(e);
    }
  } else {
    LOG(DFATAL) << "Unknown type in FadeSystem::PostCreateInit";
  }
}

AnimationId FadeSystem::FadeTo(Entity e, const Clock::duration& time,
                               const mathfu::vec4& color) {
  auto animation_system = registry_->Get<AnimationSystem>();
  return animation_system->SetTarget(e, UniformChannel::kColorChannelName,
                                     &color.x, 4, time);
}

void FadeSystem::FadeIn(Entity e) { FadeIn(e, kDefaultFadeTime); }

void FadeSystem::FadeIn(Entity e, const Clock::duration& time) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  auto transform_system = registry_->Get<TransformSystem>();

  FadeComponent* fade = fades_.Get(e);
  if (!fade) {
    fade = fades_.Emplace(e);
  }

  if (fade->data && fade->data->only_animate_on_change() &&
      transform_system->IsEnabled(e) &&
      fade->disable_animation_id == kNullAnimation) {
    FinishFadeIn(fade, false);
    return;
  }

  // Cause an interrupt for any playing disable animation:
  fade->disable_animation_id = kNullAnimation;

  transform_system->Enable(e);

  FadeInheritMode inherit_mode = FadeInheritMode_SelfAndChildren;
  if (fade->data) {
    inherit_mode = fade->data->inherit_mode();
  }
  // Defer any fadeInComplete event until the newest fadeIn is complete.
  if (fade->enable_animation_id != kNullAnimation) {
    fade->enable_animation_id = kNullAnimation;
    fade->enable_animation_connection.Disconnect();
  }

  Entity first_animating = kNullEntity;
  switch (inherit_mode) {
    case FadeInheritMode_SelfOnly:
      fade->enable_animation_id = AnimateFadeIn(*fade, e, time);
      first_animating = e;
      break;
    case FadeInheritMode_SelfAndChildren:
      transform_system->ForAllDescendants(
          e, [this, fade, &first_animating, time](Entity child) {
            AnimationId child_anim = AnimateFadeIn(*fade, child, time);
            if (fade->enable_animation_id == kNullAnimation &&
                child_anim != kNullAnimation) {
              first_animating = child;
              fade->enable_animation_id = child_anim;
            }
          });
      break;
  }
  if (fade->enable_animation_id == kNullAnimation) {
    FinishFadeIn(fade, false);
  } else {
    fade->enable_animation_connection = dispatcher_system->Connect(
        first_animating, this,
        [this, e](const AnimationCompleteEvent& event) {
          FadeComponent* fade = fades_.Get(e);
          const bool same_animation = event.id == fade->enable_animation_id;
          if (same_animation) {
            FinishFadeIn(fade, false);
          } else if (fade->enable_animation_id == kNullAnimation) {
            FinishFadeIn(fade, true);
          }
        });
  }
}

void FadeSystem::FadeOut(Entity e) { FadeOut(e, kDefaultFadeTime); }

void FadeSystem::FadeOut(Entity e, const Clock::duration& time) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* collision_system = registry_->Get<CollisionSystem>();

  FadeComponent* fade = fades_.Get(e);
  if (!fade) {
    fade = fades_.Emplace(e);
  }

  // Cause an interrupt for any playing enable animation:
  fade->enable_animation_id = kNullAnimation;

  if (!transform_system->IsEnabled(e)) {
    // Already disabled, although that might be inherited.  Don't play any
    // animation.
    FinishFadeOut(fade, false);
    return;
  }

  if (fade->disable_animation_id != kNullAnimation) {
    // If we've already started a fade out on the entity, we just let it run.
    return;
  }

  // Find all animation targets.
  std::vector<Entity> targets;
  FadeInheritMode inherit_mode = FadeInheritMode_SelfAndChildren;
  if (fade->data) {
    inherit_mode = fade->data->inherit_mode();
  }
  switch (inherit_mode) {
    case FadeInheritMode_SelfOnly:
      targets.push_back(e);
      break;
    case FadeInheritMode_SelfAndChildren:
      transform_system->ForAllDescendants(
          e, [&targets](Entity child) { targets.push_back(child); });
      break;
  }

  // Start an animation on all targets.
  Entity first_animating = kNullEntity;
  for (auto target : targets) {
    AnimationId id = AnimateFadeOut(*fade, target, time);
    if (fade->disable_animation_id == kNullAnimation && id != kNullAnimation) {
      fade->disable_animation_id = id;
      first_animating = target;
    }
  }

  if (fade->disable_animation_id == kNullAnimation) {
    // No animation, so disable immediately.
    FinishFadeOut(fade, false);
  } else {
    // If the entity had collision-checking, disable it during the animation,
    // but keep a list of those entities so we can restore it once the animation
    // completes.
    std::vector<Entity> collisionable_entities;
    if (collision_system && (!fade->data || fade->data->disable_collision())) {
      for (auto target : targets) {
        if (collision_system->IsCollisionEnabled(target)) {
          collisionable_entities.emplace_back(target);
          collision_system->DisableCollision(target);
        }
      }
    }

    // Whenever the first animation completes, disable the root entity.
    fade->disable_animation_connection = dispatcher_system->Connect(
        first_animating, this,
        [this, collision_system, e, collisionable_entities](
            const AnimationCompleteEvent& event) {
          FadeComponent* fade = fades_.Get(e);
          const bool same_animation = event.id == fade->disable_animation_id;
          if (same_animation || fade->disable_animation_id == kNullAnimation) {
            // Reenable collision on all the descendant entities we disabled.
            if (collision_system &&
                (!fade->data || fade->data->disable_collision())) {
              for (Entity e : collisionable_entities) {
                collision_system->EnableCollision(e);
              }
            }
          }
          if (same_animation) {
            // Fade out animation has completed.
            FinishFadeOut(fade, false);
          } else if (fade->disable_animation_id == kNullAnimation) {
            // Fade out animation was interrupted.
            FinishFadeOut(fade, true);
          }
        });
  }
}

void FadeSystem::FadeToEnabledState(Entity e, bool enabled) {
  FadeToEnabledState(e, enabled, kDefaultFadeTime);
}

void FadeSystem::FadeToEnabledState(Entity e, bool enabled,
                                    const Clock::duration& time) {
  bool is_enabled = registry_->Get<TransformSystem>()->IsLocallyEnabled(e);
  if (enabled != is_enabled) {
    if (enabled) {
      FadeIn(e, time);
    } else {
      FadeOut(e, time);
    }
  } else {
    FadeComponent* fade = fades_.Get(e);
    if (fade) {
      // Check if we need to interrupt a fade.
      if (enabled) {
        if (fade->disable_animation_id != kNullAnimation) {
          FadeIn(e, time);
        }
      } else {
        if (fade->enable_animation_id != kNullAnimation) {
          FadeOut(e, time);
        }
      }
    }
  }
}

AnimationId FadeSystem::AnimateFadeIn(const FadeComponent& fade, Entity e,
                                      Clock::duration time) {
  if (fade.data && fade.data->enable_anim()) {
    if (time.count() == 0) {
      // TODO would be good to be able to jump to the end of the
      // enable_anim here.
      return kNullAnimation;
    } else {
      auto* animation_system = registry_->Get<AnimationSystem>();
      if (animation_system) {
        return animation_system->PlayAnimation(e, fade.data->enable_anim());
      }
    }
  } else {
    // No custom animation, play the default alpha fade animation.
    auto* render_system = registry_->Get<RenderSystem>();
    // Get the get the target color from the render uniform, set the color to
    // (r, g, b, 0), and fade in to the target color over time.
    mathfu::vec4 color;
    if (render_system && render_system->GetUniform(e, "color", 4, &color[0])) {
      // Fade in to the default opacity.
      color[3] = render_system->GetDefaultColor(e)[3];
      if (time.count() == 0) {
        render_system->SetUniform(e, "color", &color[0], 4);
        return kNullAnimation;
      } else {
        mathfu::vec4 invisible_color(color);
        invisible_color[3] = 0;
        render_system->SetUniform(e, "color", &invisible_color[0], 4);
        if (fade.data) {
          time = std::chrono::milliseconds(fade.data->fade_time_ms());
        }
        return FadeTo(e, time, color);
      }
    }
  }
  return kNullAnimation;
}

AnimationId FadeSystem::AnimateFadeOut(const FadeComponent& fade, Entity e,
                                       Clock::duration time) {
  if (fade.data && fade.data->disable_anim()) {
    if (time.count() == 0) {
      // TODO would be good to be able to jump to the end of the
      // disable_anim here.
      return kNullAnimation;
    } else {
      auto* animation_system = registry_->Get<AnimationSystem>();
      if (animation_system) {
        return animation_system->PlayAnimation(e, fade.data->disable_anim());
      }
    }
  } else {
    // No custom animation, play the default alpha fade animation.
    auto* render_system = registry_->Get<RenderSystem>();
    mathfu::vec4 color;
    if (render_system && render_system->GetUniform(e, "color", 4, &color[0])) {
      mathfu::vec4 invisible_color(color);
      invisible_color[3] = 0;
      if (time.count() == 0) {
        render_system->SetUniform(e, "color", &color[0], 4);
        return kNullAnimation;
      } else {
        if (fade.data) {
          time = std::chrono::milliseconds(fade.data->fade_time_ms());
        }
        return FadeTo(e, time, invisible_color);
      }
    }
  }
  return kNullAnimation;
}

void FadeSystem::FinishFadeIn(FadeComponent* fade, bool interrupted) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Send(fade->GetEntity(),
                          FadeInCompleteEvent(fade->GetEntity(), interrupted));
  fade->enable_animation_id = kNullAnimation;
  fade->enable_animation_connection.Disconnect();
}

void FadeSystem::FinishFadeOut(FadeComponent* fade, bool interrupted) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Send(fade->GetEntity(),
                          FadeOutCompleteEvent(fade->GetEntity(), interrupted));
  fade->disable_animation_id = kNullAnimation;
  if (!interrupted) {
    auto* transform_system = registry_->Get<TransformSystem>();
    transform_system->Disable(fade->GetEntity());
  }
}

}  // namespace lull
