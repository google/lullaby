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

#include "examples/example_app/example_app.h"

#include "fplbase/utilities.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/script/function_binder.h"
#include "mathfu/constants.h"

namespace lull {

void ExampleApp::Initialize() {
  dispatcher_ = new QueuedDispatcher();
  registry_->Create<FunctionBinder>(registry_.get());
  registry_->Register(std::unique_ptr<Dispatcher>(dispatcher_));
  registry_->Create<AssetLoader>(fplbase::LoadFile);
  registry_->Create<InputManager>();
  registry_->Create<EntityFactory>(registry_.get());

  OnInitialize();
}

void ExampleApp::Update() {
  registry_->Get<AssetLoader>()->Finalize(1);

  const auto timestamp = Clock::now();

  // Don't advance on the first frame.  last_frame_time_ is uninitialized, and
  // some systems might not like a delta_time of 0.
  if (last_frame_time_ != Clock::time_point()) {
    const Clock::duration delta_time = timestamp - last_frame_time_;

    registry_->Get<InputManager>()->AdvanceFrame(delta_time);

    dispatcher_->Dispatch();

    OnAdvanceFrame(delta_time);

    RenderView views[2];
    const size_t num_views = config_.stereo ? 2 : 1;
    PopulateRenderViews(registry_.get(), views, num_views);

    OnRender({views, num_views});
  } else {
    dispatcher_->Dispatch();
  }

  last_frame_time_ = timestamp;
}

void ExampleApp::Shutdown() { OnShutdown(); }

}  // namespace lull