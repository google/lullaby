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

#include "lullaby/modules/javascript/engine.h"

#include <utility>

#include "libplatform/libplatform.h"
#include "lullaby/modules/javascript/convert.h"
#include "lullaby/util/logging.h"

using v8::V8;

namespace lull {
namespace script {
namespace javascript {

namespace {

// Defines a struct used to initialize and shutdown v8. This needs to be done
// once per lifetime of the process. An instance of it is allocated statically
// in the Engine's constructor.
struct V8Initializer {
  V8Initializer() : platform_(v8::platform::CreateDefaultPlatform()) {
    V8::InitializePlatform(platform_);
    bool success = V8::Initialize();
    // The documenation for V8::Initialize says "it always returns true." To be
    // conservative, we assume that there is no safe course of action in the
    // case that it returns false.
    CHECK(success);
  }

  void perform_maintenance_tasks(v8::Isolate *isolate) {
    while (v8::platform::PumpMessageLoop(platform_, isolate)) continue;
  }

  ~V8Initializer() {
    V8::Dispose();
    V8::ShutdownPlatform();
  }

  v8::Platform *platform_;
};

// The print function made available to scripts.
std::string Print(const v8::FunctionCallbackInfo<v8::Value> &args) {
  std::stringstream stream;
  for (int i = 0; i < args.Length(); i++) {
    v8::String::Utf8Value arg(args.GetIsolate(), args[i]);
    const char *s = *arg;
    if (i > 0) {
      stream << " " << s;
    } else {
      stream << s;
    }
  }
  return stream.str();
}

void SetTargetObject(v8::Isolate *isolate, v8::Local<v8::Object> object,
                     v8::Local<v8::Value> value, const std::string &name) {
  const size_t index = name.find('.');
  if (index == std::string::npos) {
    object->Set(v8::String::NewFromUtf8(isolate, name.c_str()), value);
  } else {
    const std::string sub_object_name = name.substr(0, index);
    // Check if an object by this name already exists.
    v8::Local<v8::Value> key =
        v8::String::NewFromUtf8(isolate, sub_object_name.c_str());
    if (!object->Has(key)) {
      v8::Local<v8::Object> sub_object = v8::Object::New(isolate);
      object->Set(key, sub_object);
      object = sub_object;
    } else {
      object = object->Get(key)->ToObject(isolate);
    }
    SetTargetObject(isolate, object, value, name.substr(index + 1));
  }
}

void UnsetTargetObject(v8::Isolate *isolate, v8::Local<v8::Object> object,
                       const std::string &name) {
  const size_t index = name.find('.');
  if (index == std::string::npos) {
    v8::Local<v8::Value> key = v8::String::NewFromUtf8(isolate, name.c_str());
    object->Delete(key);
  } else {
    const std::string sub_object_name = name.substr(0, index);
    // Check if an object by this name already exists.
    v8::Local<v8::Value> key =
        v8::String::NewFromUtf8(isolate, sub_object_name.c_str());
    if (object->Has(key)) {
      object = object->Get(key)->ToObject(isolate);
      UnsetTargetObject(isolate, object, name.substr(index + 1));
    }
  }
}

template <typename Types>
struct ConverterImpl : public ConverterImpl<typename Types::Rest> {
  static inline bool JsToCpp(v8::Isolate *isolate,
                             const v8::Local<v8::Value> &js_value,
                             Variant *value) {
    if (auto *cpp_value = value->Get<typename Types::First>()) {
      return Convert<typename Types::First>::JsToCpp(isolate, js_value,
                                                     cpp_value);
    }
    return ConverterImpl<typename Types::Rest>::JsToCpp(isolate, js_value,
                                                        value);
  }

  static inline v8::Local<v8::Value> CppToJs(v8::Isolate *isolate,
                                             const Variant &value) {
    if (auto *cpp_value = value.Get<typename Types::First>()) {
      return Convert<typename Types::First>::CppToJs(isolate, *cpp_value);
    }
    return ConverterImpl<typename Types::Rest>::CppToJs(isolate, value);
  }
};

template <>
struct ConverterImpl<EmptyList> {
  static inline bool JsToCpp(v8::Isolate *isolate,
                             const v8::Local<v8::Value> &js_value,
                             Variant *value) {
    return Convert<Variant>::JsToCpp(isolate, js_value, value);
  }

