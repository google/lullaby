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

#include "redux/modules/audio/vorbis_reader.h"

#include <algorithm>

#include "redux/modules/base/logging.h"

namespace redux {

// Default frame size of buffers used for internal ogg decoding.
static const uint64_t kOggInternalBufferSize = 512;

static size_t OggVorbisRead(void* ptr, size_t nbytes, size_t count,
                            void* stream) {
  auto* reader = reinterpret_cast<DataReader*>(stream);
  return reader->Read(ptr, nbytes * count);
}

static int OggVorbisSeek(void* stream, ogg_int64_t offset, int origin) {
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

static long OggVorbisTell(void* stream) {
  auto* reader = reinterpret_cast<DataReader*>(stream);
  return static_cast<long>(reader->GetCurrentPosition());
}

static int OggVorbisNoopClose(void* stream) { return 0; }

VorbisReader::VorbisReader(DataReader reader) : reader_(std::move(reader)) {
  ov_callbacks callbacks = {OggVorbisRead, OggVorbisSeek, OggVorbisNoopClose,
                            OggVorbisTell};

  int result = ov_open_callbacks(reinterpret_cast<void*>(&reader_),
                                 &vorbis_file_, nullptr, 0, callbacks);
  if (result != 0) {
    Close();
    return;
  }

  vorbis_info* info = ov_info(&vorbis_file_, -1);
  const uint64_t total_samples =
      static_cast<uint64_t>(ov_pcm_total(&vorbis_file_, -1));

  sample_rate_hz_ = static_cast<int>(info->rate);
  num_channels_ = static_cast<size_t>(info->channels);
  total_frames_ = total_samples / num_channels_;
  bytes_per_sample_ = sizeof(uint16_t);
}

VorbisReader::~VorbisReader() { Close(); }

void VorbisReader::Close() {
  ov_clear(&vorbis_file_);
  reader_.Close();
}

void VorbisReader::Reset() { SeekToFramePosition(0); }

bool VorbisReader::IsAtEndOfStream() const {
  return current_frame_ == total_frames_;
}

uint64_t VorbisReader::SeekToFramePosition(uint64_t position) {
  CHECK(reader_.IsOpen());

  if (!ov_seekable(&vorbis_file_)) {
    LOG(ERROR) << "Attempt to seek into non-seekable Vorbis stream.";
  } else if (position >= total_frames_) {
    LOG(ERROR) << "Seek out of range in Vorbis stream";
  } else if (ov_pcm_seek(&vorbis_file_, static_cast<ogg_int64_t>(position)) <
             0) {
    LOG(ERROR) << "Error seeking in Vorbis stream";
  }
  current_frame_ = static_cast<uint64_t>(ov_pcm_tell(&vorbis_file_));
  return current_frame_;
}

absl::Span<const std::byte> VorbisReader::ReadFrames(uint64_t num_frames) {
  CHECK(reader_.IsOpen());

  constexpr int kLittleEndian = 0;
  constexpr int k16BitSamples = 2;
  constexpr int kSignedSamples = 1;

  const uint64_t capacity = num_frames * bytes_per_sample_ * num_channels_;
  read_buffer_.resize(capacity);

  uint64_t frames_decoded = 0;
  char* ptr = reinterpret_cast<char*>(read_buffer_.data());
  while (frames_decoded < num_frames) {
    const uint64_t target_frame_count =
        std::min<uint64_t>(kOggInternalBufferSize, num_frames - frames_decoded);

    int bytes_read = static_cast<int>(target_frame_count * num_channels_ *
                                      bytes_per_sample_);
    bytes_read = ov_read(&vorbis_file_, ptr, bytes_read, kLittleEndian,
                         k16BitSamples, kSignedSamples, nullptr);

    ptr += bytes_read;

    const int frames_read =
        bytes_read / static_cast<int>(num_channels_ * bytes_per_sample_);
    CHECK_GE(frames_read, 0) << "Error decoding ogg-vorbis.";
    if (frames_read == 0) {
      break;
    }
    frames_decoded += frames_read;
  }

  current_frame_ += frames_decoded;

  const uint64_t num_bytes_decoded =
      frames_decoded * bytes_per_sample_ * num_channels_;
  read_buffer_.resize(num_bytes_decoded);
  return read_buffer_;
}

bool VorbisReader::CheckHeader(DataReader& reader) {
  struct Header {
    char ogg[4];
    char data[25];
    char vorbis[6];
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
  if (memcmp(header.vorbis, "vorbis", sizeof(header.vorbis))) {
    return false;
  }
  return true;
}

}  // namespace redux
