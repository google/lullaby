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

#include "lullaby/modules/ecs/blueprint_reader.h"

#include "flatbuffers/flatbuffers.h"

namespace lull {

BlueprintReader::BlueprintReader(ComponentHandlers* component_handlers)
    : component_handlers_(component_handlers) {}

Optional<BlueprintTree> BlueprintReader::ReadFlatbuffer(
    Span<uint8_t> flatbuffer) {
  flatbuffers::Verifier verifier(flatbuffer.data(), flatbuffer.size());
  if (verifier.VerifyBuffer<BlueprintDef>()) {
    const BlueprintDef* blueprint_def =
        flatbuffers::GetRoot<BlueprintDef>(flatbuffer.data());
    return ReadBlueprint(blueprint_def);
  }
  LOG(WARNING) << "Verification failed: Blueprint file contained invalid data.";
  return NullOpt;
}

BlueprintTree BlueprintReader::ReadBlueprint(
    const BlueprintDef* blueprint_def) {
  // Capture just the handlers pointer, which should outlive the returned
  // BlueprintTree.
  auto* component_handlers = component_handlers_;
  const auto* components = blueprint_def->components();
  size_t count = components ? components->size() : 0;

  // Returns a type+flatbuffer::Table pair for a given index.  The Blueprint
  // uses this function (along with the total size) to iterate over
  // components without having to know about the internal details/structure
  // of the container containing those components.
  auto component_accessor = [component_handlers, components](size_t index) {
    std::pair<HashValue, const flatbuffers::Table*> result = {0, nullptr};
    if (components == nullptr || index < 0 || index >= components->size()) {
      return result;
    }

    const BlueprintComponentDef* component =
        components->Get(static_cast<int>(index));
    const flatbuffers::Vector<uint8_t>* def = component->def();
    if (def == nullptr) {
      return result;
    }

    HashValue def_type = component->type();
    if (!component_handlers->IsRegistered(def_type)) {
      LOG(DFATAL) << "No verifier for type: " << def_type;
      return result;
    } else if (!component_handlers->Verify(def_type,
                                              {def->Data(), def->size()})) {
      LOG(WARNING) << "Verification failed: Blueprint file contained invalid "
                      "data for type: "
                   << def_type;
      return result;
    }

    result.first = def_type;
    result.second = flatbuffers::GetRoot<const flatbuffers::Table>(def->Data());
    return result;
  };

  const auto* children = blueprint_def->children();
  std::list<BlueprintTree> children_blueprints;
  if (children) {
    for (int i = 0; i < static_cast<int>(children->size()); ++i) {
      children_blueprints.emplace_back(ReadBlueprint(children->Get(i)));
    }
  }

  return BlueprintTree(std::move(component_accessor), count,
                       std::move(children_blueprints));
}

}  // namespace lull
