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

#ifndef LULLABY_UTIL_STATEGRAPH_STATEGRAPH_TRACK_H_
#define LULLABY_UTIL_STATEGRAPH_STATEGRAPH_TRACK_H_

#include <memory>
#include <vector>
#include "lullaby/modules/stategraph/stategraph_signal.h"
#include "lullaby/modules/stategraph/stategraph_transition.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

namespace lull {

// Base-class for a Track which represents "something" to be played within a
// State.  The "something" being played is provided by the derived class.
class StategraphTrack {
 public:
  virtual ~StategraphTrack() {}

  StategraphTrack(const StategraphTrack&) = delete;
  StategraphTrack& operator=(const StategraphTrack&) = delete;

  // Returns the selection parameters that will be used by the State to decide
  // which Track to play.
  const VariantMap& GetSelectionParams() const { return selection_params_; }

  // Returns the Signal with the specified |id|, or nullptr if there is no such
  // Signal.
  const StategraphSignal* GetSignal(HashValue id) const;

  // Calls Enter() and/or Exit() on Signals within the specified time window.
  // The userdata can be used to pass arbitary contexts to the underlying
  // signal callbacks.
  void ProcessSignals(Clock::duration start_time, Clock::duration end_time,
                      TypedPointer userdata = TypedPointer()) const;

  // Calls Enter() on all Signals that are active at the given |timestamp|.
  // The userdata can be used to pass arbitary contexts to the underlying
  // signal callbacks.
  void EnterActiveSignals(Clock::duration timestamp,
                          TypedPointer userdata = TypedPointer()) const;

  // Calls Exit() on all Signals that are active at the given |timestamp|.
  // The userdata can be used to pass arbitary contexts to the underlying
  // signal callbacks.
  void ExitActiveSignals(Clock::duration timestamp,
                         TypedPointer userdata = TypedPointer()) const;

 protected:
  StategraphTrack() {}

  // Used by a Selector to help decide which Track to pick.
  VariantMap selection_params_;

  // Sequence of Signals associated with the track.
  std::vector<std::unique_ptr<StategraphSignal>> signals_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StategraphTrack);

#endif  // LULLABY_UTIL_STATEGRAPH_STATEGRAPH_TRACK_H_
