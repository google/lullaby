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

#ifndef THIRD_PARTY_REDUX_MODULES_BASE_CALL_NATIVE_FUNCTION_H_
#define THIRD_PARTY_REDUX_MODULES_BASE_CALL_NATIVE_FUNCTION_H_

#include <type_traits>

#include "absl/status/status.h"
#include "redux/modules/base/function_traits.h"
#include "redux/modules/base/logging.h"

namespace redux {

// "Calls" a function of type Fn using the Context object. The Context object is
// required to provide two functions:
//
// absl::StatusCode ArgFromCpp(size_t index, T& arg_out);
//   This function should set `arg_out` to the value that will be passed as the
//   n-th argument (as specified by `index`) to the function Fn. An error code
//   can be returned (if, for example, the argument index is out of range of the
//   argument is not of type T.)
//
// absl::StatusCode ReturnFromCpp(T&&);
//   This function should store the given value into the context. This value is
//   the result of invoking the function Fn. An error code can be returned if
//   the context is unable to handle the return value.
//
// This function will return the error code returned by any of the above calls
// if an error is encountered, otherwise it will return absl::StatusCode::kOk.
template <typename Context, typename Fn>
absl::StatusCode CallNativeFunction(Context* context, const Fn& fn);

namespace detail {

// Generates a NativeFunction object that wraps the given functor.
template <typename Context, typename Signature>
class NativeFunctionCaller;

template <typename Context, typename R, typename... Args>
class NativeFunctionCaller<Context, R(Args...)> {
 public:
  explicit NativeFunctionCaller(Context* context) : context_(context) {
    CHECK(context != nullptr) << "Must provide a context!";
  }

  template <typename Fn>
  absl::StatusCode Invoke(const Fn& fn) {
    ExpandArgumentsAndCall(fn, std::index_sequence_for<Args...>{});
    return status_;
  }

 private:
  template <typename Fn, size_t... S>
  void ExpandArgumentsAndCall(const Fn& fn, std::index_sequence<S...>) {
    // Expansion will first call ReadArg for all arguments, then pass the
    // resulting values to `CallFunction` which will then invoke `fn` with the
    // arg values.
    CallFunction(fn, ReadArg<Args>(S)...);
  }

  // Attempts to evaluate the value of the Argument and the given `index`.
  template <typename T>
  std::decay_t<T> ReadArg(size_t index) {
    std::decay_t<T> value = {};
    if (status_ == absl::StatusCode::kOk) {
      status_ = context_->ArgFromCpp(index, &value);
      CHECK(status_ == absl::StatusCode::kOk)
          << "Unable to read arg " << index << ": " << status_;
    }
    return value;
  }

  template <typename Fn>
  void CallFunction(const Fn& fn, Args&&... args) {
    CHECK(status_ == absl::StatusCode::kOk)
        << "Error evaluating arguments; will not invoke function.";

    if constexpr (std::is_void_v<R>) {
      // Function returns void, just call it with the arguments.
      fn(std::forward<Args>(args)...);
    } else {
      // Call the function with the arguments, storing the result in the
      // context or any error that may have occurred in the status.
      status_ = context_->ReturnFromCpp(fn(std::forward<Args>(args)...));
      CHECK(status_ == absl::StatusCode::kOk) << "Unable to set return value.";
    }
  }

  Context* context_ = nullptr;
  absl::StatusCode status_ = absl::StatusCode::kOk;
};

}  // namespace detail

template <typename Context, typename Fn>
absl::StatusCode CallNativeFunction(Context* context, const Fn& fn) {
  using Signature = typename FunctionTraits<Fn>::Signature;
  detail::NativeFunctionCaller<Context, Signature> caller(context);
  return caller.Invoke(fn);
}

}  // namespace redux

#endif  // THIRD_PARTY_REDUX_MODULES_BASE_CALL_NATIVE_FUNCTION_H_
