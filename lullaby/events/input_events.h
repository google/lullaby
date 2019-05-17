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

#ifndef LULLABY_EVENTS_INPUT_EVENTS_H_
#define LULLABY_EVENTS_INPUT_EVENTS_H_

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"
#include "mathfu/constants.h"

namespace lull {

/// The list of input events that are not associated with any particular button.
/// InputProcessor will use macros to expand this list into an enum statement
/// and name function.
/// All events in this list will include following parameters:
///   "entity"
///   "target"
///   "device"
#define LULLABY_DEVICE_EVENT_LIST(FUNC) \
  FUNC(kFocusStart, FocusStartEvent)    \
  FUNC(kFocusStop, FocusStopEvent)

/// The list of input events that are associated with any particular button.
/// InputProcessor will use macros to expand this list into an enum statement
/// and name function.
/// All events in this list will include following parameters:
///   "entity"
///   "target"
///   "device"
///   "button"
#define LULLABY_BUTTON_EVENT_LIST(FUNC) \
  FUNC(kPress, PressEvent)              \
  FUNC(kRelease, ReleaseEvent)          \
  FUNC(kClick, ClickEvent)              \
  FUNC(kLongPress, LongPressEvent)      \
  FUNC(kCancel, CancelEvent)            \
  FUNC(kDragStart, DragStartEvent)      \
  FUNC(kDragStop, DragStopEvent)

/// The list of touch events that are associated with a devices trackpad.
/// InputProcessor will use macros to expand this list into an enum statement
/// and name function.
/// All events in this list will include following parameters:
///   "entity"
///   "target"
///   "device"
///   "touchpad"
///   "touch"
#define LULLABY_TOUCH_EVENT_LIST(FUNC)  \
  FUNC(kTouchPress, PressEvent)         \
  FUNC(kTouchRelease, ReleaseEvent)     \
  FUNC(kTouchClick, ClickEvent)         \
  FUNC(kTouchLongPress, LongPressEvent) \
  FUNC(kTouchCancel, CancelEvent)       \
  FUNC(kTouchDragStart, DragStartEvent) \
  FUNC(kTouchDragStop, DragStopEvent)   \
  FUNC(kSwipeStart, SwipeStartEvent)    \
  FUNC(kSwipeStop, SwipeStopEvent)

/// The list of input events that are associated with a gesture.
/// InputProcessor will use macros to expand this list into an enum statement
/// and name function.  These will always be combined with a Gesture's
/// EventName, and the device touchpad's prefix.
/// See GestureRecognizer::GetName() for more details.
/// All events in this list will include following parameters:
///   "entity"
///   "target"
///   "device"
///   "touchpad"
///   "touch_0"
///   "touch_1"  (Only for gestures with > 1 touch)
#define LULLABY_GESTURE_EVENT_LIST(FUNC) \
  FUNC(kGestureStart, StartEvent)       \
  FUNC(kGestureStop, StopEvent)         \
  FUNC(kGestureCancel, CancelEvent)

/// Hashes of commonly used event names for convenience:
/// Input Events with no prefix.  Generally sent out by Controller 1's button 0.
/// Sent the first frame an entity is focused by a specific device.
constexpr HashValue kFocusStartEventHash = ConstHash("FocusStartEvent");
/// Sent when an entity is no longer focused by a specific device.
constexpr HashValue kFocusStopEventHash = ConstHash("FocusStopEvent");
/// Sent when a button is pressed down.
constexpr HashValue kPressEventHash = ConstHash("PressEvent");
/// Sent when a button is released
constexpr HashValue kReleaseEventHash = ConstHash("ReleaseEvent");
/// Sent when a button is pressed an released in less than 0.5 seconds and the
/// device's collision ray hasn't left a threshold.
constexpr HashValue kClickEventHash = ConstHash("ClickEvent");
/// Sent when a button is held down for more than 0.5 seconds and the device's
/// collision ray hasn't left a threshold.
constexpr HashValue kLongPressEventHash = ConstHash("LongPressEvent");
/// Sent when a button is held down until the device's collision ray leaves the
/// cancellation threshold.
constexpr HashValue kCancelEventHash = ConstHash("CancelEvent");
/// Sent when a button is held down until the device's collision ray leaves the
/// drag threshold.  Only sent if InputFocus.draggable is true for this entity.
constexpr HashValue kDragStartEventHash = ConstHash("DragStartEvent");
/// Sent when a release or cancel event is sent to an entity that received a
/// DragStartEvent.  Only sent if InputFocus.draggable is true for this entity.
constexpr HashValue kDragStopEventHash = ConstHash("DragStopEvent");
/// Sent when a touch moves outside the touch slop and the device's collision
/// ray hasn't left a threshold.
constexpr HashValue kSwipeStartEvent = ConstHash("SwipeStartEvent");
/// Sent when a release or cancel event is sent to an entity that received a
/// SwipeStartEvent.
constexpr HashValue kSwipeStopEvent = ConstHash("SwipeStopEvent");

/// Input Events with the "Any" prefix.  Sent out by every device being updated
/// by InputProcessor, and for every button and touchpad on those devices.
constexpr HashValue kAnyFocusStartEventHash = ConstHash("AnyFocusStartEvent");
constexpr HashValue kAnyFocusStopEventHash = ConstHash("AnyFocusStopEvent");
constexpr HashValue kAnyPressEventHash = ConstHash("AnyPressEvent");
constexpr HashValue kAnyReleaseEventHash = ConstHash("AnyReleaseEvent");
constexpr HashValue kAnyClickEventHash = ConstHash("AnyClickEvent");
constexpr HashValue kAnyLongPressEventHash = ConstHash("AnyLongPressEvent");
constexpr HashValue kAnyCancelEventHash = ConstHash("AnyCancelEvent");
constexpr HashValue kAnyDragStartEventHash = ConstHash("AnyDragStartEvent");
constexpr HashValue kAnyDragStopEventHash = ConstHash("AnyDragStopEvent");
constexpr HashValue kAnySwipeStartEventHash = ConstHash("AnySwipeStartEvent");
constexpr HashValue kAnySwipeStopEventHash = ConstHash("AnySwipeStopEvent");

/// Standard fields to be included in Input Events sent by the InputProcessor:
/// The entity the device is focused on, if any.
constexpr HashValue kEntityHash = ConstHash("entity");
constexpr HashValue kTargetHash = ConstHash("target");
/// The InputManager::DeviceType that caused the event.
constexpr HashValue kDeviceHash = ConstHash("device");
/// The InputManager::ButtonId that caused the event. Value will be
/// InputManager::kInvalidButton when sent from a touchpad.
constexpr HashValue kButtonHash = ConstHash("button");
/// The InputManager::TouchpadId of the touchpad that generated the event.
constexpr HashValue kTouchpadIdHash = ConstHash("touchpad");
/// The InputManager::TouchId of the single touch that generated the event.
/// For multi touch events (gesture events), see gesture.h's kTouchIdHashes.
constexpr HashValue kTouchIdHash = ConstHash("touch");

/// A mathfu::vec3 that is the local position of the cursor on the frame the
/// button was pressed, in the space of the pressed entity. Set for Press and
/// DragStart
constexpr HashValue kLocationHash = ConstHash("location");
/// A mathfu::vec2 of the touchpad position 0,0->1,1 set for all touch events
constexpr HashValue kTouchLocationHash = ConstHash("touch_location");
/// The originally pressed entity. Only set for ReleaseEvent.
constexpr HashValue kPressedEntityHash = ConstHash("pressed_entity");
/// A duration of time in milliseconds.
constexpr HashValue kDurationHash = ConstHash("duration");

// Event sent when a new device is connected.  This should be sent by the
// ControllerSystem for devices that are being displayed. May be sent multiple
// times if multiple entities are displaying the same device.
struct DeviceConnectedEvent {
  DeviceConnectedEvent() {}
  DeviceConnectedEvent(InputManager::DeviceType device, Entity display_entity)
      : device(device), display_entity(display_entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    int device_int = static_cast<int>(device);
    archive(&device_int, ConstHash("device"));
    device = static_cast<InputManager::DeviceType>(device_int);
    archive(&display_entity, ConstHash("display_entity"));
  }

