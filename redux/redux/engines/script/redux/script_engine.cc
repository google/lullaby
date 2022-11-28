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

#include "redux/engines/script/script_engine.h"

#include <optional>
#include <utility>

#include "redux/engines/script/redux/script_env.h"
#include "redux/engines/script/redux/script_stack.h"
#include "redux/engines/script/redux/script_value.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/static_registry.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {

class ScriptCallContextImpl : public ScriptCallContext {
 public:
  explicit ScriptCallContextImpl(ScriptFrameContext* context)
      : context_(context) {}
  ScriptFrameContext* context_;
};

class ScriptImpl : public Script {
 public:
  ScriptImpl(std::unique_ptr<ScriptEnv> env, ScriptValue script)
      : env_(std::move(env)), script_(std::move(script)) {}

  std::unique_ptr<ScriptEnv> env_;
  ScriptValue script_;
};

class ScriptEngineImpl : public ScriptEngine {
 public:
  explicit ScriptEngineImpl(Registry* registry) : ScriptEngine(registry) {}

  ScriptStack* Globals() { return &globals_; }

  void RegisterFunctionImpl(std::string_view name, ScriptableFn fn);
  void UnregisterFunctionImpl(std::string_view name);
  void SetEnumValueImpl(std::string_view name, Var value);

 private:
  ScriptStack globals_;
};

void ScriptEngineImpl::RegisterFunctionImpl(std::string_view name,
                                            ScriptableFn fn) {
  auto wrapped_fn = [=](ScriptFrame* frame) mutable {
    ScriptFrameContext context(frame);
    ScriptCallContextImpl impl(&context);
    fn(&impl);
  };

  const HashValue key = Hash(name);
  globals_.SetValue(key, ScriptValue(NativeFunction(wrapped_fn)));
}

void ScriptEngineImpl::UnregisterFunctionImpl(std::string_view name) {
  const HashValue key = Hash(name);
  globals_.SetValue(key, ScriptValue());
}

void ScriptEngineImpl::SetEnumValueImpl(std::string_view name, Var value) {
  const HashValue key = Hash(name);
  globals_.SetValue(key, ScriptValue(value));
}

ScriptEngine::ScriptEngine(Registry* registry) : registry_(registry) {}

void ScriptEngine::Create(Registry* registry) {
  auto ptr = new ScriptEngineImpl(registry);
  registry->Register(std::unique_ptr<ScriptEngine>(ptr));
}

std::unique_ptr<Script> ScriptEngine::ReadScript(std::string_view code,
                                                 std::string_view debug_name) {
  auto impl = static_cast<ScriptEngineImpl*>(this);
  auto env = std::make_unique<ScriptEnv>(impl->Globals());
  auto script = env->Read(code);

  return std::unique_ptr<Script>(
      new ScriptImpl(std::move(env), std::move(script)));
}

std::unique_ptr<Script> ScriptEngine::LoadScript(std::string_view uri) {
  auto asset_loader = registry_->Get<AssetLoader>();
  CHECK(asset_loader) << "Need AssetLoader to load scripts.";
  AssetLoader::StatusOrData asset = asset_loader->LoadNow(uri);
  CHECK(asset.ok()) << "Could not load script: " << uri;

  const absl::Span<const std::byte> data = asset->GetByteSpan();
  const std::string_view code{reinterpret_cast<const char*>(data.data()),
                              data.size()};
  return ReadScript(code, uri);
}

Var ScriptEngine::RunNow(std::string_view code) {
  return ReadScript(code)->Run();
}

void ScriptEngine::DoRegisterFunction(std::string_view name, ScriptableFn fn) {
  auto impl = static_cast<ScriptEngineImpl*>(this);
  impl->RegisterFunctionImpl(name, std::move(fn));
}

void ScriptEngine::UnregisterFunction(std::string_view name) {
  auto impl = static_cast<ScriptEngineImpl*>(this);
  impl->UnregisterFunctionImpl(name);
}

Var Script::Run() {
  auto impl = static_cast<ScriptImpl*>(this);
  ScriptValue result = impl->env_->Eval(impl->script_);
  return result.IsNil() ? Var() : *result.Get<Var>();
}

void Script::DoSetValue(std::string_view name, Var value) {
  auto impl = static_cast<ScriptImpl*>(this);
  impl->env_->SetValue(Hash(name), std::move(value));
}

Var Script::DoGetValue(std::string_view name) {
  auto impl = static_cast<ScriptImpl*>(this);
  ScriptValue value = impl->env_->GetValue(Hash(name));
  const Var* var = value.Get<Var>();
  return var ? *var : Var();
}

void ScriptEngine::DoSetEnumValue(std::string_view name, Var value) {
  auto impl = static_cast<ScriptEngineImpl*>(this);
  impl->SetEnumValueImpl(name, std::move(value));
}

Var* ScriptCallContext::GetArg(size_t index) {
  auto impl = static_cast<ScriptCallContextImpl*>(this);
  return impl->context_->GetArg(index);
}

void ScriptCallContext::SetReturnValue(Var var) {
  auto impl = static_cast<ScriptCallContextImpl*>(this);
  impl->context_->SetReturnValue(std::move(var));
}

static StaticRegistry Static_Register(ScriptEngine::Create);

}  // namespace redux
