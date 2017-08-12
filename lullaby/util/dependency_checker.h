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

#ifndef LULLABY_UTIL_DEPENDENCY_CHECKER_H_
#define LULLABY_UTIL_DEPENDENCY_CHECKER_H_

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "lullaby/util/typeid.h"

namespace lull {

// Registers and checks dependencies between types.
class DependencyChecker {
 public:
  DependencyChecker() {}
  DependencyChecker(const DependencyChecker&) = delete;
  DependencyChecker& operator=(const DependencyChecker&) = delete;

  // Registers that there is a dependency for |dependent_type| on
  // |dependency_type|.
  void RegisterDependency(TypeId dependent_type, const char* dependent_name,
                          TypeId dependency_type, const char* dependency_name);

  // Satisfies a dependency for all types.
  void SatisfyDependency(TypeId dep);

  // Checks that all registered dependencies have been satisfied, logging DFATAL
  // if they are not.
  void CheckAllDependencies();

 private:
  // Data structure storing info about a type.
  struct TypeInfo {
    TypeId type;
    const char* name;

    TypeInfo(TypeId t, const char* n) : type(t), name(n) {}
  };

  // Data structure storing info about a dependency between two types.
  struct DependencyInfo {
    // The type that has the dependency.
    TypeInfo dependent_type;
    // The type being depended on.
    TypeInfo dependency_type;

    DependencyInfo(TypeInfo dependent, TypeInfo dependency)
        : dependent_type(dependent), dependency_type(dependency) {}
  };

  // Returns whether the given dependency is satisfied.
  bool IsDependencySatisfied(TypeId dep) const;

  // List of dependencies between types.
  std::vector<DependencyInfo> registered_dependencies_;

  // Set of satisfied dependencies types.
  // When a dependency is satisfied, it's satisfied for all dependent types. So
  // we only store the TypeIds of satisfied types, instead of the
  // DependencyInfo describing a dependency between two types.
  std::unordered_set<TypeId> satisfied_dependencies_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_DEPENDENCY_CHECKER_H_
