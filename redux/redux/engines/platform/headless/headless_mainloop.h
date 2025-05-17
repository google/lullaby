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

#ifndef REDUX_ENGINES_PLATFORM_HEADLESS_HEADLESS_MAINLOOP_H_
#define REDUX_ENGINES_PLATFORM_HEADLESS_HEADLESS_MAINLOOP_H_

#include <memory>
#include <string_view>

#include "absl/status/status.h"
#include "redux/engines/platform/display.h"
#include "redux/engines/platform/mainloop.h"
#include "redux/modules/math/vector.h"

namespace redux {

class HeadlessMainloop : public Mainloop {
 public:
  HeadlessMainloop() = default;

  void CreateHeadless(vec2i size) override;
  void CreateDisplay(std::string_view title, vec2i size) override;
  void CreateKeyboard() override {}
  void CreateMouse() override {}
  void CreateSpeaker() override {}

 private:
  absl::StatusCode PollEvents() override { return absl::StatusCode::kOk; }

  std::unique_ptr<Display> display_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_HEADLESS_HEADLESS_MAINLOOP_H_
