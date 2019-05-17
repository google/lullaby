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

#include "lullaby/modules/input_processor/gesture.h"

#include "lullaby/modules/input_processor/input_processor.h"

namespace lull {
void Gesture::Setup(Registry* registry, HashValue hash,
                    InputManager::DeviceType device,
                    InputManager::TouchpadId touchpad, TouchIdSpan ids,
                    const mathfu::vec2& touchpad_size_cm) {
  registry_ = registry;
  hash_ = hash;
  device_ = device;
  touchpad_ = touchpad;
  input_processor_ = registry_->Get<InputProcessor>();
  input_manager_ = registry_->Get<InputManager>();
  const InputFocus* focus = input_processor_->GetInputFocus(device_);
  if (focus) {
    target_ = focus->target;
  }
  ids_ = TouchIdVector(ids.begin(), ids.end());
  touchpad_size_cm_ = touchpad_size_cm;
  Initialize();
}

GestureRecognizer::GestureRecognizer(Registry* registry, string_view name,
                                     size_t num_touches)
    : registry_(registry),
      name_(name),
      hash_(Hash(name)),
      num_touches_(num_touches) {
  input_processor_ = registry_->Get<InputProcessor>();
  input_manager_ = registry_->Get<InputManager>();
}

}  // namespace lull
