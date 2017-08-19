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

#ifndef LULLABY_EXAMPLES_EXAMPLE_APP_PORT_SDL2_SOFTWARE_CONTROLLER_H_
#define LULLABY_EXAMPLES_EXAMPLE_APP_PORT_SDL2_SOFTWARE_CONTROLLER_H_

#include "mathfu/glsl_mappings.h"
#include "lullaby/util/registry.h"

namespace lull {

// Emulates the Daydream Controller using the mouse.
class SoftwareController {
 public:
  explicit SoftwareController(Registry* registry);
  ~SoftwareController();

  // Updates the InputManager with the current state of this device.
  void Update();

  // Updates the rotation of the controller based on mouse movement.
  void OnMouseMotion(const mathfu::vec2i& delta);

  // Updates the button state of the controller based on button click.
  void OnButtonDown();

  // Updates the button state of the controller based on button release.
  void OnButtonUp();

 private:
  Registry* registry_ = nullptr;  // Registry for accessing the InputManager.
  mathfu::vec3 rotation_;         // Euler angles of controller rotation.
  bool pressed_ = false;          // Indicates if the primary button is pressed.
};

}  // namespace lull

#endif  // LULLABY_EXAMPLES_EXAMPLE_APP_PORT_SDL2_SOFTWARE_CONTROLLER_H_
