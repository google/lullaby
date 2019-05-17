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

#ifndef LULLABY_EXAMPLES_EXAMPLE_APP_PORT_SDL2_SOFTWARE_HMD_H_
#define LULLABY_EXAMPLES_EXAMPLE_APP_PORT_SDL2_SOFTWARE_HMD_H_

#include "lullaby/modules/camera/mutable_camera.h"
#include "lullaby/util/registry.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Emulates the Daydream Head Mounted Display using the mouse.
class SoftwareHmd {
 public:
  SoftwareHmd(Registry* registry, size_t width, size_t height, bool stereo,
              float near_clip_plane, float far_clip_plane);
  ~SoftwareHmd();

  // Updates the InputManager with the current state of this device.
  void Update();

  // Represents the different ways mouse movement can be applied to move or
  // rotate the HMD.
  enum ControlMode {
    kRotatePitchYaw,
    kRotateRoll,
    kTranslateXY,
    kTranslateXZ,
  };

  // Updates the position or rotation of the HMD based on mouse movement.
  void OnMouseMotion(const mathfu::vec2i& delta, ControlMode mode);

 private:
  Registry* registry_ = nullptr;  // Registry for accessing the InputManager.
  size_t width_ = 0;              // Width of the HMD "display".
  size_t height_ = 0;             // Height of the HMD "display".
  size_t num_eyes_ = 1;           // Number of "eyes" represented by this HMD:
                                  // 1 for monoscopic, 2 for stereoscopic.
  mathfu::vec3 translation_;      // Position of the HMD.
  mathfu::vec3 rotation_;         // Euler angles of the HMD rotation.

  std::shared_ptr<MutableCamera> cameras_[2];
};

}  // namespace lull

#endif  // LULLABY_EXAMPLES_EXAMPLE_APP_PORT_SDL2_SOFTWARE_HMD_H_
