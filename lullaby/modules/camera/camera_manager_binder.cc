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

#include "lullaby/modules/camera/camera_manager_binder.h"

#include "lullaby/modules/camera/mutable_camera.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "mathfu/io.h"

namespace lull {

CameraManagerBinder::CameraManagerBinder(Registry* registry)
    : registry_(registry), camera_manager_(registry->Create<CameraManager>()) {
  registry->RegisterDependency<RenderSystem>(this);

  auto* binder = registry->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->RegisterFunction("lull.CameraManager.CreateScreenCamera",
                           [this]() { CreateScreenCamera(); });
  binder->RegisterFunction(
      "lull.CameraManager.SetupScreenCamera",
      [this](float near_clip, float far_clip, float vertical_fov_radians,
             int width, int height) {
        SetupScreenCamera(near_clip, far_clip, vertical_fov_radians, width,
                          height);
      });
  binder->RegisterFunction(
      "lull.CameraManager.RenderScreenCameras",
      [this](std::vector<HashValue> passes) { RenderScreenCameras(passes); });
}

CameraManagerBinder::~CameraManagerBinder() {
  auto* binder = registry_->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->UnregisterFunction("lull.CameraManager.CreateScreenCamera");
  binder->UnregisterFunction("lull.CameraManager.SetupScreenCamera");
  binder->UnregisterFunction("lull.CameraManager.RenderScreenCameras");
}

CameraManager* CameraManagerBinder::Create(Registry* registry) {
  auto* binder = registry->Create<CameraManagerBinder>(registry);
  return binder->camera_manager_;
}

void CameraManagerBinder::CreateScreenCamera() {
  screen_camera_ = std::make_shared<MutableCamera>(registry_);
  camera_manager_->RegisterScreenCamera(screen_camera_);
}

void CameraManagerBinder::SetupScreenCamera(float near_clip, float far_clip,
                                            float vertical_fov_radians,
                                            int width, int height) {
  screen_camera_->SetupDisplay(
      near_clip, far_clip, vertical_fov_radians,
      mathfu::recti(mathfu::kZeros2i, mathfu::vec2i(width, height)));
}

void CameraManagerBinder::RenderScreenCameras(
    const std::vector<HashValue>& passes) {
  auto* render_system = registry_->Get<RenderSystem>();
  if (!render_system) {
    LOG(DFATAL) << "No CameraManager or no RenderSystem.";
    return;
  }

  static const size_t kMaxViews = 2;
  RenderView views[kMaxViews];
  const size_t num_views = camera_manager_->GetNumCamerasForScreen();
  camera_manager_->PopulateRenderViewsForScreen(views, num_views);
  for (const HashValue pass : passes) {
    render_system->Render(views, num_views, pass);
  }
}

}  // namespace lull
