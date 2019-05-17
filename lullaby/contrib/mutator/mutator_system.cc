/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/contrib/mutator/mutator_system.h"

namespace lull {

MutatorSystem::MutatorSystem(Registry* registry) : System(registry) {}

void MutatorSystem::Destroy(Entity entity) { sqt_mutators_.erase(entity); }

void MutatorSystem::ApplySqtMutator(Entity entity, HashValue group, Sqt* mutate,
                                    bool require_valid) {
  auto comp_iter = sqt_mutators_.find(entity);
  if (comp_iter == sqt_mutators_.end()) {
    return;
  }
  auto group_iter = comp_iter->second.find(group);
  if (group_iter == comp_iter->second.end()) {
    return;
  }
  SqtSequence& sequence = group_iter->second;
  for (auto iter = sequence.begin(); iter != sequence.end(); ++iter) {
    iter->mutator->Mutate(entity, group, iter->order, mutate, require_valid);
  }
}

void MutatorSystem::RegisterSqtMutator(Entity entity, HashValue group,
                                       int order,
                                       SqtMutatorInterface* mutator) {
  if (!mutator) {
    LOG(DFATAL) << "Null mutator passed to RegisterSqtMutator";
    return;
  }
  sqt_mutators_[entity][group].emplace(mutator, order);
}

bool MutatorSystem::HasSqtMutator(Entity entity, HashValue group) const {
  auto comp_iter = sqt_mutators_.find(entity);
  if (comp_iter == sqt_mutators_.end()) {
    return false;
  }
  auto group_iter = comp_iter->second.find(group);
  if (group_iter == comp_iter->second.end()) {
    return false;
  }
  return !group_iter->second.empty();
}

}  // namespace lull
