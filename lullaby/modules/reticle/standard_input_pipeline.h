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

#ifndef LULLABY_UTIL_STANDARD_INPUT_PIPELINE_H_
#define LULLABY_UTIL_STANDARD_INPUT_PIPELINE_H_

#include <deque>

#include "lullaby/modules/input/input_focus.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/span.h"

namespace lull {

/// This is an ease-of-use class designed to encapsulate the default way to
/// handle input in Lullaby.  Use this class either by directly calling
/// AdvanceFrame(), or by calling the broken out utility functions in your own
/// input handling method.
class StandardInputPipeline {
 public:
  explicit StandardInputPipeline(Registry* registry);

  /// Executes the entire standard input update for the current main device.
  void AdvanceFrame(const Clock::duration& delta_time);

  /// Executes the entire standard input update for the given device.
  /// Notes:
  /// If this function is called directly instead of AdvanceFrame, the
  /// DevicePreference and PrimaryDevice functionality won't work unless the app
  /// calls input_processor->SetPrimaryDevice(pipeline->GetPrimaryDevice()) each
  /// frame, before calling this function for that device.
  void UpdateInputFocus(const Clock::duration& delta_time,
                        InputManager::DeviceType device);

  /// Specifies the preferred devices to be used.  The lowest index connected
  /// device in this vector will be treated as the main device.
  void SetDevicePreference(Span<InputManager::DeviceType> devices);

  /// Re calculates the collision ray so that it comes from the hmd, but points
  /// towards the pre-collision cursor position.
  void MakeRayComeFromHmd(InputFocus* focus);

  /// Applies standard systems that modify the focused entity.  i.e. collision
  /// detection, focus locking, input behavior, etc.
  void ApplySystemsToInputFocus(InputFocus* focus);

  /// Applies the collision system to the input focus.
  void ApplyCollisionSystemToInputFocus(InputFocus* focus) const;

  /// Returns the type of the device currently used as the primary input.
  InputManager::DeviceType GetPrimaryDevice() const;

  /// Gets a world space Ray coming from the input device.
  /// If |parent| is not kNullEntity, the device's position and rotation will be
  /// combined with the parent's transform to make the input source act as a
  /// child of |parent|.
  Ray GetDeviceSelectionRay(InputManager::DeviceType device,
                            Entity parent) const;

 private:
  Registry* registry_;
  std::vector<InputManager::DeviceType> device_preference_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StandardInputPipeline);

#endif  // LULLABY_UTIL_STANDARD_INPUT_PIPELINE_H_
