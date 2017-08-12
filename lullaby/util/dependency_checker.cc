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

#include "lullaby/util/dependency_checker.h"

#include "lullaby/util/logging.h"

namespace lull {

void DependencyChecker::RegisterDependency(TypeId dependent_type,
                                           const char* dependent_name,
                                           TypeId dependency_type,
                                           const char* dependency_name) {
  const TypeInfo dependent(dependent_type, dependent_name);
  const TypeInfo dependency(dependency_type, dependency_name);
  registered_dependencies_.emplace_back(dependent, dependency);
}

void DependencyChecker::SatisfyDependency(TypeId dep) {
  satisfied_dependencies_.insert(dep);
}

bool DependencyChecker::IsDependencySatisfied(TypeId dep) const {
  return satisfied_dependencies_.count(dep) > 0;
}

void DependencyChecker::CheckAllDependencies() {
  bool ok = true;
  for (const auto& dep : registered_dependencies_) {
    if (!IsDependencySatisfied(dep.dependency_type.type)) {
      ok = false;
      LOG(ERROR) << dep.dependent_type.name << " has missing dependency "
                 << dep.dependency_type.name;
    }
  }
  if (!ok) {
    LOG(DFATAL) << "Must have all dependencies!";
  }
}

}  // namespace lull
