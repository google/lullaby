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

#ifndef LULLABY_CONTRIB_FADE_FADE_SYSTEM_H_
#define LULLABY_CONTRIB_FADE_FADE_SYSTEM_H_

#include "mathfu/glsl_mappings.h"
#include "lullaby/generated/fade_in_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/animation_events.h"
#include "lullaby/util/clock.h"

namespace lull {

// The FadeSystem animates and enable/disables entities.  By default it will
// play an opacity animation.
class FadeSystem : public System {
 public:
  explicit FadeSystem(Registry* registry);

  void Initialize() override;

  void PostCreateInit(Entity e, HashValue type, const Def* def) override;

  void Destroy(Entity e) override;

  // Animates the color uniform of an entity to the specified value.
  // Returns the AnimationId of the AnimationCompleteEvent signaling when
  // the animation has completed.
  AnimationId FadeTo(Entity e, const Clock::duration& time,
                     const mathfu::vec4& color);

  // Enables an entity (via TransformSystem::Enable) and an animation.  If no
  // custom FadeDef is on that entity, it will play an opacity animation
  // on the target entity and all of its children.
  void FadeIn(Entity e);
  // If time is specified as 0, no animation will be played and the entity will
  // be enabled.  Otherwise, if there is no FadeDef for |e| time will set the
  // duration of the animation.
  void FadeIn(Entity e, const Clock::duration& time);

  // Animates an entity then disables it(via TransformSystem::Disable).  If no
  // custom FadeDef is on that entity, it will play an opacity animation
  // on the target entity and all of its children.
  void FadeOut(Entity e);
  // If time is specified as 0, no animation will be played and the entity will
  // be disabled.  Otherwise, if there is no FadeDef for |e| time will set the
  // duration of the animation.
  void FadeOut(Entity e, const Clock::duration& time);

  // Updates the enabled/disabled state of an entity. If the entity is already
  // in the desired state this does nothing. Otherwise it calls FadeIn or
  // FadeOut as appropriate.
  void FadeToEnabledState(Entity e, bool enabled);
  // If time is specified as 0, no animation will be played and the entity will
  // be instantly updated.  Otherwise, if there is no FadeDef for |e| time will
  // set the duration of the animation.
  void FadeToEnabledState(Entity e, bool enabled,
                          const Clock::duration& time);

 private:
  struct FadeComponent : Component {
    explicit FadeComponent(Entity entity);
    const FadeDef* data = nullptr;
    AnimationId enable_animation_id = kNullAnimation;
    Dispatcher::Connection enable_animation_connection;
    AnimationId disable_animation_id = kNullAnimation;
    Dispatcher::Connection disable_animation_connection;
  };

  // Plays an animation on |e| based on |comp|.  If |comp| doesn't have a custom
  // enable_anim, this will animate the color channel of the entity from
  // invisible to what it was before starting the animation.
  // Returns the AnimationId of the AnimationCompleteEvent, or kNullAnimation
  // if no animations occurred.
  AnimationId AnimateFadeIn(const FadeComponent& comp, Entity e,
                            Clock::duration time);

  // Plays an animation on |e| based on |comp|.  If |comp| doesn't have a custom
  // disable_anim, this will animate the color channel of the entity  to
  // invisible.
  // Returns the AnimationId of the AnimationCompleteEvent, or kNullAnimation
  // if no animations occurred
  AnimationId AnimateFadeOut(const FadeComponent& comp, Entity e,
                             Clock::duration time);

  void FinishFadeIn(FadeComponent* fade, bool interrupted);
  void FinishFadeOut(FadeComponent* fade, bool interrupted);

  // Map of entities with custom fade responses.
  ComponentPool<FadeComponent> fades_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FadeSystem);

#endif  // LULLABY_CONTRIB_FADE_FADE_SYSTEM_H_
