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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_ASSET_STREAM_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_ASSET_STREAM_H_

#include <atomic>
#include <cstdio>
#include <memory>
#include <vector>

#include "redux/engines/audio/resonance/audio_source_stream.h"
#include "redux/engines/audio/resonance/resonance_audio_asset.h"
#include "redux/modules/audio/audio_reader.h"
#include "resonance_audio/base/audio_buffer.h"
#include "resonance_audio/dsp/resampler.h"
#include "resonance_audio/utils/buffer_partitioner.h"
#include "resonance_audio/utils/threadsafe_fifo.h"

namespace redux {

// AudioSourceStream for an AudioAsset.
//
// There are two ways in which this AudioSourceStream can provide AudioBuffers
// for the audio device.
//
// The simplest is when the AudioAsset stores the decoded, planar audio data in
// a memory buffer. In this case, we can just read the data directly when
// needed.
//
// The more complicated case is when the AudioAsset is only able to stream data
// using an AudioReader. In this case, we try to asynchronously read and decode
// the audio stream and store a "stock" of audio buffers into a thread-safe
// FIFO queue. Then, when we get a request for data, we can return the next
// buffer from the queue.
//
// Managing this stock of buffers requires coordination with the streaming
// thread managed by the AudioStreamManager.
class AudioAssetStream : public AudioSourceStream {
 public:
  // Creates the AudioSourceStream around the AudioAsset. The audio will be
  // streamed to match the speaker device requirements.
  AudioAssetStream(const std::shared_ptr<ResonanceAudioAsset>& asset,
                   const SpeakerProfile& speaker_profile);

  ~AudioAssetStream() override;

  AudioAssetStream(const AudioAssetStream&) = delete;
  AudioAssetStream& operator=(const AudioAssetStream&) = delete;

  // Initializes the audio source stream.
  bool Initialize() override;

  // Returns true if the end of the stream has been reached.
  bool EndOfStreamReached() override { return end_of_stream_.load(); }

  // Populates `output_buffer` with data from the audio stream. Sets
  // `output_buffer` to nullptr if a buffer underrun occurs. Returns false if
  // there is no more data to be consumed.
  //
  // Should be called only from the audio thread.
  bool GetNextAudioBuffer(
      const vraudio::AudioBuffer** output_buffer) override;

  // Queries whether the asynchronously decoded stock of audio buffers needs to
  // be refilled. Should be called only from the audio thread.
  bool IsPrestockServiceNeeded() const override;

  // Refills the asynchronous stock of audio buffers. Should be called only from
  // the decode thread.
  void ServicePrestock() override;

  // Returns the number of audio channels in the audio stream.
  uint64_t GetNumChannels() const override;

  // Returns the sample rate of the audio stream. This must be the same as the
  // audio device sample rate.
  int GetSampleRateHz() const override;

  // Seeks to a target time position in the audio stream.
  bool Seek(float position_seconds) override;

  // Enables looped streaming from the audio source. Looping may not be
  // meaningful for all audio source types; usage depends on subclass.
  void EnableLooping(bool looping_enabled) override;

  // Sets the loop crossfade for the audio source if looping is enabled and
  // meaningful for the audio source type. If not called, the source will use a
  // default loop crossfade.
  void SetLoopCrossfadeDuration(float loop_crossfade_seconds) override;

 private:
  // Configures a Resampler and Partitioner if the sample rate of the input
  // asset differs from the sample rate required by the audio engine.
  void ConfigureResampler();

  // Partitioner callback for when a new buffer is completed.
  vraudio::AudioBuffer* PartitionedBufferCallback(vraudio::AudioBuffer* buffer);

  // Helper for `GetNextAudioBuffer` that gets the buffer from the asset's
  // memory cache.
  bool GetNextAudioBufferFromMemory(const vraudio::AudioBuffer** output_buffer);

  // Helper for `GetNextAudioBuffer` that gets the buffer from the prestock
  // queue.
  bool GetNextAudioBufferFromPrestockQueue(
      const vraudio::AudioBuffer** output_buffer);

  // Reads an audio buffer from the asset reader and pushes it into the prestock
  // queue.
  bool StockNextBufferFromReader();

  // Helper method which calculates a seamless loop by crossfading between the
  // end and the beginning of asset.
  void SeamlessLoopCrossfade(size_t num_frames_to_copy,
                             vraudio::AudioBuffer* target_buffer);

  // Shared pointer to the AudioAsset which will serve as the stream source.
  // This asset is externally owned.
  std::shared_ptr<ResonanceAudioAsset> asset_;

  // The AudioReader that can be used to stream the asset data.
  std::unique_ptr<redux::AudioReader> reader_;

  // The AudioPlanarData into which to store data as it is streamed and decoded.
  // This is only used if the asset was configured for streaming into memory.
  std::unique_ptr<AudioPlanarData> planar_data_;

  // A threadsafe FIFO queue that stores audio buffers that can be consumed by
  // calls to GetNextAudioBuffer.
  using AudioBufferFifo = vraudio::ThreadsafeFifo<vraudio::AudioBuffer>;
  std::unique_ptr<AudioBufferFifo> stock_fifo_;

  // Flag indicating looped playback.
  std::atomic<bool> looping_enabled_;

  // Indication that an input stream has been completely decoded (end of file).
  std::atomic<bool> end_of_stream_;

  // Current playhead frame position of the stream, as distinguished from the
  // decode position.  This would also be known as the next available frame in
  // the stock_fifo_.
  std::atomic<size_t> playhead_position_;

  // Indicates that the audio stream has started, so that mid-stream underflows
  // may be detected. This variable will be set to true once PCM data is
  // produced by this |AudioAssetStream|.
  std::atomic<bool> stream_started_;

  // Indicates that an asynchronous seek to frame position is pending.
  std::atomic<bool> pending_seek_;

  // The pending seek position to be set asynchronously. The actual seek will
  // occur in the |ServicePrestock| function.
  std::atomic<size_t> pending_seek_position_;

  // Size of seamless crossfade in frames.
  std::atomic<size_t> crossfade_size_frames_;

  // |AudioAsset| channel count.
  uint64_t channel_count_;

  // Frames per buffer.
  uint64_t frames_per_buffer_;

  // Audio system sample rate.
  int system_sample_rate_hz_;

  // Overall duration of an audio asset.
  size_t total_frames_;

  // The number of |stock_fifo_| buffers that the |ServicePrestock| routine
  // should try to fill. This number will be smaller than |kNumberFifoBuffers|
  // where a |partitioner_| is used.
  size_t num_stock_fifo_buffers_to_fill_;

  // Resampler and related objects for when asset sample rate does not match the
  // system sample rate.
  std::unique_ptr<vraudio::Resampler> resampler_;
  std::unique_ptr<vraudio::AudioBuffer> resampler_input_;
  std::unique_ptr<vraudio::AudioBuffer> resampler_output_;
  std::unique_ptr<vraudio::BufferPartitioner> partitioner_;

  // Preallocated output audio buffer.
  std::unique_ptr<vraudio::AudioBuffer> output_buffer_;

  // |GetNextAudioBuffer| returns a pointer to the |stock_fifo_| front buffer.
  // To ensure, the acquired and returned buffer is not reused, this pointer
  // holds the most recent output |AudioBuffer| which is used to release it
  // during the next |GetNextAudioBuffer| call.
  const vraudio::AudioBuffer* active_stock_buffer_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_ASSET_STREAM_H_
