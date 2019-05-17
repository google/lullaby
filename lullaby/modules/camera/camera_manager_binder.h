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

#ifndef LULLABY_MODULES_CAMERA_CAMERA_MANAGER_BINDER_H_
#define LULLABY_MODULES_CAMERA_CAMERA_MANAGER_BINDER_H_

#include <vector>

#include "lullaby/modules/camera/camera_manager.h"
#include "lullaby/modules/camera/mutable_camera.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/typeid.h"

namespace lull {

/// A simple utility class that adds some function bindings for CameraManager,
/// and removes them when destroyed.
class CameraManagerBinder final {
 public:
  explicit CameraManagerBinder(Registry* registry);
  ~CameraManagerBinder();

  /// Create and register a new CameraManager and this binder in the Registry.
  static CameraManager* Create(Registry* registry);

 private:
  void CreateScreenCamera();
  void SetupScreenCamera(float near_clip, float far_clip,
                         float vertical_fov_radians, int width, int height);
  void RenderScreenCameras(const std::vector<HashValue>& passes);

  Registry* registry_;
  CameraManager* camera_manager_;
  std::shared_ptr<MutableCamera> screen_camera_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::CameraManagerBinder);

#endif  // LULLABY_MODULES_CAMERA_CAMERA_MANAGER_BINDER_H_
