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

#ifndef LULLABY_UTIL_PERIODIC_FUNCTION_H_
#define LULLABY_UTIL_PERIODIC_FUNCTION_H_

#include <functional>
#include "lullaby/util/clock.h"

namespace lull {

// Calls a specified function at a regular interval.
class PeriodicFunction {
 public:
  PeriodicFunction() {}

  // Set the function to call and the period (interval) at which to call it.
  void Set(Clock::duration period, std::function<void()> fn) {
    time_left_ = period;
    period_ = period;
    fn_ = fn;
  }

  // Call the function if it's time and reset the timer based on the previously
  // provided interval.
  void AdvanceFrame(Clock::duration dt) {
    if (!fn_) {
      return;
    }
    time_left_ -= dt;
    if (time_left_ <= Clock::duration::zero()) {
      fn_();
      while (time_left_ < Clock::duration::zero()) {
        time_left_ += period_;
      }
    }
  }

  // Reset the timer to the previously speficied interval.
  void ResetTimer() { time_left_ = period_; }

 private:
  Clock::duration time_left_;
  Clock::duration period_;
  std::function<void()> fn_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_PERIODIC_FUNCTION_H_
