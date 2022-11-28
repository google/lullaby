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

#include "redux/engines/audio/resonance/audio_planar_data.h"

#include "redux/modules/audio/audio_reader.h"
#include "resonance_audio/dsp/resampler.h"
#include "resonance_audio/utils/planar_interleaved_conversion.h"

namespace redux {

AudioPlanarData::AudioPlanarData(size_t num_channels)
    : channels_(num_channels) {}

void AudioPlanarData::Reserve(size_t num_frames) {
  for (auto& channel : channels_) {
    channel.reserve(num_frames);
  }
}

uint64_t AudioPlanarData::GetFrameCount() const {
  return channels_.empty() ? 0 : channels_.front().size();
}

uint64_t AudioPlanarData::GetNumChannels() const {
  return static_cast<uint64_t>(channels_.size());
}

absl::Span<const float> AudioPlanarData::GetChannelData(size_t index) const {
  return index < channels_.size() ? channels_[index]
                                  : absl::Span<const float>();
}

void AudioPlanarData::AppendData(const vraudio::AudioBuffer& source) {
  AppendData(source, source.num_frames());
}

void AudioPlanarData::AppendData(const vraudio::AudioBuffer& source,
                                 size_t num_frames) {
  CHECK_EQ(source.num_channels(), channels_.size());
  for (size_t channel = 0; channel < channels_.size(); ++channel) {
    auto& dst = channels_[channel];
    const auto& src = source[channel];
    dst.insert(dst.end(), src.begin(), src.begin() + num_frames);
  }
}

std::unique_ptr<AudioPlanarData> AudioPlanarData::FromReader(
    AudioReader& reader, const SpeakerProfile& profile) {
  const int num_channels = reader.GetNumChannels();
  auto planar_data = std::make_unique<AudioPlanarData>(num_channels);

  // See if we need to use a Resampler to match the sample rate of loaded audio
  // samples with the system sample rate.
  std::unique_ptr<vraudio::Resampler> resampler;
  std::unique_ptr<vraudio::AudioBuffer> resampled_buffer;
  const int asset_sample_rate_hz = reader.GetSampleRateHz();
  if (asset_sample_rate_hz != profile.sample_rate_hz) {
    resampler = std::make_unique<vraudio::Resampler>();
    resampler->SetRateAndNumChannels(asset_sample_rate_hz,
                                     profile.sample_rate_hz,
                                     num_channels);
  }

  // Loop to perform stream decoding, as not all reader types support a query
  // for decoded frame count.
  vraudio::AudioBuffer temp_buffer(num_channels, profile.frames_per_buffer);

  while (!reader.IsAtEndOfStream()) {
    vraudio::AudioBuffer* source_buffer = &temp_buffer;
    size_t frames_decoded =
        ReadNextAudioBufferFromReader(&reader, source_buffer);
    if (frames_decoded == 0) {
      break;
    }

    CHECK(reader.IsAtEndOfStream() ||
          frames_decoded == profile.frames_per_buffer);

    if (resampler != nullptr) {
      const size_t resampled_size =
          resampler->GetNextOutputLength(frames_decoded);
      if (resampled_buffer == nullptr ||
          resampled_buffer->num_frames() != resampled_size) {
        resampled_buffer = std::make_unique<vraudio::AudioBuffer>(
            num_channels, resampled_size);
      }

      if (frames_decoded != source_buffer->num_frames()) {
        vraudio::AudioBuffer temp_buffer2(num_channels, frames_decoded);
        for (size_t i = 0; i < num_channels; ++i) {
          std::copy_n((*source_buffer)[i].begin(), frames_decoded,
                      temp_buffer2[i].begin());
        }
        resampler->Process(temp_buffer2, resampled_buffer.get());
      } else {
        resampler->Process(*source_buffer, resampled_buffer.get());
      }

      source_buffer = resampled_buffer.get();
      frames_decoded = resampled_buffer->num_frames();
    }

    planar_data->AppendData(*source_buffer, frames_decoded);
  }
  return planar_data;
}

size_t AudioPlanarData::ReadNextAudioBufferFromReader(
    AudioReader* reader, vraudio::AudioBuffer* buffer) {
  const absl::Span<const std::byte> bytes =
      reader->ReadFrames(buffer->num_frames());
  const size_t num_frames = bytes.size() / reader->GetNumBytesPerFrame();
  const int num_channels = reader->GetNumChannels();

  const auto format = reader->GetEncodingFormat();
  if (format == AudioReader::kFloat) {
    const float* data = reinterpret_cast<const float*>(bytes.data());
    vraudio::FillAudioBuffer(data, num_frames, num_channels, buffer);
  } else if (format == AudioReader::kInt16) {
    const int16_t* data = reinterpret_cast<const int16_t*>(bytes.data());
    vraudio::FillAudioBuffer(data, num_frames, num_channels, buffer);
  }
  return num_frames;
}
}  // namespace redux
