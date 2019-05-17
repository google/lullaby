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

#include "lullaby/tests/util/fake_flatbuffer_union.h"

namespace lull {
namespace testing {

const FakeFlatbufferUnion* FakeFlatbufferUnion::s_fake_union_ = nullptr;
const char* FakeFlatbufferUnion::s_default_type_names_[2] = {"NONE", nullptr};

void FakeFlatbufferUnion::SetActive(const FakeFlatbufferUnion* fake_union) {
  CHECK(!s_fake_union_);
  CHECK(fake_union);

  fake_union->OnActivate();
  s_fake_union_ = fake_union;
}

void FakeFlatbufferUnion::ClearActive() {
  if (s_fake_union_) {
    s_fake_union_->OnDeactivate();
  }
  s_fake_union_ = nullptr;
}

const char** FakeFlatbufferUnion::GetActiveTypeNames() {
  if (s_fake_union_) {
    return s_fake_union_->GetTypeNames();
  }
  return s_default_type_names_;
}

TypeId FakeFlatbufferUnion::GetTypeId(DefId type) const {
  const bool in_range = (type < type_ids_.size());

  return in_range ? type_ids_[type] : 0;
}

FakeFlatbufferUnion::DefId FakeFlatbufferUnion::GetDefId(TypeId type) const {
  const auto& reverse_type_lookup = reverse_type_map_.find(type);
  const bool found = (reverse_type_lookup != reverse_type_map_.end());

  return found ? reverse_type_lookup->second : 0;
}

void FakeFlatbufferUnion::RegisterType(const char* fully_qualified_type_name,
                                       const TypeId type_id,
                                       DefId* type_to_def_mapping) {
  // Ensure overflow doesn't happen.
  CHECK(type_ids_.size() <= std::numeric_limits<DefId>::max());

  // The final type name should be unqualified, so strip off any namespaces
  // (separated by . for flatbuffers types).
  // If the original type name was already unqualified just use it instead.
  const char* unqualified_type_name = strrchr(fully_qualified_type_name, '.');
  const char* type_name = unqualified_type_name ? (unqualified_type_name + 1)
                                                : fully_qualified_type_name;
  reverse_type_map_.emplace(type_id, static_cast<DefId>(type_names_.size()));
  type_ids_.emplace_back(type_id);
  type_names_.emplace_back(type_name);
  if (type_to_def_mapping) {
    type_to_def_mappings_.emplace_back(type_to_def_mapping);
  }
}

void FakeFlatbufferUnion::OnActivate() const {
  DefId id_counter = 1;  // 0 is taken by "NONE", so the counter is 1-based.
  for (DefId* mapping : type_to_def_mappings_) {
    *mapping = id_counter++;
  }
}

void FakeFlatbufferUnion::OnDeactivate() const {
  for (DefId* mapping : type_to_def_mappings_) {
    *mapping = 0;
  }
}

}  // namespace testing
}  // namespace lull
