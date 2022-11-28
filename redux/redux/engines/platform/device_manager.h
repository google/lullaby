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

#ifndef REDUX_ENGINES_PLATFORM_DEVICE_MANAGER_H_
#define REDUX_ENGINES_PLATFORM_DEVICE_MANAGER_H_

#include <cstddef>
#include <cstdint>
#include <functional>

#include "absl/synchronization/mutex.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/engines/platform/display.h"
#include "redux/engines/platform/keyboard.h"
#include "redux/engines/platform/mouse.h"
#include "redux/engines/platform/speaker.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/registry.h"

namespace redux {

// The DeviceManager is responsible for marshalling input events into a single,
// cohesive interface.  Input events can be generated from arbitrary sources
// (ex. event loops, callbacks, polling threads, etc.)
//
// The DeviceManager keeps a small buffer of state for each connected input
// device, containing three frames: front, current, and previous.  |front| is
// used for recording the incoming state for the device, i.e. from input events.
// |current| and |previous| are read-only and can be used to query the state of
// the device.  This two-frame history allows for limited support of queries
// like "just pressed" and "touch delta".
//
// The AdvanceFrame function is used to update the buffer such that the |front|
// state becomes the |current| state and a new |front| state is made available
// for write operations.  The DeviceManager allows multiple threads to generate
// input events by using a mutex.  State information is also safe to read from
// multiple threads as they are read-only operations.  However, it is assumed
// that no query operations will be performed during the AdvanceFrame call.
class DeviceManager {
 public:
  explicit DeviceManager(Registry* registry);

  DeviceManager(const DeviceManager&) = delete;
  DeviceManager& operator=(const DeviceManager&) = delete;

  void OnRegistryInitialize();

  // Connects a device. The DeviceManager will start tracking the state of this
  // device for the duration of the connection.
  std::unique_ptr<Display> Connect(DisplayProfile profile);
  std::unique_ptr<Mouse> Connect(MouseProfile profile);
  std::unique_ptr<Keyboard> Connect(KeyboardProfile profile);
  std::unique_ptr<Speaker> Connect(SpeakerProfile profile);

  // Returns a read-only "view" for a given device.
  Display::View Display(size_t index = 0);
  Keyboard::View Keyboard(size_t index = 0);
  Mouse::View Mouse(size_t index = 0);
  Speaker::View Speaker(size_t index = 0);

  // Updates the internal buffers such that the write-state is now the first
  // read-only state and a new write-state is available.
  // Important:  No queries should be made concurrently while calling this
  // function.
  void AdvanceFrame(absl::Duration delta_time);

  // Register a function that will be called when the speaker's buffer needs
  // to be filled with audio data.
  // IMPORTANT! Do not make any assumptions about when this callback will be
  // called (incl. from which thread).
  using FillAudioBufferFn = std::function<void(Speaker::HwBuffer)>;
  void SetFillAudioBufferFn(FillAudioBufferFn fn);

  // Request to fill the speaker's audio buffer. Must only be called by backend
  // speaker implementation.
  void AudioHwCallback(Speaker::HwBuffer hw_buffer);

 private:
  struct Device {
    Device(DeviceType type, VirtualDevice* device)
        : type(type), device(device) {}

    template <typename T>
    const T* As() const {
      if (type == T::Profile::kType) {
        return static_cast<T*>(device);
      }
      return nullptr;
    }

    DeviceType type;
    VirtualDevice* device = nullptr;
  };

  template <typename T, typename U>
  std::unique_ptr<T> CreateDevice(U profile) {
    const size_t index = devices_.size();
    T* ptr = new T(std::move(profile),
                   [this, index]() { devices_[index].device = nullptr; });
    devices_.emplace_back(U::kType, ptr);
    return std::unique_ptr<T>(ptr);
  }

  template <typename T>
  typename T::View GetView(size_t index) const {
    typename T::View view;
    const int n = GetDeviceIndex(T::Profile::kType, index);
    if (n >= 0) {
      view.getter = [this, n]() { return devices_[n].As<T>(); };
    } else {
      view.getter = []() { return nullptr; };
    }
    return view;
  }

  int GetDeviceIndex(DeviceType type, int index = 0) const;

  Registry* registry_;
  std::vector<Device> devices_;

  absl::Mutex audio_cb_mutex_;
  FillAudioBufferFn audio_cb_ ABSL_GUARDED_BY(audio_cb_mutex_);
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::DeviceManager);

#endif  // REDUX_ENGINES_PLATFORM_DEVICE_MANAGER_H_
