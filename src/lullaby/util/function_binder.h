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

#ifndef LULLABY_UTIL_FUNCTION_BINDER_H_
#define LULLABY_UTIL_FUNCTION_BINDER_H_

#include <string>

#include "lullaby/base/registry.h"
#include "lullaby/script/script_engine.h"
#include "lullaby/util/built_in_functions.h"
#include "lullaby/util/function_registry.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"

namespace lull {

// The FunctionBinder provides a centralized location to register functions,
// by delegating to a number of other systems, such as the ScriptEngine.
class FunctionBinder {
 public:
  explicit FunctionBinder(Registry* registry) : registry_(registry) {
    RegisterBuiltInFunctions(this);
  }

  // Registers a function with a name. Overloading function names is not
  // supported.
  template <typename Fn>
  void RegisterFunction(const std::string& name, Fn&& function);

  // Registers a method from a class. The class must be in the Registry.
  template <typename Method>
  void RegisterMethod(const std::string& name, Method method);

  // Unregister a function by name.
  void UnregisterFunction(const std::string& name) {
    auto it = functions_.find(Hash(name.c_str()));
    if (it == functions_.end()) {
      LOG(ERROR) << "FunctionBinder tried to unregister a "
                 << "non-existent function: " << name;
      return;
    }
    functions_.erase(it);

    auto* script_engine = registry_->Get<ScriptEngine>();
    if (script_engine) {
      script_engine->UnregisterFunction(name);
    }

    auto* function_registry = registry_->Get<FunctionRegistry>();
    if (function_registry) {
      function_registry->UnregisterFunction(name);
    }
  }

 private:
  struct FunctionWrapper {
    virtual ~FunctionWrapper() {}
  };

  template <typename Fn>
  struct TypedFunctionWrapper : public FunctionWrapper {
    explicit TypedFunctionWrapper(Fn&& fn) : fn(fn) {}
    ~TypedFunctionWrapper() override {}
    Fn fn;
  };

  Registry* registry_;
  std::unordered_map<HashValue, std::unique_ptr<FunctionWrapper>> functions_;
};

template <typename Fn>
void FunctionBinder::RegisterFunction(const std::string& name, Fn&& function) {
  const HashValue name_hash = Hash(name.c_str());
  if (functions_.find(name_hash) != functions_.end()) {
    LOG(ERROR) << "FunctionBinder tried to register a duplicate: " << name;
    return;
  }
  auto wrapper = new TypedFunctionWrapper<Fn>(std::move(function));
  functions_.emplace(name_hash,
                     std::unique_ptr<TypedFunctionWrapper<Fn>>(wrapper));

  auto* script_engine = registry_->Get<ScriptEngine>();
  if (script_engine) {
    script_engine->RegisterFunction(name, wrapper->fn);
  }

  auto* function_registry = registry_->Get<FunctionRegistry>();
  if (function_registry) {
    function_registry->RegisterFunction(name, wrapper->fn);
  }
}

namespace detail {

template <typename Method>
struct CreateMethod;

template <typename Class, typename Return, typename... Args>
struct CreateMethod<Return (Class::*)(Args...)> {
  static std::function<Return(Args...)> Call(Registry* registry,
                                             Return (Class::*method)(Args...)) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        return (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "FunctionBinder tried to call a method on a class that "
                      "isn't in the registry";
        return Return();
      }
    };
  }
};

template <typename Class, typename... Args>
struct CreateMethod<void (Class::*)(Args...)> {
  static std::function<void(Args...)> Call(Registry* registry,
                                           void (Class::*method)(Args...)) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "FunctionBinder tried to call a method on a class that "
                      "isn't in the registry";
      }
    };
  }
};

template <typename Class, typename Return, typename... Args>
struct CreateMethod<Return (Class::*)(Args...) const> {
  static std::function<Return(Args...)> Call(Registry* registry,
                                             Return (Class::*method)(Args...)
                                                 const) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        return (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "FunctionBinder tried to call a method on a class that "
                      "isn't in the registry";
        return Return();
      }
    };
  }
};

template <typename Class, typename... Args>
struct CreateMethod<void (Class::*)(Args...) const> {
  static std::function<void(Args...)> Call(Registry* registry,
                                           void (Class::*method)(Args...)
                                               const) {
    return [registry, method](Args... args) {
      Class* cls = registry->Get<Class>();
      if (cls != nullptr) {
        (cls->*method)(std::forward<Args>(args)...);
      } else {
        LOG(ERROR) << "FunctionBinder tried to call a method on a class that "
                      "isn't in the registry";
      }
    };
  }
};

}  // namespace detail

template <typename Method>
void FunctionBinder::RegisterMethod(const std::string& name, Method method) {
  RegisterFunction(name, detail::CreateMethod<Method>::Call(registry_, method));
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FunctionBinder);

#endif  // LULLABY_UTIL_FUNCTION_BINDER_H_