  InputManager::DeviceType device = InputManager::kMaxNumDeviceTypes;
  Entity display_entity = kNullEntity;
};

// The below events are deprecated, and are only used if the reticle_system is
// still used by the application.

struct StartHoverEvent {
  StartHoverEvent() {}
  explicit StartHoverEvent(Entity e) : target(e) {}
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, kTargetHash);
  }
};

struct StopHoverEvent {
  StopHoverEvent() {}
  explicit StopHoverEvent(Entity e) : target(e) {}
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, kTargetHash);
  }
};

struct ClickEvent {
  ClickEvent() {}
  ClickEvent(Entity e, const mathfu::vec3& location)
      : target(e), location(location) {}
  Entity target = kNullEntity;
  // Location of the click in local coordinates of the entity.
  mathfu::vec3 location = mathfu::kZeros3f;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, kTargetHash);
    archive(&location, kLocationHash);
  }
};

struct ClickReleasedEvent {
  ClickReleasedEvent() {}
  explicit ClickReleasedEvent(Entity pressed_entity, Entity target)
      : pressed_entity(pressed_entity), target(target) {}

  // The original entity targeted by the input controller, as the user
  // initiates the button press.
  Entity pressed_entity = kNullEntity;

  // The current entity targeted by the input controller, as the user
  // releases the input button press.
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&pressed_entity, kPressedEntityHash);
    archive(&target, kTargetHash);
  }
};

