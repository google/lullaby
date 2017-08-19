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

#include "lullaby/modules/stategraph/stategraph.h"

#include "lullaby/util/logging.h"

namespace lull {

void Stategraph::AddState(std::unique_ptr<StategraphState> state) {
  const HashValue id = state->GetId();
  auto iter = states_.find(id);
  if (iter != states_.end()) {
    LOG(DFATAL) << "State already in stategraph: " << id;
  }
  states_[id] = std::move(state);
}

const StategraphState* Stategraph::GetState(HashValue id) const {
  auto iter = states_.find(id);
  return iter != states_.end() ? iter->second.get() : nullptr;
}

Stategraph::Path Stategraph::FindPathHelper(const StategraphState* node,
                                            const StategraphState* dest,
                                            std::set<HashValue> visited) const {
  if (node == dest) {
    LOG(DFATAL) << "Should not have reached this far.";
    return {};
  }

  Path shortest_path;
  bool first = true;
  visited.insert(node->GetId());

  const std::vector<StategraphTransition>& transitions = node->GetTransitions();
  for (const StategraphTransition& transition : transitions) {
    const StategraphState* next = GetState(transition.to_state);
    if (next == nullptr) {
      LOG(DFATAL) << "Found a transition to an invalid state: "
                  << transition.to_state;
      continue;
    }
    if (visited.count(transition.to_state) != 0) {
      continue;
    }

    Path path;
    path.push_back(transition);

    if (next == dest) {
      shortest_path = std::move(path);
      break;
    }

    Path tmp = FindPathHelper(next, dest, visited);
    if (tmp.empty()) {
      continue;
    }

    path.insert(path.end(), tmp.begin(), tmp.end());
    if (first) {
      first = false;
      shortest_path = std::move(path);
    } else if (path.size() < shortest_path.size()) {
      shortest_path = std::move(path);
    }
  }
  return shortest_path;
}

Stategraph::Path Stategraph::FindPath(HashValue from_state_id,
                                      HashValue to_state_id) const {
  Path path;

  const StategraphState* from_state = GetState(from_state_id);
  if (from_state == nullptr) {
    LOG(DFATAL) << "Could not find initial state: " << from_state_id;
    return {};
  }

  const StategraphState* to_state = GetState(to_state_id);
  if (to_state == nullptr) {
    LOG(DFATAL) << "Could not find ending state: " << to_state_id;
    return {};
  }

  if (from_state == to_state) {
    return {};
  }

  std::set<HashValue> visited;
  return FindPathHelper(from_state, to_state, visited);
}

}  // namespace lull
