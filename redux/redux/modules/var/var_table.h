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

#ifndef REDUX_MODULES_VAR_VAR_TABLE_H_
#define REDUX_MODULES_VAR_VAR_TABLE_H_

#include "absl/container/flat_hash_map.h"
#include "redux/modules/var/var.h"

namespace redux {

// An unordered dictionary (flat_hash_map) of HashValue to Var objects.
class VarTable {
 public:
  VarTable() = default;

  // Clears the table of all data.
  void Clear();

  // Swaps this table with `rhs`.
  void Swap(VarTable& rhs);

  // Returns the number of Vars stored in the table.
  size_t Count() const;

  // Associates the `value` with the given `key` in the table (overwriting
  // existing values, if any).
  template <typename T>
  void Insert(HashValue key, T&& value);

  // Erases the value associated with the given `key` from the table.
  void Erase(HashValue key);

  // Returns whether the table contains a Var with the specified `key`.
  bool Contains(HashValue key) const;

  // Returns the Var associated with the `key` if it exists, null otherwise.
  Var* TryFind(HashValue key);

  // Returns the Var associated with the `key` if it exists, null otherwise.
  const Var* TryFind(HashValue key) const;

  // Returns a reference to the Var associated with the `key`, creating an
  // empty Var if no such key exists.
  Var& operator[](HashValue key);

  // Returns a reference to the Var associated with the `key`, returning an
  // empty Var if no such key exists.
  const Var& operator[](HashValue key) const;

  // Same as operator[](HashValue), but will do the hashing automatically. Only
  // works with compile-time strings.
  template <std::size_t N>
  Var& operator[](const char (&str)[N]) {
    return (*this)[HashValue(detail::ConstHash(str))];
  }
  template <std::size_t N>
  const Var& operator[](const char (&str)[N]) const {
    return (*this)[HashValue(detail::ConstHash(str))];
  }

  // Returns the value of the Var associated with the `key` if it is of type
  // `T`, otherwise returns the `default_value`.
  template <typename T>
  T ValueOr(HashValue key, T&& default_value) const;

  // Iterator API to support range based for loops.
  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
  auto begin() const { return data_.begin(); }
  auto end() const { return data_.end(); }

 private:
  absl::flat_hash_map<HashValue, Var> data_;
};

template <typename T>
void VarTable::Insert(HashValue key, T&& value) {
  data_.emplace(key, std::forward<T>(value));
}

template <typename T>
T VarTable::ValueOr(HashValue key, T&& default_value) const {
  if (auto iter = data_.find(key); iter != data_.end()) {
    return iter->second.ValueOr(std::forward<T>(default_value));
  } else {
    return std::forward<T>(default_value);
  }
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::VarTable);

#endif  // REDUX_MODULES_VAR_VAR_TABLE_H_
