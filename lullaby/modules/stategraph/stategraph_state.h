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

#ifndef LULLABY_UTIL_STATEGRAPH_STATEGRAPH_STATE_H_
#define LULLABY_UTIL_STATEGRAPH_STATEGRAPH_STATE_H_

#include <memory>
#include <vector>
#include "lullaby/modules/stategraph/stategraph_track.h"
#include "lullaby/modules/stategraph/stategraph_transition.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/selector.h"

namespace lull {

// Base-class for States inside the Stategraph.  States contain a list of
// Tracks and logic to select one of the Tracks to play when the State is
// entered.  The State also stores a list of Transitions out of the State and
// into other states.
class StategraphState {
 public:
  using TrackSelector = Selector<std::unique_ptr<StategraphTrack>>;

  virtual ~StategraphState() {}

  StategraphState(const StategraphState&) = delete;
  StategraphState& operator=(const StategraphState&) = delete;

  // Returns the ID associated with the State.
  HashValue GetId() const { return id_; }

  // Selects a Track within the State using the provided |args|.
  const StategraphTrack* SelectTrack(const VariantMap& args) const {
    if (selector_ == nullptr) {
      LOG(DFATAL) << "No selector specified.";
      return nullptr;
    }
    const auto index = selector_->Select(args, tracks_);
    return index ? tracks_[index.value()].get() : nullptr;
  }

  // Returns the list of Transitions existing this State.
  const std::vector<StategraphTransition>& GetTransitions() const {
    return transitions_;
  }

 protected:
  explicit StategraphState(HashValue id) : id_(id) {}

  // Sets the Selector to use for choosing a Track when the State is entered.
  void SetSelector(std::unique_ptr<TrackSelector> selector) {
    selector_ = std::move(selector);
  }

  // Adds a Track to the State.
  void AddTrack(std::unique_ptr<StategraphTrack> track) {
    tracks_.emplace_back(std::move(track));
  }

  // Adds a Transition to the State to another State.
  void AddTransition(StategraphTransition transition) {
    CHECK_EQ(transition.from_state, id_);
    CHECK_NE(transition.to_state, 0);
    transitions_.emplace_back(std::move(transition));
  }

 private:
  const HashValue id_;
  std::unique_ptr<TrackSelector> selector_;
  std::vector<std::unique_ptr<StategraphTrack>> tracks_;
  std::vector<StategraphTransition> transitions_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_STATEGRAPH_STATEGRAPH_STATE_H_
