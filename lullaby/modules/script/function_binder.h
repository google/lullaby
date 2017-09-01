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

#ifndef LULLABY_MODULES_FUNCTION_FUNCTION_BINDER_H_
#define LULLABY_MODULES_FUNCTION_FUNCTION_BINDER_H_

#include <string>

#include "lullaby/modules/function/call_native_function.h"
#include "lullaby/modules/function/function_call.h"
#include "lullaby/modules/script/script_engine.h"
#include "lullaby/util/built_in_functions.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/registry.h"

namespace lull {

// The FunctionBinder provides a centralized location to register functions,
// by delegating to a number of other systems, such as the ScriptEngine.
class FunctionBinder {
 public:
  explicit FunctionBinder(Registry* registry);

  // Registers a function with a name. Overloading function names is not
  // supported.
  template <typename Fn>
  void RegisterFunction(string_view name, Fn function);

  // Registers a method from a class. The class must be in the Registry.
  template <typename Method>
  void RegisterMethod(string_view name, Method method);

  // Unregister a function by name.
  void UnregisterFunction(string_view name);

  // Returns true if the function with the given |name| has been registered.
  bool IsFunctionRegistered(string_view name) const;

  // Returns true if the function with the given |id| (which is simply the Hash
  // of its name) has been registered.
  bool IsFunctionRegistered(HashValue id) const;

  // Call the function with given |name| with the provided |args|.
  template <typename... Args>
  Variant Call(string_view name, Args&&... args);

  // Call the function with data bundled in the |call| object.
  Variant Call(FunctionCall* call);

 private:
  struct FunctionWrapper {
    virtual ~FunctionWrapper() {}
    virtual void Call(FunctionCall* call) = 0;
  };

  template <typename NativeFunction>
  struct TypedFunctionWrapper : public FunctionWrapper {
    TypedFunctionWrapper(std::string name, NativeFunction fn)
        : name(std::move(name)), fn(std::move(fn)) {}

    void Call(FunctionCall* call) override {
      CallNativeFunction(call, name.c_str(), fn);
    }

    std::string name;
    NativeFunction fn;
  };

  template <typename Method>
  struct CreateMethodHelper;

  Registry* registry_;
  std::unordered_map<HashValue, std::unique_ptr<FunctionWrapper>> functions_;
};

template <typename Fn>
void FunctionBinder::RegisterFunction(string_view name, Fn function) {
  const HashValue id = Hash(name);
  if (IsFunctionRegistered(id)) {
    LOG(ERROR) << "Cannot register function twice: " << name;
    return;
  }

  auto ptr =
      new TypedFunctionWrapper<Fn>(name.to_string(), std::move(function));
  functions_.emplace(id, std::unique_ptr<FunctionWrapper>(ptr));

  auto* script_engine = registry_->Get<ScriptEngine>();
  if (script_engine) {
    script_engine->RegisterFunction(name.to_string(), ptr->fn);
  }
}

template <typename Method>
void FunctionBinder::RegisterMethod(string_view name, Method method) {
  RegisterFunction(name, CreateMethodHelper<Method>::Call(registry_, method));
}

template <typename... Args>
Variant FunctionBinder::Call(string_view name, Args&&... args) {
  FunctionCall call = FunctionCall::Create(name, std::forward<Args>(args)...);
  return Call(&call);
}

template <typename Class, typename Return, typename... Args>
struct FunctionBinder::CreateMethodHelper<Return (Class::*)(Args...)> {
  static std::function<Return(Args...)> Call(Registry* registry,
                                             Return (Class::*method)(Args...)) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        return (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "Class not in registry, cannot call method.";
        return Return();
      }
    };
  }
};

template <typename Class, typename... Args>
struct FunctionBinder::CreateMethodHelper<void (Class::*)(Args...)> {
  static std::function<void(Args...)> Call(Registry* registry,
                                           void (Class::*method)(Args...)) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "Class not in registry, cannot call method.";
      }
    };
  }
};

template <typename Class, typename Return, typename... Args>
struct FunctionBinder::CreateMethodHelper<Return (Class::*)(Args...) const> {
  static std::function<Return(Args...)> Call(Registry* registry,
                                             Return (Class::*method)(Args...)
                                                 const) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        return (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "Class not in registry, cannot call method.";
        return Return();
      }
    };
  }
};

template <typename Class, typename... Args>
struct FunctionBinder::CreateMethodHelper<void (Class::*)(Args...) const> {
  static std::function<void(Args...)> Call(Registry* registry,
                                           void (Class::*method)(Args...)
                                               const) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "Class not in registry, cannot call method.";
      }
    };
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FunctionBinder);

#endif  // LULLABY_MODULES_FUNCTION_FUNCTION_BINDER_H_
