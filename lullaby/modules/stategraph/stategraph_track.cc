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

#include "lullaby/modules/stategraph/stategraph_track.h"

namespace lull {

const StategraphSignal* StategraphTrack::GetSignal(HashValue id) const {
  for (auto& signal : signals_) {
    if (signal->GetId() == id) {
      return signal.get();
    }
  }
  return nullptr;
}

void StategraphTrack::EnterActiveSignals(Clock::duration timestamp,
                                         TypedPointer userdata) const {
  for (auto& signal : signals_) {
    if (signal->IsActive(timestamp)) {
      signal->Enter(userdata);
    }
  }
}

void StategraphTrack::ExitActiveSignals(Clock::duration timestamp,
                                        TypedPointer userdata) const {
  for (auto& signal : signals_) {
    if (signal->IsActive(timestamp)) {
      signal->Exit(userdata);
    }
  }
}

void StategraphTrack::ProcessSignals(Clock::duration start_time,
                                     Clock::duration end_time,
                                     TypedPointer userdata) const {
  for (auto& signal : signals_) {
    const bool active_at_start = signal->IsActive(start_time);
    const bool active_at_end = signal->IsActive(end_time);
    const bool ending = active_at_start && !active_at_end;
    const bool starting = !active_at_start && active_at_end;
    const bool only_active_within_window =
        signal->GetStartTime() >= start_time &&
        signal->GetEndTime() <= end_time;

    if (starting || only_active_within_window) {
      signal->Enter(userdata);
    }
    if (ending || only_active_within_window) {
      signal->Exit(userdata);
    }
  }
}


}  // namespace lull
