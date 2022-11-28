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

#ifndef REDUX_MODULES_VAR_VAR_ARRAY_H_
#define REDUX_MODULES_VAR_VAR_ARRAY_H_

#include <vector>

#include "redux/modules/base/logging.h"
#include "redux/modules/var/var.h"

namespace redux {

// A dynamic array (vector) of Var objects.
class VarArray {
 public:
  VarArray() = default;

  // Clears the array of all data.
  void Clear();

  // Resizes the array to the specified `size`.
  void Resize(size_t size);

  // Reserves the underlying vector to the specified `capacity`.
  void Reserve(size_t capacity);

  // Swaps this array with `rhs`.
  void Swap(VarArray& rhs);

  // Returns the number of Vars stored in the array.
  size_t Count() const;

  // Returns the current capacity of the underlying vector.
  size_t Capacity() const;

  // Adds the `value` to the end of the array.
  template <typename T>
  void PushBack(T&& value);

  // Removes the value at the end of the array.
  void PopBack();

  // Inserts the `value` at the specified `index` in the array.
  template <typename T>
  void Insert(size_t index, T&& value);

  // Removes the value from the specified `index` in the array.
  void Erase(size_t index);

  // Returns the n-th element of the array as specified by `index`.
  Var& operator[](size_t index);

  // Returns the n-th element of the array as specified by `index`.
  const Var& operator[](size_t index) const;

  // Returns the n-th element of the array as specified by `index`.
  Var& At(size_t index);

  // Returns the n-th element of the array as specified by `index`.
  const Var& At(size_t index) const;

  // Iterator API to support range based for loops.
  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
  auto begin() const { return data_.begin(); }
  auto end() const { return data_.end(); }

 private:
  std::vector<Var> data_;
};

template <typename T>
void VarArray::PushBack(T&& value) {
  data_.emplace_back(std::forward<T>(value));
}

template <typename T>
void VarArray::Insert(size_t index, T&& value) {
  CHECK_LE(index, data_.size());
  data_.insert(data_.begin() + index, std::forward<T>(value));
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::VarArray);

#endif  // REDUX_MODULES_VAR_VAR_ARRAY_H_
