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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_SOURCE_STREAM_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_SOURCE_STREAM_H_

#include <cstdint>

#include "resonance_audio/base/audio_buffer.h"

namespace redux {

// Base interface of an audio stream capable of providing audio data
// synchronously or asynchronously to be fed into the audio device.
class AudioSourceStream {
 public:
  AudioSourceStream() = default;
  virtual ~AudioSourceStream() = default;

  AudioSourceStream(const AudioSourceStream&) = delete;
  AudioSourceStream& operator=(const AudioSourceStream&) = delete;

  // Initializes the audio source stream.
  virtual bool Initialize() = 0;

  // Returns true if the end of the stream has been reached.
  virtual bool EndOfStreamReached() = 0;

  // Populates `output_buffer` with data from the audio stream. Sets
  // `output_buffer` to nullptr if a buffer underrun occurs. Returns false if
  // there is no more data to be consumed.
  //
  // Should be called only from the audio thread.
  virtual bool GetNextAudioBuffer(
      const vraudio::AudioBuffer** output_buffer) = 0;

  // Queries whether the asynchronously decoded stock of audio buffers needs to
  // be refilled. Should be called only from the audio thread.
  virtual bool IsPrestockServiceNeeded() const = 0;

  // Refills the asynchronous stock of audio buffers. Should be called only from
  // the decode thread.
  virtual void ServicePrestock() = 0;

  // Returns the number of audio channels in the audio stream.
  virtual uint64_t GetNumChannels() const = 0;

  // Returns the sample rate of the audio stream. This must be the same as the
  // audio device sample rate.
  virtual int GetSampleRateHz() const = 0;

  // Seeks to a target time position in the audio stream.
  virtual bool Seek(float position_seconds) = 0;

  // Enables looped streaming from the audio source. Looping may not be
  // meaningful for all audio source types; usage depends on subclass.
  virtual void EnableLooping(bool looping_enabled) = 0;

  // Sets the loop crossfade for the audio source if looping is enabled and
  // meaningful for the audio source type. If not called, the source will use a
  // default loop crossfade.
  virtual void SetLoopCrossfadeDuration(float loop_crossfade_seconds) = 0;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_SOURCE_STREAM_H_
