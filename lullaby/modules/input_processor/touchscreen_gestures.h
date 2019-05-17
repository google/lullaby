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

#ifndef LULLABY_UTIL_TOUCHSCREEN_GESTURES_H_
#define LULLABY_UTIL_TOUCHSCREEN_GESTURES_H_

#include <memory>
#include <set>
#include <vector>

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/gesture.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {

class OneFingerDragRecognizer : public GestureRecognizer {
 public:
  using Callback =
      std::function<void(Gesture::State, Entity, const mathfu::vec2&)>;
  class OneFingerDrag : public Gesture {
   public:
    /// The |callback| function will be called every frame while the gesture is
    /// active.  On the first frame the state will be kStarting.  On the last
    /// frame it will be either kEnding or kCanceled.  If it is kCanceled, any
    /// changes made in the current gesture should be undone.
    /// Callback will also receive the currently targeted entity, and the
    /// current uv of the touch.
    explicit OneFingerDrag(Callback callback)
        : callback_(std::move(callback)) {}
    Gesture::State AdvanceFrame(const Clock::duration& delta_time) override;

   protected:
    Callback callback_;
  };

  OneFingerDragRecognizer(Registry* registry, string_view event_name,
                          Callback callback)
      : GestureRecognizer(registry, event_name, 1), callback_(callback) {}
  GesturePtr TryStart(InputManager::DeviceType device,
                      InputManager::TouchpadId touchpad,
                      Gesture::TouchIdSpan ids) override;

 protected:
  Callback callback_;
};

class TwistRecognizer : public GestureRecognizer {
 public:
  using Callback = std::function<void(Gesture::State, Entity, float)>;
  class Twist : public Gesture {
   public:
    /// The |callback| function will be called every frame while the gesture is
    /// active.  On the first frame the state will be kStarting.  On the last
    /// frame it will be either kEnding or kCanceled.  If it is kCanceled, any
    /// changes made in the current gesture should be undone.
    /// Callback will also receive the currently targeted entity, and the
    /// current angle of the twist in radians.
    explicit Twist(Callback callback) : callback_(std::move(callback)) {}
    Gesture::State AdvanceFrame(const Clock::duration& delta_time) override;

   protected:
    Callback callback_;
  };

  TwistRecognizer(Registry* registry, string_view event_name, Callback callback)
      : GestureRecognizer(registry, event_name, 2), callback_(callback) {}
  GesturePtr TryStart(InputManager::DeviceType device,
                      InputManager::TouchpadId touchpad,
                      Gesture::TouchIdSpan ids) override;

 protected:
  Callback callback_;
};

class PinchRecognizer : public GestureRecognizer {
 public:
  using Callback = std::function<void(Gesture::State, Entity, float)>;
  class Pinch : public Gesture {
   public:
    /// The |callback| function will be called every frame while the gesture is
    /// active.  On the first frame the state will be kStarting.  On the last
    /// frame it will be either kEnding or kCanceled.  If it is kCanceled, any
    /// changes made in the current gesture should be undone.
    /// Callback will also receive the currently targeted entity, and the
    /// ratio of initial touch gap to current gap.
    explicit Pinch(Callback callback) : callback_(std::move(callback)) {}
    void Initialize() override;
    Gesture::State AdvanceFrame(const Clock::duration& delta_time) override;

   protected:
    Callback callback_;
    float start_gap_;
    mathfu::vec3 start_scale_;
  };

  PinchRecognizer(Registry* registry, string_view event_name, Callback callback)
      : GestureRecognizer(registry, event_name, 2), callback_(callback) {}
  GesturePtr TryStart(InputManager::DeviceType device,
                      InputManager::TouchpadId touchpad,
                      Gesture::TouchIdSpan ids) override;

 protected:
  Callback callback_;
};
}  // namespace lull

#endif  // LULLABY_UTIL_TOUCHSCREEN_GESTURES_H_
