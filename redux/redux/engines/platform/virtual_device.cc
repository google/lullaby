/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/engines/platform/virtual_device.h"

namespace redux {

const VirtualDevice::TriggerFlag VirtualDevice::DetermineTrigger(bool curr,
                                                                 bool prev) {
  if (curr) {
    return prev ? kPressed : (kPressed | kJustPressed);
  } else {
    return !prev ? kReleased : (kReleased | kJustReleased);
  }
}

const VirtualDevice::TriggerFlag VirtualDevice::DetermineTrigger(
    const BooleanState& curr, const BooleanState& prev,
    std::optional<float> long_press_time_ms) {
  // const absl::Duration long_press =
  //     absl::Milliseconds(long_press_time_ms.value_or(500.f));

  TriggerFlag flag = 0;
  if (curr.active) {
    flag |= kPressed;
    if (!prev.active) {
      flag |= kJustPressed;
    }
    // if (repeat) {
    //   flag |= kRepeat;
    // }

    // Check for long press:
    // const absl::Duration curr_press_duration =
    //     curr_time_stamp - curr.toggle_time;
    // const bool curr_long_press = curr_press_duration >= long_press;
    // if (curr_long_press) {
    //   flag |= kLongPressed;
    //   if (prev.active) {
    //     const absl::Duration prev_press_duration =
    //         prev_time_stamp - prev.toggle_time;
    //     const bool prev_long_press = prev_press_duration >= long_press;
    //     if (!prev_long_press) {
    //       flag |= kJustLongPressed;
    //     }
    //   } else {
    //     flag |= kJustLongPressed;
    //   }
    // }
  } else {
    flag |= kReleased;
    if (prev.active) {
      flag |= kJustReleased;

      // // Check if the released press was held for more than long_press_time.
      // // If released on the first frame that would be a long_press, assume it
      // // was released before the time limit passed and don't set
      // kLongPressed. const absl::Duration prev_press_duration =
      //     prev_time_stamp - prev.toggle_time;
      // const bool prev_long_press = prev_press_duration >= long_press;
      // if (prev_long_press) {
      //   flag |= kLongPressed;
      // }
      // } else if (was) {
      //   flag |= kJustPressed | kJustReleased;
    }
  }
  return flag;
}

}  // namespace redux
