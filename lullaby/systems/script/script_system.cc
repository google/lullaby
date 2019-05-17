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

#include "lullaby/systems/script/script_system.h"

#include "lullaby/generated/script_def_generated.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

namespace {
const HashValue kScriptOnEventDefHash = ConstHash("ScriptOnEventDef");
const HashValue kScriptEveryFrameDefHash = ConstHash("ScriptEveryFrameDef");
const HashValue kScriptOnCreateDefHash = ConstHash("ScriptOnCreateDef");
const HashValue kScriptOnPostCreateInitDefHash =
    ConstHash("ScriptOnPostCreateInitDef");
const HashValue kScriptOnDestroyDefHash = ConstHash("ScriptOnDestroyDef");
}  // namespace

ScriptSystem::ScriptSystem(Registry* registry)
    : System(registry),
      every_frame_scripts_(8),
      on_destroy_scripts_(8),
      event_scripts_(8) {
  RegisterDef<ScriptOnEventDefT>(this);
  RegisterDef<ScriptEveryFrameDefT>(this);
  RegisterDef<ScriptOnCreateDefT>(this);
  RegisterDef<ScriptOnPostCreateInitDefT>(this);
  RegisterDef<ScriptOnDestroyDefT>(this);

  RegisterDependency<TransformSystem>(this);

  engine_ = registry->Get<ScriptEngine>();
  if (engine_ == nullptr) {
    LOG(DFATAL) << "No script engine";
    return;
  }
}

void ScriptSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kScriptOnCreateDefHash) {
    auto data = ConvertDef<ScriptOnCreateDef>(def);
    auto script_id = LoadScriptDef(data->script(), entity);
    if (script_id.IsValid()) {
      engine_->RunScript(script_id);
      engine_->UnloadScript(script_id);
    }
  }
}

void ScriptSystem::PostCreateInit(Entity entity, HashValue type,
                                  const Def* def) {
  if (type == kScriptOnEventDefHash) {
    auto data = ConvertDef<ScriptOnEventDef>(def);
    auto script_id = LoadScriptDef(data->script(), entity);
    if (data->inputs() && script_id.IsValid()) {
      AddScript(&event_scripts_, entity, script_id);
      ConnectEventDefs(registry_, entity, data->inputs(),
                       [this, script_id](const EventWrapper& event) {
                         engine_->SetValue(script_id, "event", event);
                         engine_->RunScript(script_id);
                       });
    }
  } else if (type == kScriptEveryFrameDefHash) {
    auto data = ConvertDef<ScriptEveryFrameDef>(def);
    auto script_id = LoadScriptDef(data->script(), entity);
    if (script_id.IsValid()) {
      AddScript(&every_frame_scripts_, entity, script_id);
    }
  } else if (type == kScriptOnPostCreateInitDefHash) {
    auto data = ConvertDef<ScriptOnPostCreateInitDef>(def);
    auto script_id = LoadScriptDef(data->script(), entity);
    if (script_id.IsValid()) {
      engine_->RunScript(script_id);
      engine_->UnloadScript(script_id);
    }
  } else if (type == kScriptOnDestroyDefHash) {
    auto data = ConvertDef<ScriptOnDestroyDef>(def);
    auto script_id = LoadScriptDef(data->script(), entity);
    if (script_id.IsValid()) {
      AddScript(&on_destroy_scripts_, entity, script_id);
    }
  }
}

void ScriptSystem::AddScript(ComponentPool<Scripts>* pool, Entity entity,
                             ScriptId id) {
  Scripts* scripts = pool->Get(entity);
  if (scripts) {
    scripts->ids.emplace_back(id);
  } else {
    pool->Emplace(entity, id);
  }
}

ScriptId ScriptSystem::LoadScriptDef(const ScriptDef* script, Entity entity) {
  auto script_id = LoadScriptDef(script);
  if (script_id.IsValid()) {
    engine_->SetValue(script_id, "entity", entity);
  }
  return script_id;
}

ScriptId ScriptSystem::LoadScriptDef(const ScriptDef* script) {
  if (!script) {
    LOG(ERROR) << "No script def";
    return ScriptId();
  }
  if (script->filename()) {
    const std::string debug_name = script->debug_name()
                                       ? script->debug_name()->str()
                                       : script->filename()->str();
    if (script->language() == Language_Unknown) {
      return engine_->LoadScript(script->filename()->str(), debug_name);
    } else {
      return engine_->LoadScript(script->filename()->str(), debug_name,
                                 script->language());
    }
  } else if (script->code()) {
    if (script->language() != Language_Unknown) {
      const std::string debug_name =
          script->debug_name() ? script->debug_name()->str() : "inline script";
      return engine_->LoadInlineScript(script->code()->str(), debug_name,
                                       script->language());
    } else {
      LOG(ERROR) << "No language in inline code script def";
    }
  } else {
    LOG(ERROR) << "No filename or inline code in script def";
  }
  return ScriptId();
}

void ScriptSystem::Destroy(Entity entity) {
  Scripts* on_destroy = on_destroy_scripts_.Get(entity);
  if (on_destroy) {
    for (const auto& id : on_destroy->ids) {
      engine_->RunScript(id);
      engine_->UnloadScript(id);
    }
  }
  on_destroy_scripts_.Destroy(entity);
  Scripts* every_frame = every_frame_scripts_.Get(entity);
  if (every_frame) {
    for (const auto& id : every_frame->ids) {
      engine_->UnloadScript(id);
    }
  }
  every_frame_scripts_.Destroy(entity);
  Scripts* event = event_scripts_.Get(entity);
  if (event) {
    for (const auto& id : event->ids) {
      engine_->UnloadScript(id);
    }
  }
  event_scripts_.Destroy(entity);
}

void ScriptSystem::AdvanceFrame(Clock::duration delta_time) {
  double delta_time_double = std::chrono::duration<double>(delta_time).count();
  auto* transform_system = registry_->Get<TransformSystem>();
  for (const Scripts& scripts : every_frame_scripts_) {
    if (transform_system->IsEnabled(scripts.GetEntity())) {
      for (const auto& id : scripts.ids) {
        engine_->SetValue(id, "delta_time", delta_time_double);
        engine_->RunScript(id);
      }
    }
  }
}

}  // namespace lull
