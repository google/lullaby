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

#ifndef LULLABY_CONTRIB_MUTATOR_MUTATOR_SYSTEM_H_
#define LULLABY_CONTRIB_MUTATOR_MUTATOR_SYSTEM_H_

#include <set>
#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/math.h"

namespace lull {

/// The MutatorSystem is a utility that allows other systems to apply
/// standardized mutation functions.  Mutation systems like
/// StayInBoxMutatorSystem will handle associated Defs and register themselves
/// with MutatorSystem (Specified by Entity, Group hash, and Order).
/// Another system (i.e. GrabSystem) will ask for a set of mutations to be
/// applied, before doing something with the result.
/// NOTE: this system does not apply anything automatically.  Calling code is
/// responsible for actually using the mutated data.
/// See lullaby/examples/grab_simple for example usage.
class MutatorSystem : public System {
 public:
  /// An interface for systems that mutate sqts.
  class SqtMutatorInterface {
   public:
    virtual ~SqtMutatorInterface() {}

    /// Called to actually mutate an sqt.
    /// |mutate| is the current sqt, which should be mutated.
    /// If |require_valid| is true, then the mutated sqt cannot be 'stretching'
    /// outside of the bounds.  Generally used to get a valid position to
    /// animate to when a mutation will no longer be applied.
    virtual void Mutate(Entity entity, HashValue group, int order, Sqt* mutate,
                        bool require_valid) = 0;
  };

  explicit MutatorSystem(Registry* registry);

  void Destroy(Entity entity) override;

  /// Alter an Sqt using any mutators registered for the given group and entity.
  /// |mutate| should be the current sqt of the entity, and will have the
  /// mutators applied to it.
  /// If |require_valid| is true, then the mutated sqt will actually be in the
  /// bounds of any mutators, rather than "stretching" towards those bounds.
  void ApplySqtMutator(Entity entity, HashValue group, Sqt* mutate,
                       bool require_valid);

  /// Register a mutator for an entity.
  void RegisterSqtMutator(Entity entity, HashValue group, int order,
                          SqtMutatorInterface* mutator);

  /// Returns true if at least one mutator has been registered for this entity
  /// and group.
  bool HasSqtMutator(Entity entity, HashValue group) const;

 private:
  struct SqtMutatorComponent {
    SqtMutatorComponent(SqtMutatorInterface* mutator, int order)
        : order(order), mutator(mutator) {}
    int order;
    SqtMutatorInterface* mutator;

    // Providing a comparator so that std::set can auto sort the mutators.
    bool operator<(const SqtMutatorComponent& rhs) const {
      return order < rhs.order;
    }
  };

  using SqtSequence = std::set<SqtMutatorComponent>;
  using SqtGroupToSequence = std::unordered_map<HashValue, SqtSequence>;

  std::unordered_map<Entity, SqtGroupToSequence> sqt_mutators_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MutatorSystem);

#endif  // LULLABY_CONTRIB_MUTATOR_MUTATOR_SYSTEM_H_
