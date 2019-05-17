/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_SCRIPT_SCRIPT_ENGINE_H_
#define LULLABY_MODULES_SCRIPT_SCRIPT_ENGINE_H_

#include <string>

#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/function/function_call.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/script_def_generated.h"

namespace lull {

// A ScriptId is an opaque id that the ScriptEngine uses to manage scripts.
class ScriptId {
 public:
  ScriptId();

  // Returns whether the script is valid.
  bool IsValid() const;

 private:
  friend class ScriptEngine;

  ScriptId(Language lang, uint64_t id);

  Language lang_;
  uint64_t id_;
};

// Define a simple type list template.
struct EmptyList {};

template <typename Head, typename... Tail>
struct TypeList {
  using First = Head;
  using Rest = TypeList<Tail...>;
};

template <typename T>
struct TypeList<T> {
  using First = T;
  using Rest = EmptyList;
};

// Define the set of types that script implementations are meant to handle.  For
// anything not on the list, implementations should fallback to Variant, so if a
// script implementation can handle variant, it can handle any type convertible
// to variant.  As such, this list is more about efficiency (avoiding conversion
// to variant) than it is about functionality.  As such, it may be appropriate
// to trim this list to be the types that give the most bang for the buck, and
// simply fallback to variant for very rare types, or for types that are cheap
// to convert to/from variant.
#define OV(x) x, Optional<x>, std::vector<x>
using ScriptableTypes =
    TypeList<bool, Optional<bool>, OV(int8_t), OV(uint8_t), OV(int16_t),
             OV(uint16_t), OV(int32_t), OV(uint32_t), OV(int64_t), OV(uint64_t),
             OV(std::string), OV(float), OV(double), OV(mathfu::vec2),
             OV(mathfu::vec2i), OV(mathfu::vec3), OV(mathfu::vec3i),
             OV(mathfu::vec4), OV(mathfu::vec4i), OV(mathfu::quat),
             OV(mathfu::mat4), EventWrapper, Clock::duration, Entity,
             VariantMap, VariantArray, Dispatcher::EventHandler>;
#undef OV

#ifdef _MSC_VER
// VC++ has trouble generating names for types generated from ScriptedTypes
// so we disable the warning.
#pragma warning(disable : 4503)
#endif  // _MSC_VER

// The IContext interface defines an abstract interface for Contexts which is
// intended to better abstract the script engine implementations.
template <typename T>
class IPartialContext {
 public:
  virtual ~IPartialContext() {}

  virtual bool ArgToCpp(const char* name, size_t arg_index, T* value) = 0;
  virtual bool ReturnFromCpp(const char* name, const T& value) = 0;
};

template <typename Types>
class IContextList : public IPartialContext<typename Types::First>,
                     public IContextList<typename Types::Rest> {
 public:
  using IPartialContext<typename Types::First>::ArgToCpp;
  using IPartialContext<typename Types::First>::ReturnFromCpp;
  using IContextList<typename Types::Rest>::ArgToCpp;
  using IContextList<typename Types::Rest>::ReturnFromCpp;
};

template <>
class IContextList<EmptyList> {
 public:
  virtual ~IContextList() {}

  virtual bool ArgToCpp(const char* name, size_t arg_index, Variant* value) = 0;
  virtual bool ReturnFromCpp(const char* name, const Variant& value) = 0;

  virtual bool CheckNumArgs(const char* name, size_t expected_args) const = 0;
};

class IContext : public IContextList<ScriptableTypes> {
 public:
  using IContextList<ScriptableTypes>::ArgToCpp;
  using IContextList<ScriptableTypes>::ReturnFromCpp;

  // For any types that aren't included in ScriptableTypes, treat as Variant.
  template <typename T>
  bool ArgToCpp(const char* name, size_t arg_index, T* value) {
    Variant var;
    if (ArgToCpp(name, arg_index, &var)) {
      return VariantConverter::FromVariant(var, value);
    }
    return false;
  }
  template <typename T>
  bool ReturnFromCpp(const char* name, const T& value) {
    Variant var;
    if (VariantConverter::ToVariant(value, &var)) {
      return ReturnFromCpp(name, var);
    }
    return false;
  }
};

// The IScriptEngine interface defines an abstract interface that all script
// engines should implement.
class IScriptEngine {
 public:
  virtual ~IScriptEngine() {}

