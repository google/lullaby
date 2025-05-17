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

#include "redux/engines/platform/headless/headless_mainloop.h"

#include <memory>
#include <string_view>

#include "redux/engines/platform/device_manager.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/engines/platform/mainloop.h"
#include "redux/modules/math/vector.h"


namespace redux {

std::unique_ptr<Mainloop> Mainloop::Create() {
  auto ptr = new HeadlessMainloop();
  return std::unique_ptr<Mainloop>(ptr);
}

void HeadlessMainloop::CreateHeadless(vec2i size) {
  auto device_manager = registry_.Get<DeviceManager>();

  DisplayProfile profile;
  profile.display_size = size;
  display_ = device_manager->Connect(profile);
  display_->SetSize(size);
}

void HeadlessMainloop::CreateDisplay(std::string_view title, vec2i size) {
  CreateHeadless(size);
}

}  // namespace redux
