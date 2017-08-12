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

#ifndef LULLABY_MODULES_FUNCTION_FUNCTION_REGISTRY_H_
#define LULLABY_MODULES_FUNCTION_FUNCTION_REGISTRY_H_

#include <functional>
#include <unordered_map>

#include "lullaby/modules/script/call_native_function.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

namespace lull {

// A FunctionCall bundles up the name of the function to call, and the arguments
// as an array of Variants.
struct FunctionCall {
  static constexpr size_t kMaxArgs = 32;

  // Constructs the FunctionCall with the |id| for a function (which is just the
  // Hash of the function name) and, optionally, the actual |name| of the
  // function.
  explicit FunctionCall(HashValue id, string_view name = "")
      : id(id), name(name) {}

  // Adds an argument to the FunctionCall.
  template <typename T>
  void AddArg(T&& value);

  HashValue id = 0;
  string_view name = nullptr;
  size_t num_args = 0;
  Variant args[kMaxArgs];
};

// The FunctionRegistry allows the functions registered with the FunctionBinder
// to be called by name or ID (which is just the Hash of the name), with
// Variants as arguments, returning a Variant.
class FunctionRegistry {
 public:
  // Register the function |fn| with the given |name|.
  template <typename Fn>
  void RegisterFunction(string_view name, const Fn& fn);

  // Unregister the function with the given |name|.
  void UnregisterFunction(string_view name);

  // Returns true if the function with the given |name| has been registered.
  bool IsFunctionRegistered(string_view name) const;

  // Returns true if the function with the given |id| (which is simply the Hash
  // of its name) has been registered.
  bool IsFunctionRegistered(HashValue id) const;

  // Call the function with given |name| with the provided |args|.
  template <typename... Args>
  Variant Call(string_view name, Args... args);

  // Call the function with data bundled in the |call| object.
  Variant Call(const FunctionCall& call);

  // Bundle function name and arguments into a FunctionCall struct which can
  // then be used to actually invoke a function.
  template <typename... Args>
  FunctionCall Bundle(string_view name, Args... args) const;

 private:
  struct FunctionInfo {
    std::string name;
    std::function<Variant(const FunctionCall&)> callback;
  };
  std::unordered_map<HashValue, FunctionInfo> functions_;

  // Used by CallNativeFunction to perform the actual invocation of a function
  // by extracting args and providing storage for a return value.
  struct Context {
    explicit Context(const FunctionCall& call) : call(call) {}

    template <typename T>
    bool ArgToCpp(const char* name, size_t arg_index, T* value);

    template <typename T>
    bool ReturnFromCpp(const char* name, const T& value);

    bool CheckNumArgs(const char* name, size_t expected_args);

    const FunctionCall& call;
    Variant return_value;
  };

  template <typename T, bool IsVector = detail::IsVector<T>::kValue,
            bool IsMap = detail::IsMap<T>::kValue>
  struct Convert;

  template <typename Return, typename... Args>
  struct Convert<std::function<Return(Args...)>, false, false> {
    static const char* GetTypeName() {
      static const std::string type = "std::function<Ret(Args...)>";
      return type.c_str();
    }

    static bool VariantToNative(const Variant& variant,
                                std::function<Return(Args...)>* native) {
      LOG(DFATAL) << "This conversion is not supported.";
      return false;
    }
  };

  template <typename T>
  struct Convert<T, false, false> {
    static const char* GetTypeName() { return ::lull::GetTypeName<T>(); }

    static bool VariantToNative(const Variant& variant, T* native) {
      const T* value = variant.Get<T>();
      if (value) {
        *native = *value;
        return true;
      } else {
        return false;
      }
    }
  };

  template <typename T>
  struct Convert<Optional<T>, false, false> {
    static const char* GetTypeName() {
      const char* type_name = Convert<T>::GetTypeName();
      static const std::string type =
          std::string("lull::Optional<") + type_name + std::string(">");
      return type.c_str();
    }

    static bool VariantToNative(const Variant& variant, Optional<T>* t) {
      if (variant.Empty()) {
        t->reset();
        return true;
      }
      const T* v = variant.Get<T>();
      if (v == nullptr) {
        return false;
      }
      *t = *v;
      return true;
    }
  };

  template <typename T>
  struct Convert<T, true, false> {
    static const char* GetTypeName() {
      const char* type_name = Convert<typename T::value_type>::GetTypeName();
      static const std::string type =
          std::string("std::vector<") + type_name + std::string(">");
      return type.c_str();
    }