  // Define the function type of our wrappers which allow registered functions
  // to be called by scripts.  The wrapper function takes a pointer to an
  // IContext (each engine supplies its own implementation for interracting
  // with the script engine), and returns -1 on error, 0 if there are no return
  // values, and 1 if there is a return value.
  using ScriptableFn = std::function<int(IContext*)>;

  // Engine implementations must define a static method:
  // static Language Lang();
  // Which should return the language code for the engine.

  // For engines that support 'include', set the function used to load
  // additional resources.
  virtual void SetLoadFileFunction(const AssetLoader::LoadFileFn& fn) = 0;

  // Loads a script from a string containing inline code for the given language.
  // The debug_name is used when reporting error messages.
  virtual uint64_t LoadScript(const std::string& code,
                              const std::string& debug_name) = 0;

  // Reloads a script, swapping out its code, but retaining its environment.
  virtual void ReloadScript(uint64_t id, const std::string& code) = 0;

  // Runs a loaded script.
  virtual void RunScript(uint64_t id) = 0;

  // Unloads a loaded script.
  virtual void UnloadScript(uint64_t id) = 0;

  // Register a function to be callable from script.  The function will be
  // will be callable from subsequently loaded scripts, but not from scripts
  // loaded prior to registration.
  virtual void RegisterFunction(const std::string& name, ScriptableFn fn) = 0;

  // Unregister a function.
  virtual void UnregisterFunction(const std::string& name) = 0;

  // Set a value in the script's environment.
  virtual void SetValue(uint64_t id, const std::string& name,
                        const Variant& v) = 0;

  // Get a value from the script's environment.
  virtual bool GetValue(uint64_t id, const std::string& name, Variant* v) = 0;

  // Returns the number of scripts managed by all the language engines, for
  // testing and debugging.
  virtual size_t GetTotalScripts() const = 0;
};

// The ScriptEngine loads and runs scripts by delegating to language specific
// engines.
class ScriptEngine {
 public:
  explicit ScriptEngine(Registry* registry);

  // Installs a script engine implemntation.  The type Engine should be derived
  // from IScriptEngine.
  template <typename Engine, typename... Args>
  void CreateEngine(Args&&... args);

  // Loads a script from a file, and infer the language from the filename.
  ScriptId LoadScript(const std::string& filename);

  // Loads a script from a file, and infer the language from the filename. The
  // debug_name is used when reporting error messages.
  ScriptId LoadScript(const std::string& filename,
                      const std::string& debug_name);

  // Loads a script from a file, with the given language. The debug_name is used
  // when reporting error messages.
  ScriptId LoadScript(const std::string& filename,
                      const std::string& debug_name, Language lang);

  // Loads a script from a string containing inline code for the given language.
  // The debug_name is used when reporting error messages.
  ScriptId LoadInlineScript(const std::string& code,
                            const std::string& debug_name, Language lang);

  // Reloads a script, swapping out its code, but retaining its environment.
  void ReloadScript(ScriptId id, const std::string& code);

  // Runs a loaded script.
  void RunScript(ScriptId id);

  // Unloads a loaded script.
  void UnloadScript(ScriptId id);

  // Register a function with all language specific engines.
  template <typename Fn>
  void RegisterFunction(const std::string& name, const Fn& function);

  template <typename Fn>
  struct ReturnValues;

  // Unregister a function with all language specific engines.
  void UnregisterFunction(const std::string& name);

  // Set a value in the script's environment.
  template <typename T>
  void SetValue(ScriptId id, const std::string& name, const T& value);

  // Get a value from the script's environment.
  template <typename T>
  bool GetValue(ScriptId id, const std::string& name, T* t);

  // Returns the number of scripts managed by all the language engines, for
  // testing and debugging.
  size_t GetTotalScripts() const;

 private:
  Registry* registry_;

