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

#ifndef REDUX_MODULES_ECS_COMPONENT_SERIALIZER_H_
#define REDUX_MODULES_ECS_COMPONENT_SERIALIZER_H_

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "redux/engines/script/redux/script_env.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_convert.h"
#include "redux/modules/var/var_table.h"

namespace redux {

// Converts Blueprint component Vars into concrete "component" object instances.
class ComponentSerializer {
 public:
  ComponentSerializer(const Var& var, ScriptEnv* env)
      : root_(&var), env_(env) {}

  // Marks the start of a new object/dictionary with the given key.
  void Begin(HashValue key) {
    if (stack_.empty()) {
      stack_.push_back(root_);
    } else {
      const Var& var = GetElement(key);
      stack_.push_back(&var);
    }
  }

  // Marks the end of the current object/dictionary being serialized.
  void End() { stack_.pop_back(); }

  // Serializes the value with the given key. Will attempt to perform some
  // useful conversions (e.g. float to int) or, in the case of ScriptValues,
  // will perform an evaluation using a ScriptEnv.
  template <typename T>
  void operator()(T& value, HashValue key) {
    const Var& var = GetElement(key);
    status_ = ReadVar(key, var, value);
  }

  // Indicates that calls to operator() will modify/overwrite the `value`
  // argument.
  constexpr bool IsDestructive() { return true; }

  // Returns the current status of the serialization.
  absl::Status Status() const { return status_; }

 protected:
  const Var& GetElement(HashValue key) {
    const Var& current_object = *stack_.back();
    const Var& element = current_object[key];
    return element;
  }

  template <typename T>
  absl::Status ReadVar(HashValue key, const Var& var, T& out) {
    if (!status_.ok()) {
      return status_;
    } else if (var.Empty()) {
      return absl::OkStatus();
    } else if (const ScriptValue* val = var.Get<ScriptValue>()) {
      return ReadScriptValue(key, *val, out);
    } else if (const VarArray* arr = var.Get<VarArray>()) {
      return ReadArray(key, *arr, out);
    } else if (const VarTable* table = var.Get<VarTable>()) {
      return ReadTable(key, *table, out);
    } else {
      return ReadPrimitive(key, var, out);
    }
  }

  template <typename T>
  absl::Status ReadTable(HashValue key, const VarTable& table, T& out) {
    if constexpr (detail::IsSerializable<T, ComponentSerializer>) {
      Var root(table);
      ComponentSerializer serializer(root, env_);
      Serialize(serializer, out);
      return serializer.Status();
    }

    return absl::InvalidArgumentError(
        absl::StrCat("Expected object at key: ", key.get()));
  }

  template <typename T>
  absl::Status ReadArray(HashValue key, const VarArray& arr, T& out) {
    if constexpr (IsVectorV<T>) {
      out.clear();
      for (const auto& var : arr) {
        typename T::value_type value;
        status_ = ReadVar(key, var, value);
        if (status_.ok()) {
          out.emplace_back(std::move(value));
        } else {
          return status_;
        }
      }
      return absl::OkStatus();
    }

    return absl::InvalidArgumentError(
        absl::StrCat("Expected array at key: ", key.get()));
  }

  template <typename T>
  absl::Status ReadScriptValue(HashValue key, const ScriptValue& val, T& out) {
    const ScriptValue result = env_->Eval(val);
    const Var* result_var = result.Get<Var>();
    if (result_var == nullptr) {
      return absl::InvalidArgumentError(
          absl::StrCat("Unable to resolve script at key: ", key.get()));
    }
    return ReadVar(key, *result_var, out);
  }

  template <typename T>
  absl::Status ReadPrimitive(HashValue key, const Var& var, T& value) {
    const bool converted = FromVar(var, &value);
    if (!converted) {
      return absl::InvalidArgumentError(
          absl::StrCat("Unable to read value at key: ", key.get()));
    }
    return absl::OkStatus();
  }

  const Var* root_ = nullptr;
  ScriptEnv* env_ = nullptr;
  std::vector<const Var*> stack_;
  absl::Status status_ = absl::OkStatus();
};

}  // namespace redux

#endif  // REDUX_MODULES_ECS_COMPONENT_SERIALIZER_H_
