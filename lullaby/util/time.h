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

#ifndef LULLABY_UTIL_TIME_H_
#define LULLABY_UTIL_TIME_H_

#include "lullaby/util/clock.h"

namespace lull {

using Secondsf = std::chrono::duration<float>;
using Millisecondsf = std::chrono::duration<float, std::milli>;
using Microsecondsf = std::chrono::duration<float, std::micro>;
using Nanosecondsf = std::chrono::duration<float, std::nano>;

inline Clock::duration DurationFromSeconds(float sec) {
  return std::chrono::duration_cast<Clock::duration>(Secondsf(sec));
}

inline Clock::duration DurationFromMilliseconds(float ms) {
  return std::chrono::duration_cast<Clock::duration>(Millisecondsf(ms));
}

inline float MicrosecondsFromDuration(Clock::duration d) {
  return std::chrono::duration_cast<Microsecondsf>(d).count();
}

inline float MillisecondsFromDuration(Clock::duration d) {
  return std::chrono::duration_cast<Millisecondsf>(d).count();
}

inline float SecondsFromDuration(Clock::duration d) {
  return std::chrono::duration_cast<Secondsf>(d).count();
}

inline float MicrosecondsFromSeconds(float sec) {
  return std::chrono::duration_cast<Microsecondsf>(Secondsf(sec)).count();
}

inline float MillisecondsFromNanoseconds(uint64_t ns) {
  return std::chrono::duration_cast<Millisecondsf>(
      std::chrono::duration<uint64_t, std::nano>(ns)).count();
}

inline float SecondsFromMilliseconds(float ms) {
  return std::chrono::duration_cast<Secondsf>(Millisecondsf(ms)).count();
}

class Timer {
 public:
  Timer() { Reset(); }

  void Reset() { start_ = Clock::now(); }

  Clock::duration GetElapsedTime() const { return Clock::now() - start_; }

 private:
  Clock::time_point start_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_TIME_H_
