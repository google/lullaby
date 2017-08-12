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

#ifndef LULLABY_MODULES_ECS_BLUEPRINT_TREE_H_
#define LULLABY_MODULES_ECS_BLUEPRINT_TREE_H_

#include <list>

#include "lullaby/modules/ecs/blueprint.h"

namespace lull {

// BlueprintTree is a blueprint which may have children.
class BlueprintTree : public Blueprint {
 public:
  // Wrap Blueprint constructors.
  BlueprintTree() : Blueprint() {}

  explicit BlueprintTree(size_t buffer_size) : Blueprint(buffer_size) {}

  template <typename T>
  explicit BlueprintTree(const T* ptr) : Blueprint(ptr) {}

  BlueprintTree(ArrayAccessorFn accessor_fn, size_t count,
                std::list<BlueprintTree> children)
      : Blueprint(std::move(accessor_fn), count),
        children_(std::move(children)) {}

  BlueprintTree* NewChild() {
    children_.emplace_back();
    return &children_.back();
  }

  std::list<BlueprintTree>* Children() { return &children_; }

 private:
  std::list<BlueprintTree> children_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_ECS_BLUEPRINT_TREE_H_
