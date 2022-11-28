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

#include "redux/engines/script/redux/script_stack.h"

namespace redux {

ScriptStack::ScriptStack() { PushScope(); }

void ScriptStack::SetValue(HashValue id, ScriptValue value) {
  auto pair = lookup_.emplace(id, IndexArray());
  auto iter = pair.first;

  IndexArray& array = iter->second;
  const bool exists = !pair.second;
  if (exists) {
    const size_t index = array.index[array.count - 1];
    values_[index].value = std::move(value);
  } else {
    CHECK(array.count < IndexArray::kMaxInstancesPerValue);

    const size_t index = values_.size();
    values_.emplace_back(std::move(value), &(*iter));

    array.index[array.count] = index;
    array.count++;
  }
}

void ScriptStack::LetValue(HashValue id, ScriptValue value) {
  auto iter = lookup_.emplace(id, IndexArray()).first;

  IndexArray& array = iter->second;
  const bool exists_in_current_scope =
      array.count > 0 && array.index[array.count - 1] > scopes_.back();
  if (exists_in_current_scope) {
    const size_t index = array.index[array.count - 1];
    values_[index].value = std::move(value);
  } else {
    CHECK(array.count < IndexArray::kMaxInstancesPerValue);

    const size_t index = values_.size();
    values_.emplace_back(std::move(value), &(*iter));

    array.index[array.count] = index;
    array.count++;
  }
}

ScriptValue ScriptStack::GetValue(HashValue id) {
  auto iter = lookup_.find(id);
  if (iter == lookup_.end()) {
    return {};
  }

  const auto& entry = iter->second;
  CHECK(entry.count != 0)
      << "This entry should have been removed from the lookup table.";

  const size_t index = entry.index[entry.count - 1];
  return values_[index].value;
}

void ScriptStack::PushScope() { scopes_.emplace_back(values_.size()); }

void ScriptStack::PopScope() {
  const size_t size = scopes_.back();
  while (values_.size() > size) {
    auto* lookup = values_.back().lookup_entry;

    IndexArray& array = lookup->second;
    --array.count;
    if (array.count == 0) {
      lookup_.erase(lookup->first);
    }
    values_.pop_back();
  }
  scopes_.pop_back();
}

}  // namespace redux
