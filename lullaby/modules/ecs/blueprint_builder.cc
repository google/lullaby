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

#include "lullaby/modules/ecs/blueprint_builder.h"

#include "flatbuffers/flatbuffers.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace detail {

const char* const BlueprintBuilder::kBlueprintFileIdentifier = "BLPT";

void BlueprintBuilder::AddComponent(string_view def_type, Span<uint8_t> def) {
  AddComponent(Hash(def_type), def);
}

void BlueprintBuilder::AddComponent(HashValue def_type, Span<uint8_t> def) {
  auto def_offset = fbb_.CreateVector(def.data(), def.size());
  BlueprintComponentDefBuilder component(fbb_);
  component.add_type(def_type);
  component.add_def(def_offset);
  components_vector_.emplace_back(component.Finish());
}

void BlueprintBuilder::StartChildren() {
  children_vector_stack_.emplace_back();
}

bool BlueprintBuilder::FinishChild() {
  if (children_vector_stack_.empty()) {
    LOG(ERROR) << "No children array to add to.";
    return false;
  }
  children_vector_stack_.back().push_back(FinishEntity());
  return true;
}

bool BlueprintBuilder::FinishChildren() {
  if (children_vector_stack_.empty()) {
    LOG(ERROR) << "No children array to finish.";
    return false;
  }
  children_offsets_ = fbb_.CreateVector(children_vector_stack_.back());
  children_vector_stack_.pop_back();
  return true;
}

BlueprintBuilder::EntityOffset BlueprintBuilder::FinishEntity() {
  auto components_vector = fbb_.CreateVector(components_vector_);
  components_vector_.clear();

  BlueprintDefBuilder entity(fbb_);
  entity.add_components(components_vector);
  entity.add_children(children_offsets_);
  children_offsets_ = 0;
  return entity.Finish();
}

flatbuffers::DetachedBuffer BlueprintBuilder::Finish(const char* identifier) {
  if (!children_vector_stack_.empty()) {
    LOG(ERROR) << "Unfinished children array.";
    return flatbuffers::DetachedBuffer();
  }
  fbb_.Finish(FinishEntity(), identifier);
  return fbb_.Release();
}

}  // namespace detail
}  // namespace lull
