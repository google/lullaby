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

#ifndef LULLABY_UTIL_STATEGRAPH_STATEGRAPH_TRANSITION_H_
#define LULLABY_UTIL_STATEGRAPH_STATEGRAPH_TRANSITION_H_

#include <utility>
#include <vector>
#include "lullaby/util/clock.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Represents a Transition between States in a Stategraph.
//
// A Transition is primarily just an association between a "from" State and a
// "to" State.  By default, a Transition is only valid when the State is done
// "playing" and enters the next State at the beginning (ie. time == 0).
// However, Transitions can also use an explicit time window or Signals within
// the States to specify different exit/enter times.
struct StategraphTransition {
  // The ID of the originating State for this Transition.
  HashValue from_state = 0;

  // The ID of the destination State for this Transition.
  HashValue to_state = 0;

  // Transitions may only be valid when a specific Signal is active in the
  // "from" State/Track.  In these situations, a secondary target Signal will
  // specify the time at which to enter the next State.  A target Signal of
  // 0 indiciates the State should be entered at time 0.
  using SignalPair = std::pair<HashValue, HashValue>;
  std::vector<SignalPair> signals;

  // By default, Transitions are valid only at the end of the "from"
  // State/Track.  An extra time window can be provided which allows the
  // transition to occur before the end.  A time window of Clock::duration::max
  // will effectively allow the transition to be valid for the entire duration
  // of the State.
  Clock::duration active_time_from_end = Clock::duration(0);

  // For a non-Signal based transition, this is the target Signal to enter
  // playback.
  HashValue to_signal = 0;

  // The length of time to take for the transition. This is intended to be used
  // for things like animation blending between tracks. As far as the actual
  // stategraph is concerned, only a single state should be active.
  Clock::duration transition_time = Clock::duration(0);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StategraphTransition);

#endif  // LULLABY_UTIL_STATEGRAPH_STATEGRAPH_TRANSITION_H_
