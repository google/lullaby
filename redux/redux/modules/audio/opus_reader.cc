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

#include "redux/modules/audio/opus_reader.h"

#include <algorithm>

#include "redux/modules/base/logging.h"

namespace redux {

// Default frame size of buffers used for internal ogg decoding.
static const size_t kOggInternalBufferSize = 512;

static int OggOpusRead(void* stream, unsigned char* ptr, int nbytes) {
  auto* reader = reinterpret_cast<DataReader*>(stream);
  return static_cast<int>(reader->Read(ptr, nbytes));
}

static int OggOpusSeek(void* stream, opus_int64 offset, int origin) {
  auto* reader = reinterpret_cast<DataReader*>(stream);
  if (origin == SEEK_END) {
    offset = reader->GetTotalLength() + offset;
  } else if (origin == SEEK_CUR) {
    offset = reader->GetCurrentPosition() + offset;
  } else if (origin != SEEK_SET) {
    return -1;
  }
  reader->SetCurrentPosition(offset);
  return 0;
}

static opus_int64 OggOpusTell(void* stream) {
  auto* reader = reinterpret_cast<DataReader*>(stream);
  return static_cast<opus_int64>(reader->GetCurrentPosition());
}

static int OggOpusNoopClose(void* stream) { return 0; }

OpusReader::OpusReader(DataReader reader) : reader_(std::move(reader)) {
  OpusFileCallbacks callbacks = {OggOpusRead, OggOpusSeek, OggOpusTell,
                                 OggOpusNoopClose};

  int return_value = 0;
  opus_file_ =
      op_open_callbacks(&reader_, &callbacks, nullptr, 0, &return_value);
  if (return_value != 0) {
    Close();
    return;
  }

  const int current_link_index = op_current_link(opus_file_);
  const OpusHead* head = op_head(opus_file_, current_link_index);
  const uint64_t total_samples =
      static_cast<uint64_t>(op_pcm_total(opus_file_, -1));

  sample_rate_hz_ = head->input_sample_rate;
  num_channels_ = static_cast<uint64_t>(head->channel_count);
  total_frames_ = total_samples / num_channels_;
  bytes_per_sample_ = sizeof(float);
}

OpusReader::~OpusReader() { Close(); }

void OpusReader::Close() {
  if (opus_file_ != nullptr) {
    op_free(opus_file_);
    opus_file_ = nullptr;
  }
  reader_.Close();
}

void OpusReader::Reset() { SeekToFramePosition(0); }

bool OpusReader::IsValid() const {
  return opus_file_ != nullptr && reader_.IsOpen();
}

bool OpusReader::IsAtEndOfStream() const {
  return current_frame_ == total_frames_;
}

uint64_t OpusReader::SeekToFramePosition(uint64_t position) {
  CHECK(opus_file_ != nullptr);

  if (!op_seekable(opus_file_)) {
    LOG(ERROR) << "Attempt to seek into non-seekable opus stream.";
  } else if (position >= total_frames_) {
    LOG(ERROR) << "Seek out of range in opus stream";
  } else if (op_pcm_seek(opus_file_, static_cast<ogg_int64_t>(position)) < 0) {
    LOG(ERROR) << "Error seeking in opus stream";
  }
  current_frame_ = static_cast<uint64_t>(op_pcm_tell(opus_file_));
  return current_frame_;
}

absl::Span<const std::byte> OpusReader::ReadFrames(uint64_t num_frames) {
  CHECK(reader_.IsOpen()) << "Opus data stream is closed.";
  CHECK(opus_file_ != nullptr) << "Opus data stream is closed.";

  const uint64_t capacity = num_frames * bytes_per_sample_ * num_channels_;
  read_buffer_.resize(capacity);

  uint64_t frames_decoded = 0;
  float* ptr = reinterpret_cast<float*>(read_buffer_.data());
  while (frames_decoded < num_frames) {
    const uint64_t target_frame_count =
        std::min<uint64_t>(kOggInternalBufferSize, num_frames - frames_decoded);

    int samples_read = static_cast<int>(target_frame_count * num_channels_);
    samples_read = op_read_float(opus_file_, ptr, samples_read, nullptr);
    ptr += samples_read;

    const int frames_from_opus = samples_read / static_cast<int>(num_channels_);

    CHECK_GE(frames_from_opus, 0) << "Error decoding ogg-opus data.";
    if (frames_from_opus == 0) {
      // Have reached end of frame buffer, so just use what we have.
      break;
    }
    frames_decoded += frames_from_opus;
  }

  current_frame_ += frames_decoded;

  const uint64_t num_bytes_decoded =
      frames_decoded * bytes_per_sample_ * num_channels_;
  read_buffer_.resize(num_bytes_decoded);
  return read_buffer_;
}

bool OpusReader::CheckHeader(DataReader& reader) {
  struct Header {
    char ogg[4];
    char data[24];
    char opus[8];
  } header;

  CHECK_EQ(reader.GetCurrentPosition(), 0);
  if (reader.GetTotalLength() < sizeof(header)) {
    return false;
  }

  reader.Read(&header, sizeof(header));
  reader.SetCurrentPosition(0);

  if (memcmp(header.ogg, "OggS", sizeof(header.ogg))) {
    return false;
  }
  if (memcmp(header.opus, "OpusHead", sizeof(header.opus))) {
    return false;
  }
  return true;
}

}  // namespace redux
