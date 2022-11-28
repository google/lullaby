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

#ifndef REDUX_ENGINES_PLATFORM_MAINLOOP_H_
#define REDUX_ENGINES_PLATFORM_MAINLOOP_H_

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include "absl/status/status.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/math/vector.h"

namespace redux {

// The Main Event Loop.
//
// This is the main entry point for redux runtimes. It owns the Registry and is
// responsible for "running" the system event loop. It also provides the
// mechanism through which system devices (e.g. display, mouse, keyboard, etc.)
// are created.
class Mainloop {
 public:
  static std::unique_ptr<Mainloop> Create();
  virtual ~Mainloop() = default;

  Mainloop(const Mainloop&) = delete;
  Mainloop& operator=(const Mainloop&) = delete;

  virtual void CreateDisplay(std::string_view title, vec2i size) = 0;
  virtual void CreateKeyboard() = 0;
  virtual void CreateMouse() = 0;
  virtual void CreateSpeaker() = 0;

  // Initializes the registry.
  void Initialize();

  // Runs the mainloop. A user-supplied callback can be provided which will be
  // run "inside" the loop. If the callback returns a non-Ok status, the loop
  // will be exited.
  using PerFrameCallback = std::function<absl::StatusCode()>;
  absl::StatusCode Run(const PerFrameCallback& cb);

  // Returns the main registry.
  Registry* GetRegistry() { return &registry_; }

 protected:
  Mainloop();

  virtual absl::StatusCode PollEvents() = 0;

  Registry registry_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_MAINLOOP_H_
