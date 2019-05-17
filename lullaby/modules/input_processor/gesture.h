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

#ifndef LULLABY_UTIL_GESTURE_H_
#define LULLABY_UTIL_GESTURE_H_

#include <memory>
#include <vector>

#include "lullaby/modules/input/input_focus.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"

namespace lull {
/// The maximum number of touches that can be in a single gesture.  This is
/// primarily limited by the logic in InputProcessor::UpdateTouchGestures.
constexpr size_t kMaxTouchesPerGesture = 2;
/// Every Gesture event will include the touch id of the touches used by that
/// gesture.  These are the hashes used in the variantMap of the event.  This
/// should be kMaxTouchesPerGesture long.
// clang-format off
const HashValue kTouchIdHashes[] = {
    ConstHash("touch_0"),
    ConstHash("touch_1"),
  };
// clang-format on

class InputProcessor;

/// Gesture is a base class that actual gestures should extend.  This represents
/// a single active gesture, which owns some number of touches.
/// To create a gesture, override Gesture and GestureRecognizer, implementing
/// all of the virtual functions.
class Gesture {
 public:
  using TouchIdSpan = lull::Span<InputManager::TouchId>;
  using TouchIdVector = std::vector<InputManager::TouchId>;

  enum State {
    /// The gesture has been created, but has not been updated yet.
    kStarting,
    /// The gesture has been updated once, and is still running.
    kRunning,
    /// The gesture has completed successfully (i.e. a touch has been released,
    /// or another gesture has interupted this gesture).
    kEnding,
    /// The gesture has been interrupted, and should revert any side effects.
    /// This often happens on resume, if a gesture is active when an app is
    /// paused.
    kCanceled
  };

  virtual ~Gesture() {}
  /// Called by InputProcessor to set some initial values.
  void Setup(Registry* registry, HashValue hash,
             InputManager::DeviceType device, InputManager::TouchpadId touchpad,
             TouchIdSpan ids, const mathfu::vec2& touchpad_size_cm);
  /// Returns a hash of the name of the gesture.
  HashValue GetHash() const { return hash_; }
  /// Returns a vector of touch ids, which are the touches that are driving this
  /// gesture.
  const TouchIdVector& GetTouches() const { return ids_; }

  /// This will be called every frame while the gesture is active.  This should
  /// return the state of the gesture. If this returns kEnding or kCanceled, the
  /// gesture will be destroyed.  Cancel() will not be automatically called if
  /// this returns kCanceled.
  virtual State AdvanceFrame(const Clock::duration& delta_time) {
    return kEnding;
  }
  /// If the gesture has any side effects, they should be reverted here.
  /// This function may be called once per touch in a single frame during app
  /// resume, if the gesture was active when the app paused.
  virtual void Cancel() { state_ = kCanceled; }
  /// If the gesture events should have any custom values, return them here.
  /// Device id, touchpad id, touch ids, and other standard input event values
  /// will be set automatically.
  virtual VariantMap GetEventValues() { return VariantMap(); }

 protected:
  /// This will be called after Setup, and can be used to calculate any derived
  /// initial values.
  virtual void Initialize() {}

  Registry* registry_;
  Entity target_ = kNullEntity;
  InputManager::DeviceType device_;
  InputManager::TouchpadId touchpad_;
  InputManager* input_manager_;
  InputProcessor* input_processor_;
  TouchIdVector ids_;
  State state_ = kStarting;
  HashValue hash_;
  // Size of the screen in cm.  Multiply by inputManager touch deltas before
  // doing any threshold calculations.
  mathfu::vec2 touchpad_size_cm_;
};

using GesturePtr = std::shared_ptr<Gesture>;

/// The gesture recognizer should check if a set of touches qualifies as
/// a gesture, and if so create a Gesture for those touches.
class GestureRecognizer {
 public:
  /// |name| is used to generate event names and hashes in InputProcessor.  See
  /// GetName() for details.
  ///
  /// |num_touches| is the number of touches to pass into
  /// TryStart at a time.  A value of 2 would result in TryStart being called
  /// once for every possible pair of currently active touches.  This includes
  /// touches that are currently owned by other gestures.  |num_touches| Should
  /// never be larger than kMaxTouchesPerGesture.
  GestureRecognizer(Registry* registry, string_view name, size_t num_touches);
  virtual ~GestureRecognizer() {}

  /// The number of touches that constitutes a single instance of this gesture.
  size_t GetNumTouches() { return num_touches_; }

  /// Returns the name of the gesture.  This will be combined with event
  /// suffixes and possibly a device prefix to form the gesture's events.
  /// Events will be of the form:
  /// <Device Touch Prefix> + <Gesture Name> + <Event Suffix>.
  /// i.e. AnySwipeStartEvent, PinchCancelEvent.
  string_view GetName() { return name_; }
  HashValue GetHash() { return hash_; }

  /// This function should return a new Gesture if and only if the passed in
  /// touch |ids| have been identified as starting a gesture.  This function
  /// will be called with touches that are currently owned by another touch.
  virtual GesturePtr TryStart(InputManager::DeviceType device,
                              InputManager::TouchpadId touchpad,
                              Gesture::TouchIdSpan ids) {
    return nullptr;
  }

  /// Called by InputProcessor before any TryStart calls in a frame.  This sets
  /// the current display size in centimeters, so that touch thresholds can be
  /// independent of screen size.
  void SetTouchpadSize(const mathfu::vec2& touchpad_size_cm) {
    touchpad_size_cm_ = touchpad_size_cm;
  }

  /// Gets the physical size of the display in centimeters.
  const mathfu::vec2& GetTouchpadSize() { return touchpad_size_cm_; }

 protected:
  Registry* registry_;
  InputManager* input_manager_;
  InputProcessor* input_processor_;
  std::string name_;
  HashValue hash_;
  size_t num_touches_;
  // Size of the screen in cm.  Multiply by inputManager touch deltas before
  // doing any threshold calculations.
  mathfu::vec2 touchpad_size_cm_ = mathfu::vec2(-1.0f, -1.0f);
};

using GestureRecognizerPtr = std::shared_ptr<GestureRecognizer>;
using GestureRecognizerList = std::vector<GestureRecognizerPtr>;

}  // namespace lull

#endif  // LULLABY_UTIL_GESTURE_H_
