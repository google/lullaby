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

#include "redux/engines/platform/sdl2/sdl2_speaker.h"

#include "SDL.h"
#include "redux/engines/platform/device_manager.h"

namespace redux {

// Use standard 48 khz audio frequency for the speaker.
constexpr int kAudioFrequency = 48000;

// Use stereo output for the speakers.
constexpr int kAudioChannels = 2;

// Audio buffer will be signed 16-bit integer data.
constexpr auto kAudioFormat = AUDIO_S16;

// The size of the audio buffer. 2kb provides reasonable performance.
constexpr int kAudioSamples = 2048;

void SdlAudioCallback(void* userdata, uint8_t* stream, int len) {
  Speaker::HwBuffer hw_buffer = absl::MakeSpan(stream, len);
  reinterpret_cast<DeviceManager*>(userdata)->AudioHwCallback(hw_buffer);
}

Sdl2Speaker::Sdl2Speaker(DeviceManager* dm) : device_manager_(dm) {
  if (!SDL_WasInit(SDL_INIT_AUDIO)) {
    CHECK(SDL_Init(SDL_INIT_AUDIO) == 0) << SDL_GetError();
  }

  SDL_memset(&sdl_audio_spec_, 0, sizeof(sdl_audio_spec_));
  sdl_audio_spec_.freq = kAudioFrequency;
  sdl_audio_spec_.format = kAudioFormat;
  sdl_audio_spec_.channels = kAudioChannels;
  sdl_audio_spec_.samples = kAudioSamples;
  sdl_audio_spec_.callback = SdlAudioCallback;
  sdl_audio_spec_.userdata = device_manager_;

  sdl_device_id_ = SDL_OpenAudioDevice(nullptr, 0, &sdl_audio_spec_, nullptr,
                                       SDL_AUDIO_ALLOW_ANY_CHANGE);
  CHECK(sdl_device_id_ != 0) << SDL_GetError();

  SpeakerProfile profile;
  profile.sample_rate_hz = kAudioFrequency;
  profile.num_channels = kAudioChannels;
  profile.frames_per_buffer = kAudioSamples;

  speaker_ = dm->Connect(profile);

  // Start the audio playback. The device manager will fill in empty data if
  // there is no audio source.
  SDL_PauseAudioDevice(sdl_device_id_, 0);
}

Sdl2Speaker::~Sdl2Speaker() {
  SDL_CloseAudioDevice(sdl_device_id_);
  speaker_.reset();
}

void Sdl2Speaker::Commit() {}

}  // namespace redux
