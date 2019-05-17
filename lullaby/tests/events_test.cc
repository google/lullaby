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

#include "gtest/gtest.h"
#include "lullaby/events/animation_events.h"
#include "lullaby/events/audio_events.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/fade_events.h"
#include "lullaby/events/input_events.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/events/lifetime_events.h"
#include "lullaby/events/render_events.h"
#include "lullaby/events/scroll_events.h"
#include "lullaby/events/ui_events.h"
#include "lullaby/modules/serialize/buffer_serializer.h"
#include "lullaby/modules/serialize/serialize.h"
#include "mathfu/constants.h"

// These tests ensure that the events correctly serialize all their members.

namespace lull {
namespace {

template<typename Event>
Event ProcessEvent(const Event& event) {
  std::vector<uint8_t> buffer;
  SaveToBuffer saver(&buffer);
  Serialize(&saver, const_cast<Event*>(&event), GetTypeId<Event>());

  Event result;
  LoadFromBuffer loader(&buffer);
  Serialize(&loader, &result, GetTypeId<Event>());

  return result;
}

TEST(AnimationCompleteEvent, AnimationCompleteEvent) {
  AnimationCompleteEvent event(1, 2);
  EXPECT_EQ(event.id, ProcessEvent(event).id);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(AudioLoadedEvent, AudioLoadedEvent) {
  AudioLoadedEvent event;
  ProcessEvent(event);
}

TEST(DisableAudioEnvironmentEvent, DisableAudioEnvironmentEvent) {
  DisableAudioEnvironmentEvent event;
  ProcessEvent(event);
}

TEST(OnDisabledEvent, OnDisabledEvent) {
  OnDisabledEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(OnEnabledEvent, OnEnabledEvent) {
  OnEnabledEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(OnInteractionDisabledEvent, OnInteractionDisabledEvent) {
  OnInteractionDisabledEvent event(123);
  EXPECT_EQ(event.entity, ProcessEvent(event).entity);
}

TEST(OnInteractionEnabledEvent, OnInteractionEnabledEvent) {
  OnInteractionEnabledEvent event(123);
  EXPECT_EQ(event.entity, ProcessEvent(event).entity);
}

TEST(FadeInCompleteEvent, FadeInCompleteEvent) {
  FadeInCompleteEvent event(1, true);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.interrupted, ProcessEvent(event).interrupted);
}

TEST(FadeOutCompleteEvent, FadeOutCompleteEvent) {
  FadeOutCompleteEvent event(1, true);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.interrupted, ProcessEvent(event).interrupted);
}

TEST(ParentChangedEvent, ParentChangedEvent) {
  ParentChangedEvent event(1, 2, 3);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.old_parent, ProcessEvent(event).old_parent);
  EXPECT_EQ(event.new_parent, ProcessEvent(event).new_parent);
}

TEST(ChildAddedEvent, ChildAddedEvent) {
  ChildAddedEvent event(1, 2);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.child, ProcessEvent(event).child);
}

TEST(ChildRemovedEvent, ChildRemovedEvent) {
  ChildRemovedEvent event(1, 2);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.child, ProcessEvent(event).child);
}

