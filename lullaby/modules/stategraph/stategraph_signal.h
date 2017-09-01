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

#ifndef LULLABY_UTIL_STATEGRAPH_STATEGRAPH_SIGNAL_H_
#define LULLABY_UTIL_STATEGRAPH_STATEGRAPH_SIGNAL_H_

#include "lullaby/util/clock.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/typed_pointer.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Base class for logic that is executed during specific time windows when
// playing a StategraphTrack.
class StategraphSignal {
 public:
  virtual ~StategraphSignal() {}

  StategraphSignal(const StategraphSignal&) = delete;
  StategraphSignal& operator=(const StategraphSignal&) = delete;

  // Returns the ID associated with the Signal.
  HashValue GetId() const { return id_; }

  // Returns the start time of the Signal.
  Clock::duration GetStartTime() const { return start_time_; }

  // Returns the end time of the Signal.
  Clock::duration GetEndTime() const { return end_time_; }

  // Returns true if the signal is "active" at the specified timestamp.
  bool IsActive(Clock::duration timestamp) const {
    return start_time_ <= timestamp && timestamp < end_time_;
  }

  // Called when the Signal is entered during Track playback.  The userdata can
  // be used to provide additional context around the signal.
  virtual void Enter(TypedPointer userdata) {}

  // Called when the Signal is exited during Track playback.  The userdata can
  // be used to provide additional context around the signal.
  virtual void Exit(TypedPointer userdata) {}

 protected:
  StategraphSignal(HashValue id, Clock::duration start_time,
                   Clock::duration end_time)
      : id_(id), start_time_(start_time), end_time_(end_time) {
    DCHECK(start_time <= end_time);
  }

 private:
  const HashValue id_ = 0;
  const Clock::duration start_time_;
  const Clock::duration end_time_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::StategraphSignal);

#endif  // LULLABY_UTIL_STATEGRAPH_STATEGRAPH_SIGNAL_H_
