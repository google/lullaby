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

#include "lullaby/modules/function/call_native_function.h"
#include "lullaby/modules/function/function_call.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

namespace lull {

// The FunctionRegistry allows the functions registered with the FunctionBinder
// to be called using a FunctionCall object.
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
  Variant Call(string_view name, Args&&... args);

  // Call the function with data bundled in the |call| object.
  Variant Call(FunctionCall* call);

 private:
  struct FunctionInfo {
    std::string name;
    std::function<void(FunctionCall*, string_view)> callback;
  };
  std::unordered_map<HashValue, FunctionInfo> functions_;
};

template <typename Fn>
void FunctionRegistry::RegisterFunction(string_view name, const Fn& fn) {
  const HashValue id = Hash(name);
  auto callback = [&fn](FunctionCall* call, string_view debug_name) {
    CallNativeFunction(call, debug_name.data(), fn);
  };
  functions_.emplace(id, FunctionInfo{name.to_string(), callback});
}

inline void FunctionRegistry::UnregisterFunction(string_view name) {
  functions_.erase(Hash(name));
}

inline bool FunctionRegistry::IsFunctionRegistered(string_view name) const {
  return IsFunctionRegistered(Hash(name));
}

inline bool FunctionRegistry::IsFunctionRegistered(HashValue id) const {
  return functions_.count(id) != 0;
}

template <typename... Args>
Variant FunctionRegistry::Call(string_view name, Args&&... args) {
  FunctionCall call = FunctionCall::Create(name, std::forward<Args>(args)...);
  return Call(&call);
}

inline Variant FunctionRegistry::Call(FunctionCall* call) {
  const auto iter = functions_.find(call->GetId());
  if (iter == functions_.end()) {
    if (call->GetName().empty()) {
      LOG(ERROR) << "Unknown function: " << call->GetId();
    } else {
      LOG(ERROR) << "Unknown function: " << call->GetName();
    }
    return Variant();
  }
  iter->second.callback(call, iter->second.name);
  return call->GetReturnValue();
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::FunctionRegistry);

#endif  // LULLABY_MODULES_FUNCTION_FUNCTION_REGISTRY_H_
