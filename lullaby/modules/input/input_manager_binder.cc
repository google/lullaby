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

#include "lullaby/modules/input/input_manager_binder.h"

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/device_util.h"
#include "lullaby/util/logging.h"

namespace lull {

InputManagerBinder::InputManagerBinder(Registry* registry)
    : registry_(registry) {
  auto* binder = registry->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->RegisterMethod("lull.InputManager.AdvanceFrame",
                         &lull::InputManager::AdvanceFrame);
  void (InputManager::*update_touch)(
      InputManager::DeviceType, InputManager::TouchpadId, InputManager::TouchId,
      const mathfu::vec2&, bool) = &lull::InputManager::UpdateTouch;
  binder->RegisterMethod("lull.InputManager.UpdateTouch", update_touch);
  binder->RegisterMethod("lull.InputManager.UpdateTouchpadSize",
                         &lull::InputManager::UpdateTouchpadSize);
}

InputManager* InputManagerBinder::Create(Registry* registry) {
  registry->Create<InputManagerBinder>(registry);
  return registry->Create<InputManager>();
}

InputManagerBinder::~InputManagerBinder() {
  auto* binder = registry_->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->UnregisterFunction("lull.InputManager.AdvanceFrame");
  binder->UnregisterFunction("lull.InputManager.UpdateTouch");
  binder->UnregisterFunction("lull.InputManager.UpdateTouchpadSize");
}

}  // namespace lull
