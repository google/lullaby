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

#include "lullaby/modules/ecs/blueprint_writer.h"

#include "flatbuffers/flatbuffers.h"
#include "lullaby/modules/ecs/blueprint_type.h"
#include "lullaby/util/inward_buffer.h"

namespace lull {

BlueprintWriter::BlueprintWriter(ComponentHandlers* component_handlers)
    : component_handlers_(component_handlers) {}

flatbuffers::DetachedBuffer BlueprintWriter::WriteBlueprintTree(
    BlueprintTree* blueprint_tree) {
  WriteBlueprintTreeImpl(blueprint_tree);
  return blueprint_builder_.Finish();
}

void BlueprintWriter::WriteBlueprintTreeImpl(BlueprintTree* blueprint_tree) {
  if (!blueprint_tree->Children()->empty()) {
    blueprint_builder_.StartChildren();
    for (BlueprintTree& child : *blueprint_tree->Children()) {
      WriteBlueprintTreeImpl(&child);
      blueprint_builder_.FinishChild();
    }
    blueprint_builder_.FinishChildren();
  }
  WriteBlueprint(blueprint_tree);
}

void BlueprintWriter::WriteBlueprint(Blueprint* blueprint) {
  Variant def_t_variant;
  InwardBuffer def_buffer(256);
  blueprint->ForEachComponent([this, &def_t_variant,
                               &def_buffer](const Blueprint& bp) {
    BlueprintType::DefType def_type = bp.GetLegacyDefType();
    if (!component_handlers_->IsRegistered(def_type)) {
      LOG(DFATAL) << "Unknown type for writing: " << def_type;
      return;
    }

    // We need to reserialize the ComponentDef individually so that we are sure
    // we copy it completely and contiguously.
    component_handlers_->ReadFromFlatbuffer(def_type, &def_t_variant,
                                            bp.GetLegacyDefData());
    void* start = component_handlers_->WriteToFlatbuffer(
        def_type, &def_t_variant, &def_buffer);
    blueprint_builder_.AddComponent(
        def_type, {static_cast<uint8_t*>(start), def_buffer.BackSize()});

    def_t_variant.Clear();
    def_buffer.Reset();
  });
}

}  // namespace lull
