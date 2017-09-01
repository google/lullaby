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

#ifndef LULLABY_UTIL_CLOCK_H_
#define LULLABY_UTIL_CLOCK_H_

#include <chrono>
#include "lullaby/util/typeid.h"

namespace lull {


template <class Ratio>
struct ClockBase {
  typedef int64_t rep;
  typedef Ratio period;
  typedef std::chrono::duration<rep, period> duration;
  typedef std::chrono::time_point<ClockBase> time_point;

  static time_point now();
};

typedef ClockBase<std::nano> Clock;

template <class Ratio>
typename ClockBase<Ratio>::time_point ClockBase<Ratio>::now() {
  return std::chrono::duration_cast<ClockBase>(Clock::now().time_since_epoch());
}

template <>
inline ClockBase<std::nano>::time_point ClockBase<std::nano>::now() {
  typedef std::conditional<std::chrono::high_resolution_clock::is_steady,
                           std::chrono::high_resolution_clock,
                           std::chrono::steady_clock>::type steady_clock;
  return time_point() + std::chrono::duration_cast<duration>(
                            steady_clock::now().time_since_epoch());
}


}  //  namespace lull

LULLABY_SETUP_TYPEID(lull::Clock::duration);

#endif  // LULLABY_UTIL_CLOCK_H_
