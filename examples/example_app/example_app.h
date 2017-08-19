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

#ifndef LULLABY_EXAMPLES_EXAMPLE_APP_EXAMPLE_APP_H_
#define LULLABY_EXAMPLES_EXAMPLE_APP_EXAMPLE_APP_H_

#include <memory>
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/span.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Provides a basic structure for creating a Lullaby-based example apps.
//
// This class takes care of some of the "low-level" Lullaby setup, like
// creating the Registry, setting up the QueuedDispatcher, and
//
// Examples can inherit from this class, and then start creating/updating the
// Systems they are demonstrating using the virtual On*** functions.
//
// Examples should specify their derived class type using the
// LULLABY_EXAMPLE_APP macro. This allows the underlying platform to create the
// correct ExampleApp instance.
class ExampleApp {
 public:
  // Basic configuration information about the app.  This information is used
  // by the underlying platform to create and manage the window and devices.
  struct Config {
    std::string title = "";
    bool stereo = true;
    size_t width = 1280;
    size_t height = 640;
    float near_clip_plane = 0.2f;
    float far_clip_plane = 100.f;
    bool enable_hmd = true;
    bool enable_controller = true;
  };

  ExampleApp() : registry_(std::make_shared<Registry>()) {}
  virtual ~ExampleApp() {}

  // Returns the Config associated with this app. Derives classes can update
  // the config_ in their constructor to override the default behaviour.
  const Config& GetConfig() const { return config_; }

  // Returns the Registry associated with this app.
  std::shared_ptr<Registry> GetRegistry() { return registry_; }

  // Initializes the example (including the derived class).
  void Initialize();

  // Updates the example once per frame (including the derived class).
  void Update();

  // Shutsdown the example once per frame (including the derived class).
  void Shutdown();

 protected:
  virtual void OnInitialize() {}
  virtual void OnAdvanceFrame(Clock::duration delta_time) {}
  virtual void OnRender(Span<RenderView> views) {}
  virtual void OnShutdown() {}

  // The config associated with this example app.
  Config config_;

  // Registry class that owns all other high-level systems and utility classes.
  std::shared_ptr<Registry> registry_;

  // Event dispatcher for handling thread-safe communication.  Owned by
  // registry_.
  QueuedDispatcher* dispatcher_ = nullptr;

  // Timestamp of previous frame to calculate delta time between frames.
  Clock::time_point last_frame_time_;

 private:
  ExampleApp(const ExampleApp&) = delete;
  ExampleApp& operator=(const ExampleApp&) = delete;
};

// Creates an instance of the example app for use by the underlying platform
// layer.
std::unique_ptr<ExampleApp> CreateExampleAppInstance();

}  // namespace lull

// Allows examples to specify the actual class type for the ExampleApp instance
// to create.
#define LULLABY_EXAMPLE_APP(App) \
  namespace lull { \
  std::unique_ptr<ExampleApp> CreateExampleAppInstance() { \
    return std::unique_ptr<ExampleApp>(new App()); \
  } \
  }

#endif  // LULLABY_EXAMPLES_EXAMPLE_APP_EXAMPLE_APP_H_
