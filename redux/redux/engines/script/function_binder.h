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

#ifndef REDUX_ENGINES_SCRIPT_FUNCTION_BINDER_H_
#define REDUX_ENGINES_SCRIPT_FUNCTION_BINDER_H_

#include <string>
#include <string_view>
#include <vector>

#include "redux/engines/script/script_engine.h"

namespace redux {

// Manages the binding and (automatic) unbinding of functions with the
// ScriptEngine.
//
// Any functions bound to the ScriptEngine via the FunctionBinder will be
// unregistered when the FunctionBinder is destroyed.
class FunctionBinder {
 public:
  explicit FunctionBinder(Registry* registry);
  ~FunctionBinder();

  FunctionBinder(const FunctionBinder&) = delete;
  FunctionBinder& operator=(const FunctionBinder&) = delete;

  // Registers a function with the ScriptEngine.
  template <typename Fn>
  void RegisterFn(std::string_view name, const Fn& fn) {
    GetScriptEngine()->RegisterFunction(name, fn);
    functions_.emplace_back(name);
  }

  // Registers an enum with the ScriptEngine.
  template <typename En>
  void RegisterEnum(std::string_view prefix) {
    static_assert(std::is_enum_v<En>);
    GetScriptEngine()->RegisterEnum<En>(prefix);
  }

  // Registers a member function call with the ScriptEngine. The caller is
  // required to provide the pointer-to-member-function and the instance of the
  // object with which to invoke it.
  template <typename T, typename Fn>
  void RegisterMemFn(std::string_view name, T* obj, const Fn& fn) {
    static_assert(std::is_member_function_pointer_v<Fn>);
    RegisterMemberFunctionBinder<Fn> binder;
    GetScriptEngine()->RegisterFunction(name, binder.Bind(obj, fn));
    functions_.emplace_back(name);
  }

 private:
  ScriptEngine* GetScriptEngine();

  template <typename Fn>
  struct RegisterMemberFunctionBinder;

  Registry* registry_;
  ScriptEngine* script_engine_ = nullptr;
  std::vector<std::string> functions_;
};

// Specialization for non-const method with return type R.
template <typename T, typename R, typename... Args>
struct FunctionBinder::RegisterMemberFunctionBinder<R (T::*)(Args...)> {
  static auto Bind(T* obj, R (T::*fn)(Args...)) {
    return
        [=](Args... args) { return (obj->*fn)(std::forward<Args>(args)...); };
  }
};

// Specialization for const method with return type R.
template <typename T, typename R, typename... Args>
struct FunctionBinder::RegisterMemberFunctionBinder<R (T::*)(Args...) const> {
  static auto Bind(T* obj, R (T::*fn)(Args...) const) {
    return
        [=](Args... args) { return (obj->*fn)(std::forward<Args>(args)...); };
  }
};

}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_FUNCTION_BINDER_H_
