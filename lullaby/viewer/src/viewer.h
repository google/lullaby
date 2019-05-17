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

#ifndef LULLABY_VIEWER_SRC_VIEWER_H_
#define LULLABY_VIEWER_SRC_VIEWER_H_

#include "lullaby/viewer/src/window.h"
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace tool {

class Viewer : public Window {
 public:
  Viewer() {}

  // Creates an entity using the file at the specified path.
  void CreateEntity(const std::string& path);
  // Imports assets at the specified path.
  void ImportDirectory(const std::string& path);

 protected:
  void OnInitialize() override;
  void AdvanceFrame(double dt, int width, int height) override;
  void OnShutdown() override;

 private:
  void AdvanceLullabySystems(double dt);
  void UpdateViewerGui(int width, int height);

  std::shared_ptr<Registry> registry_;
  QueuedDispatcher* dispatcher_ = nullptr;
  Clock::time_point last_frame_time_;
  bool paused_ = false;
  bool single_step_ = false;
  float dt_override_ = 0.f;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_VIEWER_SRC_VIEWER_H_
