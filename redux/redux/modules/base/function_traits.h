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

#ifndef REDUX_MODULES_BASE_FUNCTION_TRAITS_H_
#define REDUX_MODULES_BASE_FUNCTION_TRAITS_H_

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace redux {

// Traits-like class for getting information about functions, like its return
// type, the number of arguments, and the type of each argument. Given a
// decltype for a function (e.g. FunctionTraits<decltype(Fn)>):
//
// FunctionTraits<Fn>::ReturnType:
//    The return type of the function Fn.
//
// FunctionTraits<Fn>::ArgTuple:
//    An std::tuple<> where the types of the elements in the tuple matches the
//    types of the function arguments.
//
// FunctionTraits<Fn>::kNumArgs:
//    The number of arguments (as a size_t) the function accepts.
//
// FunctionTraits<Fn>::Arg<N>::Type:
//    The type of the Nth function argument.
//
// FunctionTraits<Fn>::Signature:
//    A type alias for the expression: `ReturnType(Args...)`. Useful for
//    creating template specializations.

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())> {
  // For functors (incl. lambdas and std::function).
};

template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> : FunctionTraits<R(Args...)> {
  // For non-const member functions.
  using ClassType = C;
};

template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const> : FunctionTraits<R(Args...)> {
  // For const member functions.
  using ClassType = C;
};

template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)> : FunctionTraits<R(Args...)> {
  // For function pointers.
};

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)> {
  // Base implementation for free functions.
  using Signature = R(Args...);
  using ReturnType = R;
  using ArgTuple = std::tuple<Args...>;
  static constexpr size_t kNumArgs = sizeof...(Args);

  template <size_t N>
  struct Arg {
    using Type = typename std::tuple_element<N, ArgTuple>::type;
  };
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_FUNCTION_TRAITS_H_
