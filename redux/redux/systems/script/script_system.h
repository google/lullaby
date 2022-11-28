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

#ifndef REDUX_SYSTEMS_SCRIPT_SCRIPT_SYSTEM_H_
#define REDUX_SYSTEMS_SCRIPT_SCRIPT_SYSTEM_H_

#include <string_view>
#include <utility>

#include "redux/engines/script/script_engine.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/dispatcher/dispatcher_system.h"
#include "redux/systems/script/script_def_generated.h"

namespace redux {

// Attaches scripts to Entities that will be invoked by specific triggers.
class ScriptSystem : public System {
 public:
  explicit ScriptSystem(Registry* registry);

  void OnRegistryInitialize();

  // Invokes all the scripts associated with `ScriptTriggerType::OnUpdate'.
  // Note: this function is automatically bound to the Choreographer (if
  // available) to run after event dispatching.
  void Update(absl::Duration timestep);

  // Invokes all the scripts associated with `ScriptTriggerType::OnLateUpdate'.
  // Note: this function is automatically bound to the Choreographer (if
  // available) to run after rendering.
  void LateUpdate(absl::Duration timestep);

  // Adds a script to an Entity from a ScriptDef instance.
  void AddFromScriptDef(Entity entity, const ScriptDef& def);

 private:
  using ScriptPtr = std::unique_ptr<Script>;
  ScriptPtr LoadScript(const ScriptDef& def);

  // Entity life-cycle callbacks.
  void OnEnable(Entity entity) override;
  void OnDisable(Entity entity) override;
  void OnDestroy(Entity entity) override;

  void ConnectScript(Entity entity, TypeId event, Script* script);

  struct ScriptComponent {
    // Lists of scripts that will be invoked because of various triggers.
    std::vector<ScriptPtr> on_event;
    std::vector<ScriptPtr> on_update;
    std::vector<ScriptPtr> on_late_update;
    std::vector<ScriptPtr> on_enable;
    std::vector<ScriptPtr> on_disable;
    std::vector<ScriptPtr> on_destroy;

    // Tracks event connections that will be disconnected automatically when
    // the Component is destroyed.
    std::vector<Dispatcher::ScopedConnection> connections;
  };

  absl::flat_hash_map<Entity, ScriptComponent> scripts_;
  ScriptEngine* engine_ = nullptr;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ScriptSystem);

#endif  // REDUX_SYSTEMS_SCRIPT_SCRIPT_SYSTEM_H_
