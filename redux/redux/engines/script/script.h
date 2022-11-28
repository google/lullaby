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

#ifndef REDUX_ENGINES_SCRIPT_SCRIPT_H_
#define REDUX_ENGINES_SCRIPT_SCRIPT_H_

#include <optional>

#include "redux/modules/var/var.h"
#include "redux/modules/var/var_convert.h"

namespace redux {

// A "runnable" script as loaded by the ScriptEngine.
class Script {
 public:
  virtual ~Script() = default;

  // Runs the loaded script.
  Var Run();

  // Sets the value of a variable in the script.
  template <typename T>
  void SetValue(std::string_view name, const T& value) {
    DoSetValue(name, value);
  }

  // Retrieves the value of a variable in the script. Returns std::nullopt if
  // no such value exists.
  template <typename T>
  std::optional<T> GetValue(std::string_view name) {
    const Var var = DoGetValue(name);
    T value;
    if (FromVar(var, &value)) {
      return value;
    } else {
      return std::nullopt;
    }
  }

 protected:
  Script() = default;

  void DoSetValue(std::string_view name, Var value);
  Var DoGetValue(std::string_view name);
};

}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_SCRIPT_H_