  std::unordered_map<Language, IScriptEngine*> engines_;
};

template <typename Engine, typename... Args>
void ScriptEngine::CreateEngine(Args&&... args) {
  auto* engine = registry_->Create<Engine>(std::forward<Args>(args)...);
  engines_.emplace(Engine::Lang(), engine);

  AssetLoader* asset_loader = registry_->Get<AssetLoader>();
  if (asset_loader) {
    AssetLoader::LoadFileFn load_fn =
        asset_loader ? asset_loader->GetLoadFunction() : nullptr;
    engine->SetLoadFileFunction(load_fn);
  }
}

template <typename Fn>
void ScriptEngine::RegisterFunction(const std::string& name,
                                    const Fn& function) {
  auto wrapped = [=](IContext* context) {
    if (!CallNativeFunction(context, name.c_str(), function)) {
      return -1;
    }
    return ReturnValues<decltype(&Fn::operator())>::kNumReturnValues;
  };
  for (const auto& kv : engines_) {
    kv.second->RegisterFunction(name, wrapped);
  }
}

// ReturnValues<Fn>::value is 0 if the function returns void, and 1 otherwise,
// because we may have to tell the engine how many values we're returning.
template <typename F, typename Return, typename... Args>
struct ScriptEngine::ReturnValues<Return (F::*)(Args...) const> {
  constexpr static int kNumReturnValues = std::is_void<Return>::value ? 0 : 1;
};

template <typename T>
void ScriptEngine::SetValue(ScriptId id, const std::string& name,
                            const T& value) {
  auto it = engines_.find(id.lang_);
  if (it == engines_.end()) {
    LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
    return;
  }
  Variant var;
  VariantConverter::ToVariant(value, &var);
  it->second->SetValue(id.id_, name, var);
}

template <typename T>
bool ScriptEngine::GetValue(ScriptId id, const std::string& name, T* value) {
  auto it = engines_.find(id.lang_);
  if (it == engines_.end()) {
    LOG(ERROR) << "Unsupported language enum: " << static_cast<int>(id.lang_);
    return false;
  }
  // We initialize the variant value so that the script engine can determine the
  // desired type.
  Variant var(*value);
  if (it->second->GetValue(id.id_, name, &var)) {
    return VariantConverter::FromVariant(var, value);
  }
  return false;
}

// The ContextAdaptor is a helper to conform types which implement the Context
// interface (such as FunctionCall) to the IContext interface.
template <typename Impl, typename Types>
class ContextAdaptorImpl
    : public ContextAdaptorImpl<Impl, typename Types::Rest> {
 public:
  template <typename... CArgs>
  ContextAdaptorImpl(CArgs&&... cargs)
      : ContextAdaptorImpl<Impl, typename Types::Rest>(
            std::forward<CArgs>(cargs)...) {}

  bool ArgToCpp(const char* name, size_t arg_index,
                typename Types::First* value) override {
    return Impl::ArgToCpp(name, arg_index, value);
  }
  bool ReturnFromCpp(const char* name,
                     const typename Types::First& value) override {
    return Impl::ReturnFromCpp(name, value);
  }
};

template <typename Impl>
class ContextAdaptorImpl<Impl, EmptyList> : public IContext, public Impl {
 public:
  template <typename... Args>
  ContextAdaptorImpl(Args&&... args) : Impl(std::forward<Args>(args)...) {}

  bool ArgToCpp(const char* name, size_t arg_index, Variant* value) override {
    return Impl::ArgToCpp(name, arg_index, value);
  }
  bool ReturnFromCpp(const char* name, const Variant& value) override {
    return Impl::ReturnFromCpp(name, value);
  }

  bool CheckNumArgs(const char* name, size_t expected_args) const override {
    return Impl::CheckNumArgs(name, expected_args);
  }
};

template <typename Impl>
class ContextAdaptor : public ContextAdaptorImpl<Impl, ScriptableTypes> {
 public:
  template <typename... Args>
  ContextAdaptor(Args&&... args)
      : ContextAdaptorImpl<Impl, ScriptableTypes>(std::forward<Args>(args)...) {
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScriptEngine);

#endif  // LULLABY_MODULES_SCRIPT_SCRIPT_ENGINE_H_
