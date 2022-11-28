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

#ifndef REDUX_MODULES_AUDIO_AUDIO_READER_H_
#define REDUX_MODULES_AUDIO_AUDIO_READER_H_

#include <cstdint>
#include <functional>
#include <string>

#include "absl/types/span.h"

namespace redux {

// A generic streaming API for audio data.
class AudioReader {
 public:
  virtual ~AudioReader() = default;

  // Resets the reader to a just-initialized state. Should be run if an error
  // occurs or if the reader is going to be reused to decode the stream again.
  // Implies a seek back to start of the stream.
  virtual void Reset() = 0;

  // Queries the reader to determine if it contains a valid stream.
  virtual bool IsValid() const = 0;

  // Queries the reader to determine if the end of stream has been reached.
  virtual bool IsAtEndOfStream() const = 0;

  // Attempts a seek to frame position within the stream. Returns the frame
  // position to which the reader was actually able to seek.
  virtual uint64_t SeekToFramePosition(uint64_t position) = 0;

  // Gets the position of the data frame from which the next Read operation will
  // take place.
  virtual uint64_t GetReadFramePosition() const = 0;

  // Returns the total number of data frames in the stream. May return 0 if
  // unknown.
  virtual uint64_t GetTotalFrameCount() const = 0;

  // Returns the number of audio channels in the stream.
  virtual uint64_t GetNumChannels() const = 0;

  // Returns the sample rate (in hertz) of the audio data.
  virtual int GetSampleRateHz() const = 0;

  // Returns the number of bytes required to store each frame.
  virtual size_t GetNumBytesPerFrame() const = 0;

  // Returns the format in which the frame data is encoded.
  enum EncodingFormat { kFloat, kInt16 };
  virtual EncodingFormat GetEncodingFormat() const = 0;

  // Reads up to `num_frames` of audio data.
  virtual absl::Span<const std::byte> ReadFrames(uint64_t num_frames) = 0;
};

}  // namespace redux

#endif  // REDUX_MODULES_AUDIO_AUDIO_READER_H_
