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

#include "redux/engines/audio/resonance/audio_asset_stream.h"

#include <cmath>

#include "absl/functional/bind_front.h"
#include "resonance_audio/dsp/resampler.h"

namespace redux {

// 64 buffers correspond to 0.5 and 1.3 seconds of audio for 512 and 1024
// frames per buffer and a sample rate of 48000 Hz respectively.
static const size_t kNumberFifoBuffers = 64;

// Number of available buffers in which indicates a "low water mark" signal for
// the buffer stock to be refilled.
static const size_t kRefillOnBufferCount = kNumberFifoBuffers;

AudioAssetStream::AudioAssetStream(
    const std::shared_ptr<ResonanceAudioAsset>& asset,
    const SpeakerProfile& speaker_profile)
    : asset_(asset),
      looping_enabled_(false),
      end_of_stream_(false),
      playhead_position_(0),
      stream_started_(false),
      pending_seek_(false),
      pending_seek_position_(0),
      crossfade_size_frames_(0),
      channel_count_(0),
      frames_per_buffer_(speaker_profile.frames_per_buffer),
      system_sample_rate_hz_(speaker_profile.sample_rate_hz),
      num_stock_fifo_buffers_to_fill_(kNumberFifoBuffers),
      active_stock_buffer_(nullptr) {
  // Take over the asset's reader and the decoding process.
  reader_ = asset_->AcquireReader();

  if (reader_) {
    channel_count_ = reader_->GetNumChannels();
    if (asset_->ShouldStreamIntoMemory()) {
      planar_data_ = std::make_unique<AudioPlanarData>(channel_count_);
    }

    // Make sure we're starting from the beginning.
    reader_->Reset();
  } else {
    CHECK(asset_->GetPlanarData() != nullptr);
    channel_count_ = asset_->GetPlanarData()->GetNumChannels();
  }
}

AudioAssetStream::~AudioAssetStream() {
  if (planar_data_ != nullptr) {
    if (end_of_stream_.load()) {
      asset_->SetAudioPlanarData(std::move(planar_data_));
      reader_.reset();
    }
  }

  // Return the reader to the asset.
  asset_->SetAudioReader(std::move(reader_));
}

bool AudioAssetStream::Initialize() {
  output_buffer_ = std::make_unique<vraudio::AudioBuffer>(channel_count_,
                                                          frames_per_buffer_);

  if (asset_->GetStatus() == ResonanceAudioAsset::Status::kLoadedInMemory) {
    // Seamless loop size is set to 200ms (empirically obtained).
    static const float kDefaultSeamlessCrossfadeLengthSec = 0.2f;
    SetLoopCrossfadeDuration(kDefaultSeamlessCrossfadeLengthSec);
  } else {
    if (reader_ == nullptr) {
      LOG(ERROR) << "AudioAssetStream failed to create decoder.";
      return false;
    }
    total_frames_ = static_cast<size_t>(reader_->GetTotalFrameCount());
    stock_fifo_ =
        std::make_unique<AudioBufferFifo>(kNumberFifoBuffers, *output_buffer_);
    ConfigureResampler();
  }

  return true;
}

void AudioAssetStream::ConfigureResampler() {
  if (system_sample_rate_hz_ == reader_->GetSampleRateHz()) {
    return;
  }

  resampler_ = std::make_unique<vraudio::Resampler>();
  resampler_->SetRateAndNumChannels(reader_->GetSampleRateHz(),
                                    system_sample_rate_hz_, channel_count_);
  const size_t max_resampled_frames_per_buffer =
      resampler_->GetMaxOutputLength(frames_per_buffer_);
  resampler_input_ = std::make_unique<vraudio::AudioBuffer>(channel_count_,
                                                            frames_per_buffer_);
  resampler_output_ = std::make_unique<vraudio::AudioBuffer>(
      channel_count_, max_resampled_frames_per_buffer);
  partitioner_ = std::make_unique<vraudio::BufferPartitioner>(
      channel_count_, frames_per_buffer_,
      absl::bind_front(&AudioAssetStream::PartitionedBufferCallback, this));

  // Adjust the |stock_fifo_| fill to point so that buffer repartitioning
  // will not cause |stock_fifo_| overflow.
  num_stock_fifo_buffers_to_fill_ -= static_cast<size_t>(
      std::ceil(static_cast<float>(max_resampled_frames_per_buffer) /
                static_cast<float>(frames_per_buffer_)));
}

uint64_t AudioAssetStream::GetNumChannels() const { return channel_count_; }

int AudioAssetStream::GetSampleRateHz() const { return system_sample_rate_hz_; }

void AudioAssetStream::EnableLooping(bool looping_enabled) {
  looping_enabled_ = looping_enabled;
}

bool AudioAssetStream::GetNextAudioBuffer(
    const vraudio::AudioBuffer** output_buffer) {
  DCHECK(output_buffer);
  if (asset_->GetStatus() == ResonanceAudioAsset::Status::kLoadedInMemory) {
    return GetNextAudioBufferFromMemory(output_buffer);
  } else {
    return GetNextAudioBufferFromPrestockQueue(output_buffer);
  }
}

bool AudioAssetStream::GetNextAudioBufferFromMemory(
    const vraudio::AudioBuffer** output_buffer) {
  if (end_of_stream_.load()) {
    return false;
  }

  const AudioPlanarData* planar_data = asset_->GetPlanarData();
  const size_t total_frames = planar_data->GetFrameCount();
  DCHECK_LT(0, total_frames);

  size_t num_frames_read = 0;
  while (num_frames_read < frames_per_buffer_) {
    const size_t read_offset = playhead_position_.load();
    CHECK_LE(read_offset, total_frames);

    // Calculate the number of frames we can safely copy from the stream to the
    // output buffer.
    const size_t available_frames = total_frames - read_offset;
    const size_t num_frames_left_in_buffer =
        frames_per_buffer_ - num_frames_read;
    const size_t num_frames_to_copy =
        std::min<size_t>(available_frames, num_frames_left_in_buffer);

    // Copy some frames.
    for (size_t i = 0; i < output_buffer_->num_channels(); ++i) {
      absl::Span<const float> src = planar_data->GetChannelData(i);
      auto& dst = (*output_buffer_)[i];
      std::copy_n(&src[read_offset], num_frames_to_copy, &dst[num_frames_read]);
    }

    // Do wrap-around seamless looping if looping is enabled.
    if (looping_enabled_.load() &&
        available_frames - num_frames_to_copy < crossfade_size_frames_.load()) {
      SeamlessLoopCrossfade(num_frames_to_copy, output_buffer_.get());
    }

    // Advance the playhead_position by the number of frames we've copied.
    // Keep the position within the bounds, so a position of 0 is the end of
    // the stream.
    playhead_position_ = playhead_position_.load() + num_frames_to_copy;
    playhead_position_ = playhead_position_.load() % total_frames;
    if (playhead_position_.load() == 0 && !looping_enabled_.load()) {
      end_of_stream_ = true;
    }

    num_frames_read += num_frames_to_copy;
    if (num_frames_read < frames_per_buffer_) {
      if (looping_enabled_ && num_frames_to_copy > 0) {
        // If looping is enabled, begin a crossfade near the end of the asset
        // sample buffer.
        playhead_position_ = crossfade_size_frames_.load();
      } else {
        // If looping is disabled, silence the remaining buffer and mark the
        // stream as invalid.
        for (vraudio::AudioBuffer::Channel& channel : *output_buffer_) {
          std::fill(channel.begin() + num_frames_read, channel.end(), 0.0f);
        }
        num_frames_read = frames_per_buffer_;
        end_of_stream_ = true;
      }
    }
  }

  *output_buffer = output_buffer_.get();
  stream_started_ = true;
  return true;
}

bool AudioAssetStream::GetNextAudioBufferFromPrestockQueue(
    const vraudio::AudioBuffer** output_buffer) {
  // Push the last buffer back into the fifo queue.
  if (active_stock_buffer_ != nullptr) {
    stock_fifo_->ReleaseOutputObject(active_stock_buffer_);
    active_stock_buffer_ = nullptr;
  }

  if (stock_fifo_->Size() == 0) {
    // There are no more buffers because we are done.
    if (end_of_stream_.load()) {
      return false;
    }

    if (stream_started_.load()) {
      *output_buffer = nullptr;
      LOG(WARNING) << "Stream underflow at play position "
                   << playhead_position_.load() << " of "
                   << total_frames_;
    }
    // Return true in the case of an under-run so that the stream isn't
    // stopped. Under-runs may occur because of performance issues.
    return true;
  }

  // Get the next (filled) buffer from the fifo queue.
  active_stock_buffer_ = stock_fifo_->AcquireOutputObject();
  DCHECK(active_stock_buffer_);

  if (planar_data_ != nullptr) {
    planar_data_->AppendData(*active_stock_buffer_);
  }

  // Advance the play position by the number of frames in the buffer.
  playhead_position_ = playhead_position_.load() + frames_per_buffer_;

  // Loop the playhead position back to the start if we're looping.
  if (looping_enabled_) {
    if (total_frames_ > 0) {
      playhead_position_ =
          playhead_position_.load() % total_frames_;
    } else {
      playhead_position_ = 0;
    }
  }

  stream_started_ = true;
  *output_buffer = active_stock_buffer_;
  return true;
}

bool AudioAssetStream::IsPrestockServiceNeeded() const {
  if (asset_->GetStatus() == ResonanceAudioAsset::Status::kLoadedInMemory) {
    // Do not prestock buffers if asset is in memory.
    return false;
  }
  if (end_of_stream_.load()) {
    return false;
  }

  return pending_seek_.load() || stock_fifo_->Size() < kRefillOnBufferCount;
}

void AudioAssetStream::ServicePrestock() {
  DCHECK(asset_->GetStatus() != ResonanceAudioAsset::Status::kLoadedInMemory);
  DCHECK(reader_ != nullptr);

  // Perform any pending seek before decoding more PCM data.
  if (pending_seek_.load()) {
    pending_seek_ = false;
    reader_->SeekToFramePosition(pending_seek_position_.load());
  }

  // Decode additional data to refill the queue.
  while (stock_fifo_->Size() < num_stock_fifo_buffers_to_fill_) {
    if (!StockNextBufferFromReader()) {
      return;
    }
  }
}

bool AudioAssetStream::StockNextBufferFromReader() {
  if (reader_->IsAtEndOfStream()) {
    if (looping_enabled_.load()) {
      // Rewind to beginning of asset if looping is enabled.
      if (reader_->SeekToFramePosition(0) != 0) {
        LOG(ERROR) << "Could not perform loop back to position zero.";
      }
    } else {
      // Flush the partitioner, if necessary.
      if (partitioner_ != nullptr) {
        partitioner_->Flush();
      }

      // If end of stream has been reached and not in looping mode, prevent
      // the decoder from being called for more data. Some decoders (namely
      // SLES) will block until timeout if they have no more data available.
      end_of_stream_ = true;
      return false;
    }
  }

  if (resampler_ != nullptr) {
    // |AudioBuffer| from decoder must be resampled and repartitioned, with
    // the resulting buffer delivered via the |PartitionedBufferCallback|
    // function.
    const size_t frames_decoded =
        AudioPlanarData::ReadNextAudioBufferFromReader(reader_.get(),
                                                       resampler_input_.get());
    if (frames_decoded == 0) {
      end_of_stream_ = true;
      return false;
    }

    const size_t resampled_buffer_size =
        resampler_->GetNextOutputLength(frames_decoded);
    resampler_->Process(*resampler_input_, resampler_output_.get());

    partitioner_->AddBuffer(resampled_buffer_size, *resampler_output_);
  } else {
    // No resampling is required. |AudioBuffer| from |stock_fifo_| can be
    // directly passed into |GetNextBuffer|.
    vraudio::AudioBuffer* buffer = stock_fifo_->AcquireInputObject();
    if (buffer != nullptr) {
      const size_t frames_decoded =
          AudioPlanarData::ReadNextAudioBufferFromReader(reader_.get(), buffer);
      if (frames_decoded == 0) {
        LOG(WARNING) << "DecodeToBuffer returned zero samples before EOS.";
        buffer->Clear();
      }
      stock_fifo_->ReleaseInputObject(buffer);
    }
  }
  return true;
}

vraudio::AudioBuffer* AudioAssetStream::PartitionedBufferCallback(
    vraudio::AudioBuffer* buffer) {
  if (buffer != nullptr) {
    stock_fifo_->ReleaseInputObject(buffer);
  }
  vraudio::AudioBuffer* next = stock_fifo_->AcquireInputObject();
  if (next == nullptr) {
    LOG(WARNING) << "Failed to get free buffer from stock_fifo_.";
  }
  return next;
}

bool AudioAssetStream::Seek(float position_seconds) {
  if (planar_data_ != nullptr) {
    // Seeking is not allowed for assets of this type until they have streamed
    // once from start to finish.
    return false;
  } else if (asset_->GetStatus() ==
             ResonanceAudioAsset::Status::kLoadedInMemory) {
    const AudioPlanarData* planar_data = asset_->GetPlanarData();
    const size_t seek_frame_position =
        static_cast<size_t>(position_seconds * system_sample_rate_hz_);
    const size_t total_frames = planar_data->GetFrameCount();
    if (seek_frame_position <= total_frames) {
      playhead_position_ = seek_frame_position;
      return true;
    }
  } else if (asset_->GetStatus() ==
             ResonanceAudioAsset::Status::kReadyForStreaming) {
    // For a streamed asset, set a pending seek frame position for a seek
    // which will occur when the next |ServicePrestock| call occurs.
    const size_t tentative_seek_frame_position =
        static_cast<size_t>(position_seconds * reader_->GetSampleRateHz());
    if (tentative_seek_frame_position <= total_frames_) {
      pending_seek_position_ = tentative_seek_frame_position;
      pending_seek_ = true;

      // TODO(b/62629658) For instant seek operation, it would be preferable to
      // clear the |stock_fifo_| to prevent already stocked data from being
      // played before the seeked data. The fifo will be restocked from the
      // |pending_seek_position_| on the next call to |ServicePrestock|.

      return true;
    }
  }
  return false;
}

void AudioAssetStream::SetLoopCrossfadeDuration(float loop_crossfade_seconds) {
  crossfade_size_frames_ = 0;

  const AudioPlanarData* planar_data = asset_->GetPlanarData();
  if (planar_data == nullptr) {
    return;
  }

  const uint64_t total_frames = planar_data->GetFrameCount();
  if (total_frames == 0) {
    return;
  }

  const size_t num_crossfade_frames = static_cast<uint64_t>(
      loop_crossfade_seconds * static_cast<float>(system_sample_rate_hz_));
  // To be able to render a seamless loop, we need at least two samples.
  crossfade_size_frames_ =
      std::min<size_t>(num_crossfade_frames, total_frames - 1);
}

void AudioAssetStream::SeamlessLoopCrossfade(
    size_t num_frames_to_copy, vraudio::AudioBuffer* target_buffer) {
  size_t source_buffer_play_position = playhead_position_.load();
  size_t crossfade_size_frames = crossfade_size_frames_.load();

  // Beginning of loop tail in source buffer that fades out.
  const AudioPlanarData* planar_data = asset_->GetPlanarData();
  const size_t tail_begin_in_source_buffer =
      planar_data->GetFrameCount() - crossfade_size_frames;

  // Beginning of loop head in source buffer that fades in.
  size_t head_begin_in_source_buffer = 0;
  // Beginning of loop in target buffer.
  size_t offset_in_target_buffer =
      tail_begin_in_source_buffer - source_buffer_play_position;
  // Crossfade percentage at beginning of loop in target buffer.
  float crossfade_percentage = 0.0f;

  if (source_buffer_play_position >= tail_begin_in_source_buffer) {
    // Loop started in one of the previous buffers.
    // TODO(b/33060500) Consider an energy preserving crossfade window.
    head_begin_in_source_buffer =
        source_buffer_play_position - tail_begin_in_source_buffer;
    crossfade_percentage = static_cast<float>(head_begin_in_source_buffer) /
                           static_cast<float>(crossfade_size_frames);
    offset_in_target_buffer = 0;
  }

  DCHECK_LE(static_cast<size_t>(offset_in_target_buffer), num_frames_to_copy);
  const size_t num_loop_frames_to_process =
      num_frames_to_copy - offset_in_target_buffer;

  for (size_t channel = 0; channel < target_buffer->num_channels(); ++channel) {
    const float* source_channel_ptr =
        planar_data->GetChannelData(channel).data();
    float* target_channel_ptr = &(*target_buffer)[channel][0];
    for (size_t frame = 0; frame < num_loop_frames_to_process; ++frame) {
      const float crossfade_factor =
          crossfade_percentage +
          static_cast<float>(frame) / static_cast<float>(crossfade_size_frames);
      target_channel_ptr[offset_in_target_buffer + frame] =
          crossfade_factor *
              source_channel_ptr[head_begin_in_source_buffer + frame] +
          (1.0f - crossfade_factor) *
              target_channel_ptr[offset_in_target_buffer + frame];
    }
  }
}

}  // namespace redux
