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

#include "redux/modules/var/var.h"

#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {

Var::Var() {
  static_assert(sizeof(std::string) <= kStoreSize);
  static_assert(sizeof(VarArray) <= kStoreSize);
  static_assert(sizeof(VarTable) <= kStoreSize);
}

Var::~Var() {
  Destroy();
  Free();
}

Var::Var(const Var& rhs) { SetVar(rhs); }

Var::Var(Var&& rhs) { SetVar(std::move(rhs)); }

Var& Var::operator=(const Var& rhs) {
  SetVar(rhs);
  return *this;
}

Var& Var::operator=(Var&& rhs) {
  SetVar(std::move(rhs));
  return *this;
}

void Var::SetVar(const Var& rhs) {
  Destroy();
  if (!rhs.Empty()) {
    Alloc(rhs.capacity_);
    type_ = rhs.type_;
    handler_ = rhs.handler_;
    handler_(kCopy, GetDataPtr(), rhs.GetDataPtr(), nullptr);
  }
}

void Var::SetVar(Var&& rhs) {
  Destroy();
  if (!rhs.Empty()) {
    if (IsSmallData() || rhs.IsSmallData()) {
      Alloc(rhs.capacity_);
      type_ = rhs.type_;
      handler_ = rhs.handler_;
      handler_(kMove, GetDataPtr(), nullptr, rhs.GetDataPtr());
    } else {
      // we can swap buffers
      std::swap(type_, rhs.type_);
      std::swap(handler_, rhs.handler_);
      std::swap(capacity_, rhs.capacity_);
      std::swap(heap_data_, rhs.heap_data_);
    }
  }
}

void Var::Destroy() {
  if (handler_) {
    handler_(kDestroy, nullptr, nullptr, GetDataPtr());
    handler_ = nullptr;
    type_ = TypeId();
  }
}

void Var::Alloc(std::size_t size) {
  Destroy();
  if (size <= capacity_) {
    // We already have enough space.
    return;
  }
  Free();
  if (size > kStoreSize) {
    heap_data_ = new std::byte[size];
    capacity_ = size;
  }
}

void Var::Free() {
  CHECK(handler_ == nullptr) << "Must Destroy() before Free()ing.";
  Destroy();
  if (!IsSmallData()) {
    delete[] heap_data_;
  }
  capacity_ = kStoreSize;
}

std::size_t Var::Count() const {
  if (const auto* array = Get<VarArray>()) {
    return array->Count();
  } else if (const auto* table = Get<VarTable>()) {
    return table->Count();
  } else {
    return Empty() ? 0 : 1;
  }
}

Var& Var::operator[](std::size_t index) {
  if (auto array = Get<VarArray>()) {
    return (*array)[index];
  } else {
    return *this;
  }
}

const Var& Var::operator[](std::size_t index) const {
  if (auto array = Get<VarArray>()) {
    return (*array)[index];
  } else {
    return *this;
  }
}

Var& Var::operator[](HashValue key) {
  if (auto table = Get<VarTable>()) {
    return (*table)[key];
  } else {
    return *this;
  }
}

const Var& Var::operator[](HashValue key) const {
  if (auto table = Get<VarTable>()) {
    return (*table)[key];
  } else {
    return *this;
  }
}

bool Var::IsSmallData() const { return capacity_ <= kStoreSize; }

std::byte* Var::GetDataPtr() {
  return IsSmallData() ? &small_data_[0] : heap_data_;
}

const std::byte* Var::GetDataPtr() const {
  return IsSmallData() ? &small_data_[0] : heap_data_;
}

}  // namespace redux
