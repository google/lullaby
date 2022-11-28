/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/modules/base/registry.h"

namespace redux {

Registry::~Registry() {
  // Destroy objects in reverse order of registration.
  for (auto iter = objects_.rbegin(); iter != objects_.rend(); ++iter) {
    // Destroy the object before removing it from the ObjectTable in case
    // the object is referenced in the registry in its destructor.
    iter->second.reset();
    table_.erase(iter->first);
  }
  table_.clear();
  objects_.clear();
}

void Registry::Initialize() {
  bool ok = true;
  for (const auto& dep : registered_dependencies_) {
    if (!satisfied_dependencies_.contains(dep.dependency_target_type.type)) {
      ok = false;
      LOG(ERROR) << dep.dependent_type.name << " has missing dependency "
                 << dep.dependency_target_type.name;
    }
  }
  CHECK(ok) << "Missing dependencies!";

  initialization_dependencies_.Traverse([=](const TypeId& type) {
    auto iter = initializers_.find(type);
    if (iter != initializers_.end()) {
      iter->second();
      initializers_.erase(iter);
    }
  });
  for (auto& initializer : initializers_) {
    initializer.second();
  }
  initializers_.clear();
}
}  // namespace redux
