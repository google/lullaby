/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_VALUE_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_VALUE_H_

#include "lullaby/util/common_types.h"
#include "lullaby/util/variant.h"

namespace lull {

// Represents a "value-type" in LullScript, eg. int, float, vec3, list, map,
// AstNode, etc.
//
// It is basically an intrusive shared pointer to a lull::Variant.  This allows
// the ScriptValue to be copied/shared with no overhead.  However, one must be
// aware that changing a ScriptValue will result in changing all other
// ScriptValues referencing it.
//
// In order to create a new ScriptValue instance, you must use either the
// Create or Clone static function.  This helps easily identify callsites where
// a new ScriptValue is being created vs. just a new value being set on the
// ScriptValue.
//
// IMPORTANT: Because we are using reference-counting, it is possible to create
// cycles (eg. map[key] = map).  The current expectation with LullScript is that
// it is for simple "one-off" scripts whose lifetimes are closely associated
// with individual Entities with reasonably small memory requirements (ie. a
// dozen or so variables at most).  As such, the expectation is on the script
// writer to manually manage such cyclical references if they occur.
class ScriptValue {
 public:
  // Creates a ScriptValue with the specified internal value.
  template <typename T>
  static ScriptValue Create(T&& t);
  static ScriptValue CreateFromVariant(Variant var);

  // Clones the provided ScriptValue.
  static ScriptValue Clone(ScriptValue rhs);

  ScriptValue() {}
  ~ScriptValue() { Reset(); }

  ScriptValue(ScriptValue&& rhs);
  ScriptValue(const ScriptValue& rhs);

  ScriptValue& operator=(const ScriptValue& rhs);
  ScriptValue& operator=(ScriptValue&& rhs);

  // Returns true if no value is currently stored.
  bool IsNil() const;

  // Clears the stored value.
  void Reset();

  // Returns the TypeId of the stored value.
  TypeId GetTypeId() const;

  // Returns true if the stored value is of type T.
  template <typename T>
  bool Is() const;

  // Sets the stored value to |t|.
  template <typename T>
  void Set(T&& t);

  // Gets a pointer to the stored value if of type T, or nullptr otherwise.
  template <typename T>
  T* Get();

  // Gets a pointer to the stored value if of type T, or nullptr otherwise.
  template <typename T>
  const T* Get() const;

  // Utility type to enable/disable the NumericCast function below.
  template <typename T>
  using EnableIfNumeric =
      typename std::enable_if<std::is_arithmetic<T>::value, T>;

  // Similar to Get(), but attempts to perform a static_cast on the underlying
  // type.  Both T and the stored type must be a numeric value (ie. int, float,
  // etc.).  If neither type is numeric, returns an empty value.
  template <typename T>
  auto NumericCast() const -> Optional<typename EnableIfNumeric<T>::type>;

  // Returns a pointer to the underlying variant (if available).
  const Variant* GetVariant() const;

  // Sets a value directly from a variant rather than a value-type.
  void SetFromVariant(Variant&& variant);
  void SetFromVariant(const Variant& variant);

 private:
  struct Impl {
    template <typename T>
    explicit Impl(T&& t) : count(0) {
      var = std::forward<T>(t);
    }
    Variant var;
    int count;
  };

  explicit ScriptValue(Impl* impl) { Acquire(impl); }

  void Acquire(Impl* other);
  void Release();

  Impl* impl_ = nullptr;
};

template <typename T>
ScriptValue ScriptValue::Create(T&& t) {
  return ScriptValue(new Impl(std::forward<T>(t)));
}

inline ScriptValue ScriptValue::CreateFromVariant(Variant var) {
  ScriptValue value = Create(0);
  value.SetFromVariant(std::move(var));
  return value;
}

inline ScriptValue ScriptValue::Clone(ScriptValue rhs) {
  return ScriptValue(new Impl(rhs.impl_->var));
}

template <typename T>
bool ScriptValue::Is() const {
  return impl_ ? impl_->var.GetTypeId() == lull::GetTypeId<T>() : false;
}

template <typename T>
void ScriptValue::Set(T&& t) {
  if (impl_) {
    impl_->var = std::forward<T>(t);
  }
}

template <typename T>
T* ScriptValue::Get() {
  return impl_ ? impl_->var.Get<T>() : nullptr;
}

template <typename T>
const T* ScriptValue::Get() const {
  return impl_ ? impl_->var.Get<T>() : nullptr;
}

template <typename T>
auto ScriptValue::NumericCast() const
    -> Optional<typename EnableIfNumeric<T>::type> {
  if (auto ptr = Get<int32_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<float>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint32_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<int64_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint64_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<double>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<int16_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint16_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<int8_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint8_t>()) return static_cast<T>(*ptr);
  return NullOpt;
}

inline bool ScriptValue::IsNil() const {
  return impl_ == nullptr || impl_->var.GetTypeId() == TypeId();
}

inline TypeId ScriptValue::GetTypeId() const {
  return impl_ ? impl_->var.GetTypeId() : 0;
}

inline const Variant* ScriptValue::GetVariant() const {
  return impl_ ? &impl_->var : nullptr;
}

inline void ScriptValue::SetFromVariant(Variant&& variant) {
  if (impl_) {
    impl_->var = std::move(variant);
  }
}

inline void ScriptValue::SetFromVariant(const Variant& variant) {
  if (impl_) {
    impl_->var = variant;
  }
}

inline void ScriptValue::Reset() { Release(); }

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScriptValue);

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_VALUE_H_
