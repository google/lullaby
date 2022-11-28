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

#include "redux/engines/platform/display.h"

namespace redux {

Display::Display(DisplayProfile profile, OnDestroy on_destroy)
    : VirtualDevice(std::move(on_destroy)), profile_(std::move(profile)) {
  DisplayState init;
  init.size = profile_.display_size;
  state_.Initialize(init);
}

void Display::Apply(absl::Duration delta_time) { state_.Commit(); }

void Display::SetSize(const vec2i& size) { state_.GetMutable().size = size; }

const DisplayProfile* Display::View::GetProfile() const {
  const Display* device = GetDevice();
  return device ? &device->profile_ : nullptr;
}

vec2i Display::View::GetSize() const {
  const Display* display = GetDevice();
  return display ? display->state_.GetCurrent().size : vec2i::Zero();
}
}  // namespace redux
