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

#ifndef LULLABY_MODULES_FUNCTION_CALL_NATIVE_FUNCTION_H_
#define LULLABY_MODULES_FUNCTION_CALL_NATIVE_FUNCTION_H_

#include <functional>

namespace lull {

// Calls a native function by getting arguments from the Context, passing them
// to the function, and then passing the result back to the Context. The Context
// should look like this:
// struct Context {
//   // Converts an arg contained in the context to a C++ type.
//   template <typename T>
//   bool ArgToCpp(const char* function_name, size_t arg_index, T* value);
//
//   // Converts the return value of the function from a C++ type and stores it
//   // in the context->
//   template <typename T>
//   bool ReturnFromCpp(const char* function_name, const T& value);
//
//   // Returns whether the number of arguments in the context is correct.
//   bool CheckNumArgs(const char* function_name, size_t expected_args);
// };
template <typename Context, typename Fn>
bool CallNativeFunction(Context* context, const char* name, const Fn& fn);

namespace detail {

// A Pack groups together variadic template parameter packs. It is never
// instantiated, only used as a template parameter.
template <typename... T>
class Pack {};

// AppendPack appends a template parameter to a Pack.
template <typename P, typename T>
struct AppendPackImpl;

template <typename... T>
using AppendPack = typename AppendPackImpl<T...>::Type;

template <typename... P, typename T>
struct AppendPackImpl<Pack<P...>, T> {
  using Type = Pack<P..., T>;
};

template <typename T>
struct AppendPackImpl<Pack<>, T> {
  using Type = Pack<T>;
};

// ReversePack reverses a parameter pack.
template <typename... T>
struct ReversePackImpl;

template <typename... T>
using ReversePack = typename ReversePackImpl<T...>::Type;

template <typename T0, typename... T>
struct ReversePackImpl<T0, T...> {
  using Type = AppendPack<ReversePack<T...>, T0>;
};

template <>
struct ReversePackImpl<> {
  using Type = Pack<>;
};

// NativeFunctionCaller pops arguments from the Context one by one, then calls
// the function using the arguments, then returns the result to the Context.
// Returns false if any Context function returns false.
template <typename Context, typename Return, typename ArgPack,
          typename ToDoPack, typename DonePack>
struct NativeFunctionCaller;

template <typename Context, typename Fn, typename Return, typename FirstTodo,
          typename... ToDos, typename... Dones>
struct NativeFunctionCaller<Context, Fn, Return, Pack<FirstTodo, ToDos...>,
                            Pack<Dones...>> {
  static bool Call(Context* context, const char* name, const Fn& fn,
                   const Dones&... dones) {
    using ValueType = typename std::decay<FirstTodo>::type;
    ValueType value;
    const size_t arg_index = sizeof...(ToDos);
    if (!context->ArgToCpp(name, arg_index, &value)) {
      return false;
    }
    return NativeFunctionCaller<Context, Fn, Return, Pack<ToDos...>,
                                Pack<FirstTodo, Dones...>>::Call(context, name,
                                                                 fn, value,
                                                                 dones...);
  }
};

template <typename Context, typename Fn, typename Return, typename... Dones>
struct NativeFunctionCaller<Context, Fn, Return, Pack<>, Pack<Dones...>> {
  static bool Call(Context* context, const char* name, const Fn& fn,
                   const Dones&... dones) {
    using ReturnType = typename std::decay<Return>::type;
    const ReturnType ret = fn(dones...);
    return context->ReturnFromCpp(name, ret);
  }
};

template <typename Context, typename Fn, typename... Dones>
struct NativeFunctionCaller<Context, Fn, void, Pack<>, Pack<Dones...>> {
  static bool Call(Context* context, const char* name, const Fn& fn,
                   const Dones&... dones) {
    fn(dones...);
    return true;
  }
};

// NativeFunctionWrapper checks that the number of arguments stored in the
// Context is correct, then uses NativeFunctionCaller to call the function.
template <typename Context, typename Fn>
struct NativeFunctionWrapper;

template <typename Context, typename Fn, typename Return, typename... Args>
struct NativeFunctionWrapper<Context, Return (Fn::*)(Args...) const> {
  static bool Call(Context* context, const char* name, const Fn& fn) {
    if (!context->CheckNumArgs(name, sizeof...(Args))) {
      return false;
    }
    return NativeFunctionCaller<Context, Fn, Return, ReversePack<Args...>,
                                Pack<>>::Call(context, name, fn);
  }
};

// NativeFunctionWrapper expects Fn to be a class with a (...) operator.
// WrapperType allows raw function pointers to be passed as well by converting
// them to a std::function.
template <typename Fn>
struct WrapperType {
  using Type = Fn;
};

template <typename Return, typename... Args>
struct WrapperType<Return (*)(Args...)> {
  using Type = std::function<Return(Args...)>;
};

}  // namespace detail

template <typename Context, typename Fn>
inline bool CallNativeFunction(Context* context, const char* name,
                               const Fn& fn) {
  using WrapperType = typename detail::WrapperType<Fn>::Type;
  using CallType = decltype(&WrapperType::operator());
  using Helper = detail::NativeFunctionWrapper<Context, CallType>;
  return Helper::Call(context, name, WrapperType(fn));
}

}  // namespace lull

#endif  // LULLABY_MODULES_FUNCTION_CALL_NATIVE_FUNCTION_H_
