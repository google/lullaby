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

#include "redux/modules/audio/wav_reader.h"

#include "redux/modules/base/logging.h"

namespace redux {
namespace {

// Supported WAV encoding formats.
static const uint16_t kPcmFormat = 0x1;
static const uint16_t kExtensibleWavFormat = 0xfffe;

struct ChunkHeader {
  char id[4];
  uint32_t size;
};

struct WavFormat {
  ChunkHeader header;
  uint16_t format_tag;
  uint16_t num_channels;
  uint32_t samples_rate;
  uint32_t average_bytes_per_second;
  uint16_t block_align;
  uint16_t bits_per_sample;
};

struct WavHeader {
  struct Riff {
    ChunkHeader header;
    char format[4];
  };
  struct DataChunk {
    ChunkHeader header;
  };

  Riff riff;
  WavFormat format;
  DataChunk data;
};

}  // namespace

WavReader::WavReader(DataReader reader) : reader_(std::move(reader)) {
  if (!ParseHeader()) {
    reader_ = DataReader();
  }
}

void WavReader::Reset() { SeekToFramePosition(0); }

uint64_t WavReader::SeekToFramePosition(uint64_t position) {
  CHECK(reader_.IsOpen()) << "Wav data stream is closed.";

  current_frame_ = std::min(position, total_frames_);

  const uint64_t byte_offset =
      pcm_offset_bytes_ + (position * num_channels_ * bytes_per_sample_);
  reader_.SetCurrentPosition(byte_offset);
  return current_frame_;
}

absl::Span<const std::byte> WavReader::ReadFrames(uint64_t num_frames) {
  CHECK(reader_.IsOpen()) << "Wav data stream is closed.";

  uint64_t num_frames_read =
      std::min(total_frames_ - current_frame_, num_frames);
  uint64_t num_bytes_read = num_frames_read * bytes_per_sample_ * num_channels_;
  read_buffer_.resize(num_bytes_read);

  if (num_frames_read > 0) {
    num_bytes_read = reader_.Read(read_buffer_.data(), num_bytes_read);
    num_frames_read = num_bytes_read / (bytes_per_sample_ * num_channels_);
    read_buffer_.resize(num_bytes_read);
  }

  current_frame_ += num_frames_read;
  return read_buffer_;
}

bool WavReader::ParseHeader() {
  WavHeader header;
  if (reader_.Read(&header.riff, sizeof(header.riff)) != sizeof(header.riff)) {
    return false;
  }

  ChunkHeader chunk;
  const uint64_t chunk_size = sizeof(chunk);
  while (!reader_.IsAtEndOfStream()) {
    if (reader_.Read(&chunk, chunk_size) != chunk_size) {
      return false;
    }
    if (!memcmp(chunk.id, "fmt ", 4)) {
      header.format.header = chunk;
      break;
    }
    if (reader_.Advance(chunk.size) != chunk.size) {
      return false;
    }
  }

  // We've already read the format ChunkHeader.  Now read the rest of the
  // WavHeader, starting from the format_tag.
  const uint64_t header_size_remaining = sizeof(header.format) - sizeof(chunk);
  if (reader_.Read(&header.format.format_tag, header_size_remaining) !=
      header_size_remaining) {
    return false;
  }

  const uint32_t format_size = header.format.header.size;
  const uint64_t kFormatSubChunkHeader =
      sizeof(WavFormat) - sizeof(ChunkHeader);
  if (format_size < kFormatSubChunkHeader) {
    return false;
  }
  if (format_size > kFormatSubChunkHeader) {
    // Skip extension data.
    reader_.Advance(format_size - kFormatSubChunkHeader);
  }

  if (header.format.format_tag == kExtensibleWavFormat) {
    // Skip extensible wav format "fact" header.  It is assumed that the
    // "fact" header comes before the "data" header.
    ChunkHeader fact_header;
    while (!reader_.IsAtEndOfStream()) {
      if (reader_.Read(&fact_header, chunk_size) != chunk_size) {
        return false;
      }
      if (!memcmp(fact_header.id, "fact", 4)) {
        break;
      }
      reader_.Advance(fact_header.size);
    }
  }

  // Read the "data" header.
  while (!reader_.IsAtEndOfStream()) {
    if (reader_.Read(&header.data, sizeof(header.data)) !=
        sizeof(header.data)) {
      return false;
    }
    if (!memcmp(header.data.header.id, "data", 4)) {
      break;
    }
    if (reader_.Advance(header.data.header.size) != header.data.header.size) {
      return false;
    }
  }

  num_channels_ = header.format.num_channels;
  sample_rate_hz_ = header.format.samples_rate;
  bytes_per_sample_ = header.format.bits_per_sample / 8;
  const uint64_t bytes_in_payload = header.data.header.size;

  if (sample_rate_hz_ <= 0) {
    return false;
  } else if (num_channels_ == 0) {
    return false;
  } else if (bytes_in_payload == 0) {
    return false;
  } else if (bytes_per_sample_ != sizeof(uint16_t)) {
    return false;
  } else if (bytes_in_payload % bytes_per_sample_ != 0) {
    return false;
  } else if (header.format.format_tag != kPcmFormat &&
             header.format.format_tag != kExtensibleWavFormat) {
    return false;
  } else if (std::string(header.riff.header.id, 4) != "RIFF") {
    return false;
  } else if (std::string(header.riff.format, 4) != "WAVE") {
    return false;
  } else if (std::string(header.format.header.id, 4) != "fmt ") {
    return false;
  } else if (std::string(header.data.header.id, 4) != "data") {
    return false;
  }

  auto num_total_samples = bytes_in_payload / bytes_per_sample_;
  total_frames_ = static_cast<uint64_t>(num_total_samples / num_channels_);
  pcm_offset_bytes_ = static_cast<uint64_t>(reader_.GetCurrentPosition());
  return true;
}

bool WavReader::CheckHeader(DataReader& reader) {
  struct Header {
    char riff[4];
    uint32_t size;
    char wave[4];
  } header;

  CHECK_EQ(reader.GetCurrentPosition(), 0);
  if (reader.GetTotalLength() < sizeof(header)) {
    return false;
  }

  reader.Read(&header, sizeof(header));
  reader.SetCurrentPosition(0);

  if (memcmp(header.riff, "RIFF", sizeof(header.riff))) {
    return false;
  }
  if (memcmp(header.wave, "WAVE", sizeof(header.wave))) {
    return false;
  }
  return true;
}

}  // namespace redux
