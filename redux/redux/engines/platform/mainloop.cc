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

#include "redux/engines/platform/mainloop.h"

#ifdef REDUX_EDITOR
#include "redux/editor/editor.h"
#endif
#include "redux/engines/platform/device_manager.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/static_registry.h"

namespace redux {

Mainloop::Mainloop() {
  registry_.Create<AssetLoader>(GetRegistry());
  registry_.Create<Choreographer>(GetRegistry());
  registry_.Create<DeviceManager>(GetRegistry());
  StaticRegistry::Create(GetRegistry());
}

void Mainloop::Initialize() { registry_.Initialize(); }

absl::StatusCode Mainloop::Run(const PerFrameCallback& cb) {
  absl::Time last_time = absl::Now();
  absl::StatusCode status = absl::StatusCode::kOk;
  while (status == absl::StatusCode::kOk) {
    status = PollEvents();
    if (status == absl::StatusCode::kOk && cb) {
      status = cb();
    }

    if (status == absl::StatusCode::kOk) {
      const absl::Time now = absl::Now();
      const absl::Duration delta = now - last_time;
#ifdef REDUX_EDITOR
      // Editor will step the choreographer, but can do things like slow
      // down or single step the framerate.
      status = registry_.Get<Editor>()->Update(delta);
#else
      registry_.Get<Choreographer>()->Step(delta);
#endif
      last_time = now;
    }
  }

  return status;
}

}  // namespace redux
