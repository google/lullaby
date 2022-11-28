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

#include <vector>

#include "absl/status/status.h"
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
    if (var.Empty()) {
      return;
    } else if (const ScriptValue* val = var.Get<ScriptValue>()) {
      const ScriptValue result = env_->Eval(*val);
      const Var* result_var = result.Get<Var>();
      CHECK(result_var != nullptr);
      if (!FromVar(*result_var, &value)) {
        status_ = absl::InvalidArgumentError(
            absl::StrCat("Unknown key: ", key.get()));
      }
    } else {
      if (!FromVar(var, &value)) {
        status_ = absl::InvalidArgumentError(
            absl::StrCat("Unknown key: ", key.get()));
      }
    }
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

  const Var* root_ = nullptr;
  ScriptEnv* env_ = nullptr;
  std::vector<const Var*> stack_;
  absl::Status status_ = absl::OkStatus();
};

}  // namespace redux

#endif  // REDUX_MODULES_ECS_COMPONENT_SERIALIZER_H_
