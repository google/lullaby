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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_AUDIO_ASSET_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_AUDIO_ASSET_H_

#include <atomic>            
#include <condition_variable>
#include <mutex>             

#include "redux/engines/audio/audio_asset.h"
#include "redux/engines/audio/resonance/audio_planar_data.h"
#include "redux/modules/audio/audio_reader.h"

namespace redux {

// Container that stores information related to an audio asset.
//
// The audio data for the asset will either be streamed from an AudioReader, or
// stored in memory in an AudioPlanarData buffer.
class ResonanceAudioAsset : public AudioAsset {
 public:
  enum class Status {
    // Uninitialized asset.
    kWaitingForReader,
    // Audio data can be streamed using the an AudioReader.
    kReadyForStreaming,
    // Audio data is fully decoded to into a memory buffer.
    kLoadedInMemory,
    // Initialization failed.
    kInvalid,
  };

  // Constructor.
  explicit ResonanceAudioAsset(Id id, bool stream_into_memory = false);

  // Returns the current status of the asset.
  Status GetStatus() const { return status_.load(); }

  // Blocks the calling thread until the asset is ready for use as a stream
  // source, or if an unexpected failure has occurred.
  void WaitForInitialization();

  // Returns true if the asset is initialized and valid for use.
  bool IsValid() const;

  // Binds an audio stream reader to the asset. This function may be called
  // asynchronously, but is needed to initialize the asset.
  void SetAudioReader(std::unique_ptr<AudioReader> reader);

  // Sets the audio data for the asset.
  void SetAudioPlanarData(std::unique_ptr<AudioPlanarData> planar_data);

  // Releases the internal AudioReader to the caller. We cannot have concurrent
  // users attempting to stream data from the same AudioReader, so this
  // effectively locks this ResonanceAudioAsset to the caller. The caller must
  // either call SetAudioReader with this reader, or call SetAudioPlanarData
  // with the decoded data, once they have finished streaming the data.
  std::unique_ptr<AudioReader> AcquireReader();

  // Returns the audio data for the asset. Returns nullptr if the audio data is
  // not stored in memory (i.e. the asset is streaming).
  const AudioPlanarData* GetPlanarData() const { return planar_data_.get(); }

  // Returns true if the asset is being streamed (i.e. someone has called
  // AcquireReader()).
  bool IsActivelyStreaming() const;

  // Should the process that is streaming the asset also attempt to save the
  // deocded data.
  bool ShouldStreamIntoMemory() const { return stream_into_memory_; }

 private:
  // Updates the status of the asset.
  void SetStatus(Status status);

  // Flag indicating whether the audio data should be cached into memory as it
  // is being streamed.
  const bool stream_into_memory_;

  // AudioReader for streaming audio data.
  std::unique_ptr<AudioReader> reader_;

  // Buffer containing uncompressed, planar audio data.
  std::unique_ptr<AudioPlanarData> planar_data_;

  // Mutex and conditional variable to signal status changes.
  std::atomic<Status> status_;
  std::mutex status_change_mutex_;
  std::condition_variable status_change_conditional_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_AUDIO_ASSET_H_
