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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_PLANAR_DATA_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_PLANAR_DATA_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "absl/types/span.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/modules/audio/audio_reader.h"
#include "resonance_audio/base/audio_buffer.h"

namespace redux {

// Storage for uncompressed planar audio data.
class AudioPlanarData {
 public:
  explicit AudioPlanarData(size_t num_channels);

  // Reserves memory in the buffer for the given number of frames.
  void Reserve(size_t num_frames);

  // Returns the total number of frames of audio data.
  uint64_t GetFrameCount() const;

  // Returns the number of channels of audio data.
  uint64_t GetNumChannels() const;

  // Adds decoded frames to the internal buffers.
  void AppendData(const vraudio::AudioBuffer& source);

  // Returns the data for the given channel. Returns an empty span if the
  // channel index is invalid.
  absl::Span<const float> GetChannelData(size_t index) const;

  // Decodes the entirety of an AudioReader into a AudioPlanarData object.
  static std::unique_ptr<AudioPlanarData> FromReader(
      AudioReader& reader, const SpeakerProfile& profile);

  // Populates the vraudio::AudioBuffer with data from the AudioReader.
  static size_t ReadNextAudioBufferFromReader(AudioReader* reader,
                                              vraudio::AudioBuffer* buffer);

 private:
  // Adds decoded frames to the internal PCM buffer.
  void AppendData(const vraudio::AudioBuffer& source, size_t num_frames);

  // Buffer containing uncompressed planar data.
  std::vector<std::vector<float>> channels_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_PLANAR_DATA_H_
