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

#include "lullaby/examples/example_app/example_app.h"

#include "lullaby/events/render_events.h"
#include "lullaby/modules/camera/camera_manager.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/script/function_binder.h"

#if LULLABY_ENABLE_EDITOR
#include "lullaby/editor/src/editor.h"
#endif

namespace lull {
constexpr HashValue kScreenshotTestHash = ConstHash("screenshot_test");
constexpr int kMaxFrameCount = 30;
constexpr float kConstantFrameRate = 10.0f;

void ExampleApp::Initialize(void* native_window) {
  dispatcher_ = new QueuedDispatcher();
  registry_->Create<FunctionBinder>(registry_.get());
  registry_->Register(std::unique_ptr<Dispatcher>(dispatcher_));
  registry_->Create<AssetLoader>(registry_.get());
  registry_->Create<InputManager>();
  registry_->Create<CameraManager>();
  registry_->Create<EntityFactory>(registry_.get());

  native_window_ = native_window;
  OnInitialize();

  dispatcher_->Send(lull::SetNativeWindowEvent(native_window_));

#if LULLABY_ENABLE_EDITOR
  // Initialize the editor.  By default this will go to port 1235
  Editor::Initialize(registry_.get());
#endif
}

void ExampleApp::Update() {
  registry_->Get<AssetLoader>()->Finalize(1);
  ++frame_count_;

  const auto timestamp = Clock::now();
  bool should_advance = true;

  // Don't advance on the first frame.  last_frame_time_ is uninitialized, and
  // some systems might not like a delta_time of 0.
  if (last_frame_time_ != Clock::time_point()) {
    Clock::duration delta_time = timestamp - last_frame_time_;
    const auto* global_config = registry_->Get<lull::Config>();
    if (global_config) {
      if (global_config->Get(kScreenshotTestHash, false)) {
        // Need to advance frame at constant time for screenshot testing.
        delta_time = lull::DurationFromMilliseconds(kConstantFrameRate);
        if (frame_count_ >= kMaxFrameCount) {
          should_advance = false;
        }
      }
    }
    if (should_advance) {
      registry_->Get<InputManager>()->AdvanceFrame(delta_time);

      ProcessEventsForDevice(registry_.get(), InputManager::kController);

      dispatcher_->Dispatch();

      OnAdvanceFrame(delta_time);
    }
    static const size_t kMaxViews = 2;
    RenderView views[kMaxViews];

    auto* camera_manager = registry_->Get<CameraManager>();
    const size_t num_views = camera_manager->GetNumCamerasForScreen();
    camera_manager->PopulateRenderViewsForScreen(views, num_views);
    OnRender({views, num_views});

#if LULLABY_ENABLE_EDITOR
    auto* editor = registry_->Get<Editor>();
    if (editor) {
      editor->AdvanceFrame(delta_time, {views, num_views});
    }
#endif
  } else {
    dispatcher_->Dispatch();
  }

  last_frame_time_ = timestamp;
}

void ExampleApp::Shutdown() { OnShutdown(); }

}  // namespace lull
