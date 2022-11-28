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

#include "redux/engines/platform/device_manager.h"

#include "redux/modules/base/choreographer.h"

namespace redux {

DeviceManager::DeviceManager(Registry* registry) : registry_(registry) {}

void DeviceManager::OnRegistryInitialize() {
  auto* choreographer = registry_->Get<Choreographer>();
  if (choreographer) {
    choreographer->Add<&DeviceManager::AdvanceFrame>(
        Choreographer::Stage::kInput);
  }
}

std::unique_ptr<Display> DeviceManager::Connect(DisplayProfile profile) {
  return CreateDevice<redux::Display>(std::move(profile));
}

std::unique_ptr<Mouse> DeviceManager::Connect(MouseProfile profile) {
  return CreateDevice<redux::Mouse>(std::move(profile));
}

std::unique_ptr<Keyboard> DeviceManager::Connect(KeyboardProfile profile) {
  return CreateDevice<redux::Keyboard>(std::move(profile));
}

std::unique_ptr<Speaker> DeviceManager::Connect(SpeakerProfile profile) {
  return CreateDevice<redux::Speaker>(std::move(profile));
}

Display::View DeviceManager::Display(size_t index) {
  return GetView<redux::Display>(index);
}

Keyboard::View DeviceManager::Keyboard(size_t index) {
  return GetView<redux::Keyboard>(index);
}

Mouse::View DeviceManager::Mouse(size_t index) {
  return GetView<redux::Mouse>(index);
}

Speaker::View DeviceManager::Speaker(size_t index) {
  return GetView<redux::Speaker>(index);
}

void DeviceManager::SetFillAudioBufferFn(FillAudioBufferFn fn) {
  absl::MutexLock lock(&audio_cb_mutex_);
  audio_cb_ = std::move(fn);
}

void DeviceManager::AudioHwCallback(absl::Span<uint8_t> hw_buffer) {
  absl::MutexLock lock(&audio_cb_mutex_);
  if (audio_cb_) {
    audio_cb_(hw_buffer);
  } else {
    // Fill with silence.
    std::memset(hw_buffer.data(), 0, hw_buffer.size());
  }
}

void DeviceManager::AdvanceFrame(absl::Duration delta_time) {
  for (Device& device : devices_) {
    if (device.device) {
      device.device->Apply(delta_time);
    }
  }
}

int DeviceManager::GetDeviceIndex(DeviceType type, int index) const {
  for (int i = 0; i < devices_.size(); ++i) {
    if (devices_[i].type == type) {
      if (index <= 0) {
        return i;
      } else {
        --index;
      }
    }
  }
  return -1;
}

}  // namespace redux