struct ClickPressedAndReleasedEvent {
  ClickPressedAndReleasedEvent() {}
  explicit ClickPressedAndReleasedEvent(Entity e) : target(e) {}
  ClickPressedAndReleasedEvent(Entity e, int64_t ms)
      : target(e), duration(ms) {}
  Entity target = kNullEntity;
  int64_t duration = -1;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, kTargetHash);
    archive(&duration, kDurationHash);
  }
};

struct CollisionExitEvent {
  CollisionExitEvent() {}
  explicit CollisionExitEvent(Entity e) : target(e) {}
  Entity target = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, kTargetHash);
  }
};

// Sent when Primary button is pressed down.
struct PrimaryButtonPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is released in less than 500 ms.
struct PrimaryButtonClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is held for more than 500 ms.
struct PrimaryButtonLongPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is released after being held for more than 500 ms.
struct PrimaryButtonLongClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Primary button is released.
struct PrimaryButtonRelease {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is pressed down.
struct SecondaryButtonPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is released in less than 500 ms.
struct SecondaryButtonClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is held for more than 500 ms.
struct SecondaryButtonLongPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is released after being held for more than 500 ms.
struct SecondaryButtonLongClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when Secondary button is released.
struct SecondaryButtonRelease {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is pressed down.
struct SystemButtonPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is released in less than 500 ms.
struct SystemButtonClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is held for more than 500 ms.
struct SystemButtonLongPress {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is released after being held for more than 500 ms.
struct SystemButtonLongClick {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Sent when System button is released.
struct SystemButtonRelease {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

struct GlobalRecenteredEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DeviceConnectedEvent);

// Deprecated Events:
LULLABY_SETUP_TYPEID(lull::StartHoverEvent);
LULLABY_SETUP_TYPEID(lull::StopHoverEvent);
LULLABY_SETUP_TYPEID(lull::ClickEvent);
LULLABY_SETUP_TYPEID(lull::ClickPressedAndReleasedEvent);
LULLABY_SETUP_TYPEID(lull::ClickReleasedEvent);
LULLABY_SETUP_TYPEID(lull::CollisionExitEvent);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonPress);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonClick);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonLongPress);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonLongClick);
LULLABY_SETUP_TYPEID(lull::PrimaryButtonRelease);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonPress);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonClick);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonLongPress);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonLongClick);
LULLABY_SETUP_TYPEID(lull::SecondaryButtonRelease);
LULLABY_SETUP_TYPEID(lull::SystemButtonPress);
LULLABY_SETUP_TYPEID(lull::SystemButtonClick);
LULLABY_SETUP_TYPEID(lull::SystemButtonLongPress);
LULLABY_SETUP_TYPEID(lull::SystemButtonLongClick);
LULLABY_SETUP_TYPEID(lull::SystemButtonRelease);
LULLABY_SETUP_TYPEID(lull::GlobalRecenteredEvent);

#endif  // LULLABY_EVENTS_INPUT_EVENTS_H_
