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

#include "redux/modules/var/var_table.h"

namespace redux {

void VarTable::Clear() { data_.clear(); }

void VarTable::Swap(VarTable& rhs) { data_.swap(rhs.data_); }

size_t VarTable::Count() const { return data_.size(); }

void VarTable::Erase(HashValue key) { data_.erase(key); }

bool VarTable::Contains(HashValue key) const { return data_.contains(key); }

Var* VarTable::TryFind(HashValue key) {
  auto iter = data_.find(key);
  return iter != data_.end() ? &iter->second : nullptr;
}

const Var* VarTable::TryFind(HashValue key) const {
  auto iter = data_.find(key);
  return iter != data_.end() ? &iter->second : nullptr;
}

Var& VarTable::operator[](HashValue key) { return data_[key]; }

const Var& VarTable::operator[](HashValue key) const {
  static const Var kEmpty;
  auto iter = data_.find(key);
  return iter != data_.end() ? iter->second : kEmpty;
}

}  // namespace redux
