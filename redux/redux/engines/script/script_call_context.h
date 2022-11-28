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

#ifndef REDUX_ENGINES_SCRIPT_SCRIPT_CALL_CONTEXT_H_
#define REDUX_ENGINES_SCRIPT_SCRIPT_CALL_CONTEXT_H_

#include "absl/status/status.h"
#include "redux/modules/var/var.h"

namespace redux {

// The context used to invoke a native function using arguments extracted
// from a script and returning a value to the script. See
// call_native_function.h for more information.
class ScriptCallContext {
 public:
  template <typename T>
  absl::StatusCode ArgFromCpp(size_t index, T* out);

  template <typename T>
  absl::StatusCode ReturnFromCpp(const T& value);

 protected:
  Var* GetArg(size_t index);
  void SetReturnValue(Var var);
};

template <typename T>
absl::StatusCode ScriptCallContext::ArgFromCpp(size_t index, T* out) {
  Var* var = GetArg(index);
  if (var == nullptr) {
    return absl::StatusCode::kOutOfRange;
  }

  const bool ok = FromVar(*var, out);
  return ok ? absl::StatusCode::kOk : absl::StatusCode::kInvalidArgument;
}

template <typename T>
absl::StatusCode ScriptCallContext::ReturnFromCpp(const T& value) {
  Var var;
  const bool ok = ToVar(value, &var);
  if (ok) {
    SetReturnValue(std::move(var));
  }
  return ok ? absl::StatusCode::kOk : absl::StatusCode::kInvalidArgument;
}

}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_SCRIPT_CALL_CONTEXT_H_
