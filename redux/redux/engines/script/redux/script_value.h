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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_VALUE_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_VALUE_H_

#include <memory>

#include "redux/modules/base/typeid.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_convert.h"

namespace redux {

class ScriptValue {
 public:
  ScriptValue() = default;

  template <typename T>
  ScriptValue(T&& value);

  template <typename T>
  void Set(T&& value);

  template <typename T>
  bool Is() const;

  template <typename T>
  T* Get();

  template <typename T>
  const T* Get() const;

  template <typename T>
  bool GetAs(T* out) const;

  bool IsNil() const;

  TypeId GetTypeId() const;

  explicit operator bool() const;

 private:
  std::shared_ptr<Var> var_ptr_;
};

template <typename T>
ScriptValue::ScriptValue(T&& value) {
  Set(std::forward<T>(value));
}

template <typename T>
void ScriptValue::Set(T&& value) {
  if constexpr (std::is_same_v<typename std::decay<T>::type, ScriptValue>) {
    var_ptr_ = value.var_ptr_;
  } else {
    var_ptr_ = std::make_shared<Var>();
    const bool ok = ToVar(value, var_ptr_.get());
    CHECK(ok);
  }
}

template <typename T>
bool ScriptValue::Is() const {
  return var_ptr_ ? var_ptr_->Is<T>() : false;
}

template <typename T>
T* ScriptValue::Get() {
  if constexpr (std::is_same_v<T, Var>) {
    return var_ptr_ ? var_ptr_.get() : nullptr;
  } else {
    return var_ptr_ ? var_ptr_->Get<T>() : nullptr;
  }
}

template <typename T>
const T* ScriptValue::Get() const {
  if constexpr (std::is_same_v<T, Var>) {
    return var_ptr_ ? var_ptr_.get() : nullptr;
  } else {
    return var_ptr_ ? var_ptr_->Get<T>() : nullptr;
  }
}

template <typename T>
bool ScriptValue::GetAs(T* out) const {
  if (var_ptr_) {
    return FromVar(*var_ptr_, out);
  }
  *out = T{};
  return false;
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ScriptValue);

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_VALUE_H_
