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

#include "redux/engines/audio/resonance/audio_stream_renderer.h"

#include "resonance_audio/utils/planar_interleaved_conversion.h"

namespace redux {

// Number of buffers to process to allow ResonanceAudioApi to fade out and ramp
// down to the target volume of zero.
static const size_t kNumBuffersToProcessDuringFadeOut = 3;

AudioStreamRenderer::AudioStreamRenderer(
    SoundType type, std::unique_ptr<AudioSourceStream> stream,
    vraudio::ResonanceAudioApi* resonance,
    vraudio::RenderingMode rendering_mode)
    : stream_(std::move(stream)),
      resonance_(resonance),
      is_paused_(true),  //  initialize in paused state
      shutdown_triggered_(false),
      stream_volume_(1.0f),
      fade_out_count_down_(0),
      prestock_service_pending_(false) {
  CHECK(stream_ != nullptr);
  CHECK(resonance_ != nullptr);

  const auto num_channels = stream_->GetNumChannels();
  if (type == SoundType::Point) {
    CHECK(num_channels == vraudio::kNumMonoChannels);
  } else if (type == SoundType::Stereo) {
    CHECK(num_channels <= vraudio::kNumStereoChannels);
  }
  output_channels_.resize(num_channels);

  switch (type) {
    case SoundType::Point:
      source_id_ = resonance_->CreateSoundObjectSource(rendering_mode);
      break;
    case SoundType::Field:
      source_id_ = resonance_->CreateAmbisonicSource(num_channels);
      break;
    case SoundType::Stereo:
      source_id_ = resonance_->CreateStereoSource(num_channels);
      break;
  }
  CHECK(source_id_ != vraudio::kInvalidSourceId);
}

AudioStreamRenderer::~AudioStreamRenderer() {
  resonance_->DestroySource(source_id_);
}

void AudioStreamRenderer::Resume() {
  is_paused_ = false;
  resonance_->SetSourceVolume(source_id_, stream_volume_);
  fade_out_count_down_ = 0;
}

void AudioStreamRenderer::Pause() {
  resonance_->SetSourceVolume(source_id_, 0.0f);
  fade_out_count_down_ = kNumBuffersToProcessDuringFadeOut;
}

void AudioStreamRenderer::Shutdown() {
  Pause();
  shutdown_triggered_ = true;
}

void AudioStreamRenderer::SetVolume(float volume) {
  stream_volume_ = volume;
  if (!is_paused_) {
    resonance_->SetSourceVolume(source_id_, stream_volume_);
  }
}

bool AudioStreamRenderer::Render() {
  if (is_paused_) {
    return !shutdown_triggered_;
  }

  const vraudio::AudioBuffer* next_buffer = nullptr;
  if (!stream_->GetNextAudioBuffer(&next_buffer) || next_buffer == nullptr) {
    return !stream_->EndOfStreamReached();
  }

  // Get raw planar channel pointers from `next_buffer` to set input buffer in
  // `ResonanceAudioApi`.
  GetRawChannelDataPointersFromAudioBuffer(*next_buffer, &output_channels_);
  resonance_->SetPlanarBuffer(source_id_, output_channels_.data(),
                              next_buffer->num_channels(),
                              next_buffer->num_frames());

  if (fade_out_count_down_ > 0) {
    --fade_out_count_down_;
    is_paused_ = true;
  }
  return true;
}

bool AudioStreamRenderer::IsPrestockServiceNeeded() const {
  if (prestock_service_pending_.load()) {
    // We're already queued up for a prestock service, so we return false to
    // prevent being doubly-queued up.
    return false;
  }
  return stream_->IsPrestockServiceNeeded();
}

void AudioStreamRenderer::SetPrestockServicePending(bool pending) {
  prestock_service_pending_ = pending;
}

void AudioStreamRenderer::ServicePrestock() {
  stream_->ServicePrestock();
  prestock_service_pending_ = false;
}

}  // namespace redux
