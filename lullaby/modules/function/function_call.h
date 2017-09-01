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

#ifndef LULLABY_UTIL_FUNCTION_CALL_H_
#define LULLABY_UTIL_FUNCTION_CALL_H_

#include "lullaby/modules/function/variant_converter.h"
#include "lullaby/util/fixed_string.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/type_name_generator.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

namespace lull {

/// A FunctionCall bundles up the name (as a HashValue ID) of a function to
/// call, the arguments to that function (as an array of Variants), and storage
/// for the return value (also as a Variant).
///
/// The FunctionCall class can be used as a CallNativeFunction Context.  It
/// is used by the FunctionBinder to programmatically "call" registered
/// functions dynamically.
class FunctionCall {
 public:
  using Handler = std::function<void(FunctionCall*)>;

  /// Bundle function name or ID, and arguments into a FunctionCall object.
  template <typename... Args>
  static FunctionCall Create(HashValue id, Args... args);
  template <typename... Args>
  static FunctionCall Create(string_view name, Args... args);

  /// Creates a FunctionCall object with the specified name or ID.
  explicit FunctionCall(HashValue id) : id_(id) {}
  explicit FunctionCall(string_view name) : id_(Hash(name)), name_(name) {}

  /// Returns the ID of the function call (which is the Hash of the name
  /// specified in the Create function).
  HashValue GetId() const { return id_; }

  /// Returns the name of the function call.
  using Name = FixedString<64>;
  const Name& GetName() const { return name_; }

  /// Returns the Variant returned when the function was "called" by the
  /// FunctionBinder.
  const Variant& GetReturnValue() const { return return_value_; }

  /// Adds a value to the argument list.
  template <typename T>
  void AddArg(T&& value);

  /// Converts the argument at the specified index to the native CPP value type.
  /// This function is required by CallNativeFunction.
  template <typename T>
  bool ArgToCpp(string_view name, size_t arg_index, T* value) const;

  /// Stores the value as the return value of the function.  This function is
  /// required by CallNativeFunction.
  template <typename T>
  bool ReturnFromCpp(string_view name, const T& value);

  /// Checks that the number of arguments expected by the actual native function
  /// matches the number of arguments stored in this class.  This function is
  /// required by CallNativeFunction.
  bool CheckNumArgs(string_view name, size_t expected_args) const;

 private:
  static constexpr size_t kMaxArgs = 15;

  HashValue id_ = 0;
  size_t num_args_ = 0;
  Variant args_[kMaxArgs];
  Variant return_value_;
  Name name_;
};

template <typename... Args>
FunctionCall FunctionCall::Create(HashValue id, Args... args) {
  FunctionCall call(id);
  int dummy[] = {(call.AddArg(std::forward<Args>(args)), 0)...};
  (void)dummy;
  return call;
}

template <typename... Args>
FunctionCall FunctionCall::Create(string_view name, Args... args) {
  FunctionCall call(name);
  int dummy[] = {(call.AddArg(std::forward<Args>(args)), 0)...};
  (void)dummy;
  return call;
}

template <typename T>
void FunctionCall::AddArg(T&& value) {
  if (num_args_ < kMaxArgs) {
    args_[num_args_] = std::forward<T>(value);
    ++num_args_;
  } else {
    LOG(DFATAL) << "Maximum number of args exceeded.";
  }
}

template <typename T>
bool FunctionCall::ArgToCpp(string_view name, size_t arg_index,
                            T* value) const {
  const bool result = VariantConverter::FromVariant(args_[arg_index], value);
  if (!result) {
    LOG(DFATAL) << name << " expects the type of arg " << arg_index + 1
                << " to be " << TypeNameGenerator::Generate<T>();
  }
  return result;
}

template <typename T>
bool FunctionCall::ReturnFromCpp(string_view name, const T& value) {
  return_value_ = value;
  return true;
}

inline bool FunctionCall::CheckNumArgs(string_view name,
                                       size_t expected_args) const {
  if (num_args_ != expected_args) {
    LOG(DFATAL) << name << " expects " << expected_args << " args, but got "
                << num_args_;
    return false;
  }
  return true;
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FunctionCall);

#endif  // LULLABY_UTIL_FUNCTION_CALL_H_