    static bool VariantToNative(const Variant& variant, T* t) {
      const auto* vvect = variant.Get<VariantArray>();
      if (vvect == nullptr) {
        return false;
      }
      T out;
      out.reserve(vvect->size());
      for (const Variant& var : *vvect) {
        const auto* tv = var.Get<typename T::value_type>();
        if (tv == nullptr) {
          return false;
        }
        out.emplace_back(*tv);
      }
      *t = std::move(out);
      return true;
    }
  };

  template <typename T>
  struct Convert<T, false, true> {
    using K = typename T::key_type;
    using V = typename T::mapped_type;
    static const char* GetTypeName() {
      static std::string type =
          (detail::IsMap<T>::kUnordered ? std::string("std::unordered_map")
                                        : std::string("std::map")) +
          std::string("<lull::HashValue, ") + Convert<V>::GetTypeName() +
          std::string(">");
      return type.c_str();
    }

    static bool VariantToNative(const Variant& variant, T* t) {
      static_assert(
          std::is_same<K, HashValue>::value,
          "FunctionRegistry only supports maps with lull::HashValue keys.");
      const auto* vmap = variant.Get<VariantMap>();
      if (vmap == nullptr) {
        return false;
      }
      T out;
      for (const auto& kv : *vmap) {
        const auto* v = kv.second.Get<V>();
        if (v == nullptr) {
          return false;
        }
        out[kv.first] = *v;
      }
      *t = std::move(out);
      return true;
    }
  };
};

template <typename T>
void FunctionCall::AddArg(T&& value) {
  if (num_args < kMaxArgs) {
    args[num_args] = std::forward<T>(value);
    ++num_args;
  } else {
    LOG(DFATAL) << "Max number of args exceeded";
  }
}

template <typename Fn>
void FunctionRegistry::RegisterFunction(string_view name, const Fn& fn) {
  const HashValue id = Hash(name.data(), name.size());
  auto callback = [&fn](const FunctionCall& call) {
    Context context(call);
    CallNativeFunction(&context, call.name.data(), fn);
    return context.return_value;
  };
  functions_.emplace(id, FunctionInfo{name.to_string(), callback});
}

inline void FunctionRegistry::UnregisterFunction(string_view name) {
  const HashValue id = Hash(name.data(), name.size());
  functions_.erase(id);
}

inline bool FunctionRegistry::IsFunctionRegistered(string_view name) const {
  const HashValue id = Hash(name.data(), name.size());
  return IsFunctionRegistered(id);
}

inline bool FunctionRegistry::IsFunctionRegistered(HashValue id) const {
  return functions_.count(id) != 0;
}

template <typename... Args>
Variant FunctionRegistry::Call(string_view name, Args... args) {
  return Call(Bundle(name, std::forward<Args>(args)...));
}

inline Variant FunctionRegistry::Call(const FunctionCall& call) {
  const auto iter = functions_.find(call.id);
  if (iter == functions_.end()) {
    LOG(ERROR) << "Unknown function: " << call.name.data();
    return Variant();
  }
  return iter->second.callback(call);
}

template <typename... Args>
FunctionCall FunctionRegistry::Bundle(string_view name, Args... args) const {
  const HashValue id = Hash(name.data(), name.size());

  const auto iter = functions_.find(id);
  if (iter == functions_.end()) {
    LOG(ERROR) << "Unknown function: " << name.data();
    return FunctionCall(0, "");
  }

  FunctionCall call(id, iter->second.name);
  int dummy[] = {(call.AddArg(args), 0)...};
  (void)dummy;
  return call;
}

template <typename T>
bool FunctionRegistry::Context::ArgToCpp(const char* name, size_t arg_index,
                                         T* value) {
  if (!Convert<T>::VariantToNative(call.args[arg_index], value)) {
    LOG(ERROR) << name << " expects the type of arg " << arg_index + 1
               << " to be " << Convert<T>::GetTypeName();
    return false;
  }
  return true;
}

template <typename T>
bool FunctionRegistry::Context::ReturnFromCpp(const char* name,
                                              const T& value) {
  return_value = value;
  return true;
}

inline bool FunctionRegistry::Context::CheckNumArgs(const char* name,
                                                    size_t expected_args) {
  if (call.num_args != expected_args) {
    LOG(ERROR) << name << " expects " << expected_args << " args, but got "
               << call.num_args;
    return false;
  }
  return true;
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FunctionRegistry);

#endif  // LULLABY_MODULES_FUNCTION_FUNCTION_REGISTRY_H_