TEST(AabbChangedEvent, AabbChangedEvent) {
  AabbChangedEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(StartHoverEvent, StartHoverEvent) {
  StartHoverEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(StopHoverEvent, StopHoverEvent) {
  StopHoverEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(ClickEvent, ClickEvent) {
  ClickEvent event(123, mathfu::kZeros3f);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.location, ProcessEvent(event).location);
}

TEST(ClickPressedAndReleasedEvent, ClickPressedAndReleasedEvent) {
  ClickPressedAndReleasedEvent event(1);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(ClickReleasedEvent, ClickReleasedEvent) {
  ClickReleasedEvent event(1, 2);
  EXPECT_EQ(event.pressed_entity, ProcessEvent(event).pressed_entity);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(CollisionExitEvent, CollisionExitEvent) {
  CollisionExitEvent event(1);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(PrimaryButtonPress, PrimaryButtonPress) {
  PrimaryButtonPress event;
  ProcessEvent(event);
}

TEST(PrimaryButtonClick, PrimaryButtonClick) {
  PrimaryButtonClick event;
  ProcessEvent(event);
}

TEST(PrimaryButtonLongPress, PrimaryButtonLongPress) {
  PrimaryButtonLongPress event;
  ProcessEvent(event);
}

TEST(PrimaryButtonLongClick, PrimaryButtonLongClick) {
  PrimaryButtonLongClick event;
  ProcessEvent(event);
}

TEST(PrimaryButtonRelease, PrimaryButtonRelease) {
  PrimaryButtonRelease event;
  ProcessEvent(event);
}

TEST(SecondaryButtonPress, SecondaryButtonPress) {
  SecondaryButtonPress event;
  ProcessEvent(event);
}

TEST(SecondaryButtonClick, SecondaryButtonClick) {
  SecondaryButtonClick event;
  ProcessEvent(event);
}

TEST(SecondaryButtonLongPress, SecondaryButtonLongPress) {
  SecondaryButtonLongPress event;
  ProcessEvent(event);
}

TEST(SecondaryButtonLongClick, SecondaryButtonLongClick) {
  SecondaryButtonLongClick event;
  ProcessEvent(event);
}

TEST(SecondaryButtonRelease, SecondaryButtonRelease) {
  SecondaryButtonRelease event;
  ProcessEvent(event);
}

TEST(SystemButtonPress, SystemButtonPress) {
  SystemButtonPress event;
  ProcessEvent(event);
}

TEST(SystemButtonClick, SystemButtonClick) {
  SystemButtonClick event;
  ProcessEvent(event);
}

TEST(SystemButtonLongPress, SystemButtonLongPress) {
  SystemButtonLongPress event;
  ProcessEvent(event);
}

TEST(SystemButtonLongClick, SystemButtonLongClick) {
  SystemButtonLongClick event;
  ProcessEvent(event);
}

TEST(SystemButtonRelease, SystemButtonRelease) {
  SystemButtonRelease event;
  ProcessEvent(event);
}

TEST(GlobalRecenteredEvent, GlobalRecenteredEvent) {
  GlobalRecenteredEvent event;
  ProcessEvent(event);
}

TEST(LayoutChangedEvent, LayoutChangedEvent) {
  LayoutChangedEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(OriginalBoxChangedEvent, OriginalBoxChangedEvent) {
  OriginalBoxChangedEvent event(123);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
}

TEST(DesiredSizeChangedEvent, DesiredSizeChangedEvent) {
  DesiredSizeChangedEvent event(123, 456, 7.f, 8.f, Optional<float>());
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.source, ProcessEvent(event).source);
  EXPECT_EQ(event.x, ProcessEvent(event).x);
  EXPECT_EQ(event.y, ProcessEvent(event).y);
  EXPECT_EQ(event.z, ProcessEvent(event).z);
}

TEST(ActualBoxChangedEvent, ActualBoxChangedEvent) {
  ActualBoxChangedEvent event(123, 456);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.source, ProcessEvent(event).source);
}

TEST(OnPauseThreadUnsafeEvent, OnPauseThreadUnsafeEvent) {
  OnPauseThreadUnsafeEvent event;
  ProcessEvent(event);
}

TEST(OnResumeThreadUnsafeEvent, OnResumeThreadUnsafeEvent) {
  OnResumeThreadUnsafeEvent event;
  ProcessEvent(event);
}

TEST(OnResumeEvent, OnResumeEvent) {
  OnResumeEvent event;
  ProcessEvent(event);
}

TEST(OnQuitRequestEvent, OnQuitRequestEvent) {
  OnQuitRequestEvent event;
  ProcessEvent(event);
}

TEST(TextureReadyEvent, TextureReadyEvent) {
  TextureReadyEvent event(1, 2);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.texture_unit, ProcessEvent(event).texture_unit);
}

TEST(ReadyToRenderEvent, ReadyToRenderEvent) {
  ReadyToRenderEvent event(123);
  EXPECT_EQ(event.entity, ProcessEvent(event).entity);
}

TEST(HiddenEvent, HiddenEvent) {
  HiddenEvent event(123);
  EXPECT_EQ(event.entity, ProcessEvent(event).entity);
}

TEST(UnhiddenEvent, UnhiddenEvent) {
  UnhiddenEvent event(123);
  EXPECT_EQ(event.entity, ProcessEvent(event).entity);
}

TEST(ScrollViewTargeted, ScrollViewTargeted) {
  ScrollViewTargeted event;
  ProcessEvent(event);
}

TEST(ScrollOffsetChanged, ScrollOffsetChanged) {
  ScrollOffsetChanged event(123, mathfu::vec2(4, 5), mathfu::vec2(6, 7));
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.old_offset, ProcessEvent(event).old_offset);
  EXPECT_EQ(event.new_offset, ProcessEvent(event).new_offset);
}

TEST(ScrollVisibilityChanged, ScrollVisibilityChanged) {
  ScrollVisibilityChanged event(1, 2, true);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.scroll_view, ProcessEvent(event).scroll_view);
  EXPECT_EQ(event.visible, ProcessEvent(event).visible);
}

TEST(ScrollSnappedToEntity, ScrollSnappedToEntity) {
  ScrollSnappedToEntity event(1, 2);
  EXPECT_EQ(event.entity, ProcessEvent(event).entity);
  EXPECT_EQ(event.snapped_entity, ProcessEvent(event).snapped_entity);
}

TEST(ButtonClickEvent, ButtonClickEvent) {
  ButtonClickEvent event(1, 2);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.id, ProcessEvent(event).id);
}

TEST(TextChangedEvent, TextChangedEvent) {
  TextChangedEvent event(123, "hi");
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.text, ProcessEvent(event).text);
}

TEST(TextEnteredEvent, TextEnteredEvent) {
  TextEnteredEvent event(123, "hi");
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.text, ProcessEvent(event).text);
}

TEST(SliderEvent, SliderEvent) {
  SliderEvent event(1, 2, 3.f);
  EXPECT_EQ(event.target, ProcessEvent(event).target);
  EXPECT_EQ(event.id, ProcessEvent(event).id);
  EXPECT_EQ(event.value, ProcessEvent(event).value);
}

}  // namespace
}  // namespace lull
