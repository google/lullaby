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

#include "redux/systems/script/script_system.h"

#include <string_view>
#include <utility>

#include "redux/modules/base/choreographer.h"
#include "redux/modules/ecs/entity_factory.h"

namespace redux {

constexpr char kEntity[] = "$entity";
constexpr char kMessage[] = "$message";
constexpr char kDeltaTime[] = "$delta_time";

ScriptSystem::ScriptSystem(Registry* registry) : System(registry) {
  RegisterDef(&ScriptSystem::AddFromScriptDef);
  RegisterDependency<ScriptEngine>(this);
}

void ScriptSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<ScriptEngine>();
  CHECK(engine_ != nullptr) << "ScriptEngine is required.";

  auto choreo = registry_->Get<Choreographer>();
  if (choreo != nullptr) {
    choreo->Add<&ScriptSystem::Update>(Choreographer::Stage::kEvents)
        .After<&DispatcherSystem::Dispatch>();
    choreo->Add<&ScriptSystem::LateUpdate>(Choreographer::Stage::kEpilogue);
  }
}

void ScriptSystem::AddFromScriptDef(Entity entity, const ScriptDef& def) {
  if (entity == kNullEntity) {
    return;
  }
  auto script = LoadScript(def);
  if (script == nullptr) {
    return;
  }

  switch (def.type) {
    case ScriptTriggerType::OnCreate:
      // Invoke the OnCreate script now.
      script->SetValue(kEntity, entity);
      script->Run();
      break;
    case ScriptTriggerType::OnEnable:
      scripts_[entity].on_enable.push_back(std::move(script));
      break;
    case ScriptTriggerType::OnDisable:
      scripts_[entity].on_disable.push_back(std::move(script));
      break;
    case ScriptTriggerType::OnDestroy:
      scripts_[entity].on_destroy.push_back(std::move(script));
      break;
    case ScriptTriggerType::OnUpdate:
      scripts_[entity].on_update.push_back(std::move(script));
      break;
    case ScriptTriggerType::OnLateUpdate:
      scripts_[entity].on_late_update.push_back(std::move(script));
      break;
    case ScriptTriggerType::OnEvent:
      ConnectScript(entity, TypeId(def.event.get()), script.get());
      scripts_[entity].on_event.push_back(std::move(script));
      break;
  }
}

void ScriptSystem::ConnectScript(Entity entity, TypeId event, Script* script) {
  CHECK(script != nullptr) << "Must provide script.";
  CHECK_NE(event, TypeId(0)) << "Must specify TypeId for event.";
  auto dispatcher_system = registry_->Get<DispatcherSystem>();

  // Note that the handler is capturing the raw pointer to the script, so the
  // lifetime of the connection must not exceed the lifetime of the script.
  auto handler = [script, entity](const Message& msg) {
    script->SetValue(kEntity, entity);
    script->SetValue(kMessage, msg);
    script->Run();
  };
  auto connection =
      dispatcher_system->Connect(entity, event, std::move(handler));
  scripts_[entity].connections.emplace_back(std::move(connection));
}

void ScriptSystem::Update(absl::Duration timestep) {
  for (auto& iter : scripts_) {
    if (IsEntityEnabled(iter.first)) {
      for (const ScriptPtr& script : iter.second.on_update) {
        script->SetValue(kEntity, iter.first);
        script->SetValue(kDeltaTime, timestep);
        script->Run();
      }
    }
  }
}

void ScriptSystem::LateUpdate(absl::Duration timestep) {
  for (auto& iter : scripts_) {
    if (IsEntityEnabled(iter.first)) {
      for (const ScriptPtr& script : iter.second.on_late_update) {
        script->SetValue(kEntity, iter.first);
        script->SetValue(kDeltaTime, timestep);
        script->Run();
      }
    }
  }
}

void ScriptSystem::OnEnable(Entity entity) {
  auto iter = scripts_.find(entity);
  if (iter != scripts_.end()) {
    for (const ScriptPtr& script : iter->second.on_enable) {
      script->SetValue(kEntity, entity);
      script->Run();
    }
  }
}

void ScriptSystem::OnDisable(Entity entity) {
  auto iter = scripts_.find(entity);
  if (iter != scripts_.end()) {
    for (const ScriptPtr& script : iter->second.on_disable) {
      script->SetValue(kEntity, entity);
      script->Run();
    }
  }
}

void ScriptSystem::OnDestroy(Entity entity) {
  auto iter = scripts_.find(entity);
  if (iter != scripts_.end()) {
    for (const ScriptPtr& script : iter->second.on_destroy) {
      script->SetValue(kEntity, entity);
      script->Run();
    }
    iter->second.connections.clear();
    scripts_.erase(iter);
  }
}

ScriptSystem::ScriptPtr ScriptSystem::LoadScript(const ScriptDef& def) {
  if (!def.code.empty()) {
    return engine_->ReadScript(def.code, "script");
  } else if (!def.uri.empty()) {
    return engine_->LoadScript(def.uri);
  } else {
    LOG(FATAL) << "ScriptDef must specify either code or uri.";
  }
}

}  // namespace redux
