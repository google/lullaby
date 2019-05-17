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

#ifndef LULLABY_MODULES_ECS_BLUEPRINT_WRITER_H_
#define LULLABY_MODULES_ECS_BLUEPRINT_WRITER_H_

#include "flatbuffers/flatbuffers.h"
#include "lullaby/modules/ecs/blueprint.h"
#include "lullaby/modules/ecs/blueprint_builder.h"
#include "lullaby/modules/ecs/blueprint_tree.h"
#include "lullaby/modules/ecs/component_handlers.h"

namespace lull {

// Creates BlueprintDef flatbuffer binaries from BlueprintTrees.
class BlueprintWriter {
 public:
  BlueprintWriter(ComponentHandlers* component_handlers);

  flatbuffers::DetachedBuffer WriteBlueprintTree(BlueprintTree* blueprint_tree);

 private:
  void WriteBlueprintTreeImpl(BlueprintTree* blueprint_tree);
  void WriteBlueprint(Blueprint* blueprint);

  ComponentHandlers* component_handlers_;
  detail::BlueprintBuilder blueprint_builder_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_ECS_BLUEPRINT_WRITER_H_