  static inline v8::Local<v8::Value> CppToJs(v8::Isolate *isolate,
                                             const Variant &value) {
    return Convert<Variant>::CppToJs(isolate, value);
  }
};

struct Converter : ConverterImpl<ScriptableTypes> {};

}  // namespace

Engine::Engine() {
  // Initialize v8.
  static V8Initializer initializer;
  script_maintainer_fn_ = [&](v8::Isolate *isolate) {
    initializer.perform_maintenance_tasks(isolate);
  };

  // Create a v8 Isolate that represents our execution engine.
  allocator_ = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_;
  isolate_ = v8::Isolate::New(create_params);

  // Store the array buffer allocator as data in the isolate. It is used
  // to allocate memory for ArrayBuffer objects when doing conversion from
  // array-like data structures, e.g.: mathfu::vec3, etc.
  isolate_->SetData(IsolateDataSlot::kArrayBufferAllocator,
                    create_params.array_buffer_allocator);
}

Engine::~Engine() {
  TriggerGarbageCollection();
  // Release all loaded scripts before disposing the isolate.
  scripts_.clear();
  isolate_->Dispose();
  delete allocator_;
}

v8::MaybeLocal<v8::Script> Engine::CompileScript(
    const std::string &code, const std::string &debug_name) {
  IsolateLocker lock(isolate_);
  v8::Local<v8::Context> context = isolate_->GetCurrentContext();
  v8::Context::Scope context_scope(context);

  // Setup the script source and name(origin).
  v8::ScriptOrigin origin(
      v8::String::NewFromUtf8(isolate_, debug_name.c_str()));
  v8::Local<v8::String> script_source(
      v8::String::NewFromUtf8(isolate_, code.c_str()));
  v8::ScriptCompiler::Source source(script_source, origin);

  // Set up an exception handler before compiling the script.
  v8::TryCatch try_catch(isolate_);

  // Now compile the script.
  v8::MaybeLocal<v8::Script> script =
      v8::ScriptCompiler::Compile(context, &source);
  if (try_catch.HasCaught()) {
    v8::String::Utf8Value error(isolate_, try_catch.Exception());
    LOG(ERROR) << debug_name << ": " << *error;

    v8::Local<v8::Value> stacktrace;
    if (try_catch.StackTrace(context).ToLocal(&stacktrace)) {
      v8::String::Utf8Value stack(isolate_, stacktrace);
      LOG(ERROR) << *stack;
    }
  }
  return script;
}

void Engine::SetLoadFileFunction(const AssetLoader::LoadFileFn &load_fn) {
  load_fn_ = load_fn;
}

uint64_t Engine::LoadScript(const std::string &filename) {
  if (!load_fn_) {
    LOG(ERROR) << "No LoadFileFn. Call SetLoadFileFunction first.";
    return 0;
  }
  std::string data;
  if (!load_fn_(filename.c_str(), &data)) {
    return 0;
  }
  return LoadScript(data, filename);
}

uint64_t Engine::LoadScript(const std::string &code,
                            const std::string &debug_name) {
  IsolateLocker lock(isolate_);

  // Create the global environment.
  v8::Global<v8::Context> context_global = CreateEnv();
  v8::Local<v8::Context> context = context_global.Get(isolate_);
  v8::Context::Scope context_scope(context);

  // Now compile the script.
  v8::MaybeLocal<v8::Script> script = CompileScript(code, debug_name);
  if (script.IsEmpty()) {
    return 0;
  }

  // Create a ScriptInfo to hold on to the script and its execution context.
  ScriptInfo *script_info = new ScriptInfo();
  script_info->name = debug_name;
  script_info->script.Reset(isolate_, script.ToLocalChecked());
  script_info->context.Reset(isolate_, context);
  scripts_.emplace(reinterpret_cast<uint64_t>(script_info),
                   std::unique_ptr<ScriptInfo>(script_info));

  // Use the pointer to the script info as script id.
  return reinterpret_cast<uint64_t>(script_info);
}

void Engine::UnloadScript(uint64_t id) {
  const auto &it = scripts_.find(id);
  if (it != scripts_.end()) {
    scripts_.erase(it);
  }
}

size_t Engine::GetTotalScripts() const { return scripts_.size(); }

void Engine::RunScript(uint64_t id) {
  const auto &it = scripts_.find(id);
  if (it == scripts_.end()) {
    LOG(ERROR) << "Script not found";
    return;
  }

  ScriptInfo *script_info = it->second.get();

  IsolateContextLocker lock(isolate_, script_info->context);

  // Set up an exception handler before running the script.
  v8::TryCatch try_catch(isolate_);

  // Now run the script.
  v8::Local<v8::Script> script = script_info->script.Get(isolate_);
  v8::MaybeLocal<v8::Value> result = script->Run(lock.GetLocalContext());

  if (result.IsEmpty()) {
    if (try_catch.HasCaught()) {
      v8::String::Utf8Value error(isolate_, try_catch.Exception());
      LOG(ERROR) << script_info->name << ": " << *error;

      v8::Local<v8::Value> stacktrace;
      if (try_catch.StackTrace(lock.GetLocalContext()).ToLocal(&stacktrace)) {
        v8::String::Utf8Value stack(isolate_, stacktrace);
        LOG(ERROR) << *stack;
      }
    } else {
      LOG(ERROR) << script_info->name <<": Execution failed with unknown error";
    }
  }

  TriggerGarbageCollection();
  script_maintainer_fn_(isolate_);
}

void Engine::ReloadScript(uint64_t id, const std::string &code) {
  ScriptInfo *script_info = reinterpret_cast<ScriptInfo *>(id);

  IsolateContextLocker lock(isolate_, script_info->context);

  // Now compile the script.
  v8::MaybeLocal<v8::Script> script = CompileScript(code, script_info->name);
  if (script.IsEmpty()) {
    // Error already logged in CompileScript.
    return;
  }

  // Save the new script.
  script_info->script.Reset(isolate_, script.ToLocalChecked());
}

v8::Local<v8::Object> Engine::IncludeImpl(
    const v8::FunctionCallbackInfo<v8::Value> &args) {
  v8::Local<v8::Object> ret_value;
  if (args.Length() != 1) {
    LOG(ERROR) << "include expects exactly 1 argument";
    return ret_value;
  }

  if (!args[0]->IsString()) {
    LOG(ERROR) << "include expects a string filename";
    return ret_value;
  }

  v8::String::Utf8Value arg(args.GetIsolate(), args[0]);
  std::string filename(*arg);
  uint64_t id = 0;
  auto itr = included_scripts_.find(filename);
  if (itr == included_scripts_.end()) {
    id = LoadScript(filename);
    if (id == 0) {
      // Error already displayed by LoadScript.
      return ret_value;
    }
    RunScript(id);
    included_scripts_[filename] = id;
  } else {
    id = itr->second;
  }

  ScriptInfo *script_info = reinterpret_cast<ScriptInfo *>(id);
  v8::Local<v8::Context> context = script_info->context.Get(isolate_);
  ret_value = context->Global();

  return ret_value;
}

v8::Global<v8::Context> Engine::CreateEnv() {
  IsolateLocker lock(isolate_);

  v8::Local<v8::ObjectTemplate> object_template =
      v8::ObjectTemplate::New(isolate_);

  // Builtins: console.log|debug|error functions.
  v8::Local<v8::ObjectTemplate> console = v8::ObjectTemplate::New(isolate_);
  console->Set(
      v8::String::NewFromUtf8(isolate_, "log"),
      v8::FunctionTemplate::New(
          isolate_, [](const v8::FunctionCallbackInfo<v8::Value> &args) {
            LOG(INFO) << Print(args);
          }));
  console->Set(
      v8::String::NewFromUtf8(isolate_, "debug"),
      v8::FunctionTemplate::New(
          isolate_, [](const v8::FunctionCallbackInfo<v8::Value> &args) {
            DLOG(INFO) << Print(args);
          }));
  console->Set(
      v8::String::NewFromUtf8(isolate_, "error"),
      v8::FunctionTemplate::New(
          isolate_, [](const v8::FunctionCallbackInfo<v8::Value> &args) {
            LOG(ERROR) << Print(args);
          }));

  object_template->Set(v8::String::NewFromUtf8(isolate_, "console"), console);

  object_template->Set(
      v8::String::NewFromUtf8(isolate_, "include"),
      v8::FunctionTemplate::New(
          isolate_,
          [](const v8::FunctionCallbackInfo<v8::Value> &args) {
            // Get the reference to Engine passed in as function data.
            v8::Local<v8::External> external =
                v8::Local<v8::External>::Cast(args.Data());
            Engine *engine = reinterpret_cast<Engine *>(external->Value());

            v8::Local<v8::Value> ret_value = engine->IncludeImpl(args);
            args.GetReturnValue().Set(ret_value);
          },
          v8::External::New(
              isolate_,
              const_cast<void *>(reinterpret_cast<const void *>(this)))));

  // Allow access to this context from other script contexts in order to
  // enable the include script feature.
  object_template->SetAccessCheckCallback(
      [](v8::Local<v8::Context>, v8::Local<v8::Object> accessed_object,
         v8::Local<v8::Value> data) { return true; });

  // Create a context needed to add registered functions.
  v8::Local<v8::Context> context =
      v8::Context::New(isolate_, nullptr /* extensions */, object_template);
  v8::Context::Scope context_scope(context);

  // Get the global object to add registered functions to.
  v8::Local<v8::Object> global = context->Global();

  // Add the registered functions.
  for (const auto &kv : functions_) {
    auto &fn_info = kv.second;
    auto fn_value = v8::Function::New(
        context,
        [](const v8::FunctionCallbackInfo<v8::Value> &args) {
          // Get the reference to FunctionInfo passed in as function data.
          v8::Local<v8::External> external =
              v8::Local<v8::External>::Cast(args.Data());

          auto fn_info = reinterpret_cast<FunctionInfo *>(external->Value());
          fn_info->fn(args);
        },
        v8::External::New(isolate_, reinterpret_cast<void *>(fn_info.get())));

    if (!fn_value.IsEmpty()) {
      fn_value.ToLocalChecked()->SetName(
          v8::String::NewFromUtf8(isolate_, fn_info->name.c_str()));

      SetTargetObject(isolate_, global, fn_value.ToLocalChecked(),
                      fn_info->name);
    }
  }

  return v8::Global<v8::Context>(isolate_, context);
}

void Engine::RegisterFunction(const std::string &name, ScriptableFn fn) {
  RegisterFunctionImpl(
      name, [name, fn](const v8::FunctionCallbackInfo<v8::Value> &args) {
        ContextAdaptor<JsContext> context(args);
        fn(&context);
      });
}

void Engine::RegisterFunctionImpl(const std::string &name, JsLambda &&fn) {
  functions_.emplace(
      Hash(name.c_str()),
      std::unique_ptr<FunctionInfo>(new FunctionInfo(name, std::move(fn))));
}

void Engine::UnregisterFunction(const std::string &name) {
  if (functions_.find(Hash(name)) == functions_.end()) {
    return;
  }

  // Remove the named function from all scripts.
  for (const auto &kv : scripts_) {
    ScriptInfo *script_info = kv.second.get();

    IsolateContextLocker lock(isolate_, script_info->context);

    // Get the global object to remove registered functions from.
    v8::Local<v8::Object> global = lock.GetLocalContext()->Global();

    UnsetTargetObject(isolate_, global, name);
  }

  functions_.erase(Hash(name));
}

void Engine::TriggerGarbageCollection() { isolate_->LowMemoryNotification(); }

void Engine::SetValue(uint64_t id, const std::string &name,
                      const Variant &value) {
  const auto &it = scripts_.find(id);
  if (it == scripts_.end()) {
    LOG(ERROR) << "Script not found";
    return;
  }

  ScriptInfo *script_info = it->second.get();
  IsolateContextLocker lock(isolate_, script_info->context);

  v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate_, name.c_str());
  v8::Local<v8::Value> js_value = Converter::CppToJs(isolate_, value);
  v8::Local<v8::Object> global = lock.GetLocalContext()->Global();
  global->Set(key, js_value);
}

bool Engine::GetValue(uint64_t id, const std::string &name, Variant *value) {
  const auto &it = scripts_.find(id);
  if (it == scripts_.end()) {
    LOG(ERROR) << "Script not found";
    return false;
  }

  ScriptInfo *script_info = it->second.get();
  IsolateContextLocker lock(isolate_, script_info->context);

  v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate_, name.c_str());
  v8::Local<v8::Context> context = lock.GetLocalContext();
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> js_value = global->Get(context, key).ToLocalChecked();
  if (js_value.IsEmpty() || js_value->IsUndefined()) {
    return false;
  }
  return Converter::JsToCpp(isolate_, js_value, value);
}

}  // namespace javascript
}  // namespace script
}  // namespace lull
