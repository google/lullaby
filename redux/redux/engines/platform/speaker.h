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

#ifndef REDUX_ENGINES_PLATFORM_SPEAKER_H_
#define REDUX_ENGINES_PLATFORM_SPEAKER_H_

#include "absl/types/span.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/engines/platform/virtual_device.h"

namespace redux {

class Speaker : public VirtualDevice {
 public:
  using Profile = SpeakerProfile;

  // The state of the speaker that will be exposed by the device manager.
  struct View : VirtualView<Speaker> {
    const Profile* GetProfile() const;
  };

  // A chunk of audio data that will be fed into the hardware speaker.
  using HwBuffer = absl::Span<uint8_t>;

 private:
  friend class DeviceManager;
  Speaker(SpeakerProfile profile, OnDestroy on_destroy);

  void Apply(absl::Duration delta_time) override;

  SpeakerProfile profile_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_SPEAKER_H_
