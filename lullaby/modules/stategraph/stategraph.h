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

#ifndef LULLABY_UTIL_STATEGRAPH_STATEGRAPH_H_
#define LULLABY_UTIL_STATEGRAPH_STATEGRAPH_H_

#include <deque>
#include <memory>
#include <set>
#include <unordered_map>
#include "lullaby/modules/stategraph/stategraph_state.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"

namespace lull {

// A graph consisting of States and Transitions.
//
// The Stategraph stores a set of StategraphState objects.  Each State contains
// a list of StategraphTransitions that link to a neighbouring State.
// Transitions are single-directional, with the State that owns the Transition
// being the originating State for the Transition.  The Stategraph provides
// functions to find a path of Transitions between two States.
class Stategraph {
 public:
  Stategraph() {}

  Stategraph(const Stategraph&) = delete;
  Stategraph& operator=(const Stategraph&) = delete;

  // Adds a State to the graph.
  void AddState(std::unique_ptr<StategraphState> state);

  // Returns the State in the graph with the specified |id|, or nullptr if
  // no such State exists.
  const StategraphState* GetState(HashValue id) const;

  // Returns the sequence of Transitions required to go between the given
  // States.
  using Path = std::deque<StategraphTransition>;
  Path FindPath(HashValue from_state_id, HashValue to_state_id) const;

 private:
  Path FindPathHelper(const StategraphState* node, const StategraphState* dest,
                      std::set<HashValue> visited) const;

  std::unordered_map<HashValue, std::unique_ptr<StategraphState>> states_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Stategraph);

#endif  // LULLABY_UTIL_STATEGRAPH_STATEGRAPH_H_
