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

#ifndef REDUX_MODULES_AUDIO_OPUS_READER_H_
#define REDUX_MODULES_AUDIO_OPUS_READER_H_

#include <vector>

#include "opusfile.h"
#include "redux/modules/audio/audio_reader.h"
#include "redux/modules/base/data_reader.h"

namespace redux {

// Stream-like API for Ogg Opus files.
class OpusReader : public AudioReader {
 public:
  explicit OpusReader(DataReader reader);
  ~OpusReader() override;

  // Resets the reader to a just-initialized state. Should be run if an error
  // occurs or if the reader is going to be reused to decode the stream again.
  // Implies a seek back to start of the stream.
  void Reset() override;

  // Queries the reader to determine if it contains a valid stream.
  bool IsValid() const override;

  // Queries the reader to determine if the end of stream has been reached.
  bool IsAtEndOfStream() const override;

  // Attempts a seek to frame position within the stream. Returns the frame
  // position to which the reader was actually able to seek.
  uint64_t SeekToFramePosition(uint64_t position) override;

  // Gets the position of the data frame from which the next Read operation will
  // take place.
  uint64_t GetReadFramePosition() const override { return current_frame_; }

  // Returns the total number of data frames in the stream. May return 0 if
  // unknown.
  uint64_t GetTotalFrameCount() const override { return total_frames_; }

  // Returns the number of audio channels in the stream.
  uint64_t GetNumChannels() const override { return num_channels_; }

  // Returns the sample rate (in hertz) of the audio data.
  int GetSampleRateHz() const override { return sample_rate_hz_; }

  // Returns the number of bytes required to store each frame.
  size_t GetNumBytesPerFrame() const {
    return bytes_per_sample_ * num_channels_;
  }

  // Returns the format in which the frame data is encoded.
  EncodingFormat GetEncodingFormat() const override {
    return EncodingFormat::kFloat;
  }

  // Reads up to `num_frames` of audio data.
  absl::Span<const std::byte> ReadFrames(uint64_t num_frames) override;

  // Checks the data reader to see if it contains a opus .ogg header.
  static bool CheckHeader(DataReader& reader);

 private:
  void Close();

  int sample_rate_hz_ = -1;
  uint64_t num_channels_ = 0;
  uint64_t bytes_per_sample_ = 0;
  uint64_t current_frame_ = 0;
  uint64_t total_frames_ = 0;
  OggOpusFile* opus_file_ = nullptr;
  DataReader reader_;
  std::vector<std::byte> read_buffer_;
};

}  // namespace redux

#endif  // REDUX_MODULES_AUDIO_OPUS_READER_H_
