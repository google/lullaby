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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_STREAM_RENDERER_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_STREAM_RENDERER_H_

#include <atomic>
#include <memory>

#include "redux/engines/audio/resonance/audio_source_stream.h"
#include "redux/modules/audio/enums.h"
#include "resonance_audio/api/resonance_audio_api.h"

namespace redux {

// Reads audio data from a AudioSourceStream and pushes it into Resonance.
class AudioStreamRenderer {
 public:
  // Creates a Sound object in Resonance that will render audio data from the
  // given stream. The sound will start in a paused state and must be explicitly
  // unpaused.
  AudioStreamRenderer(SoundType type, std::unique_ptr<AudioSourceStream> stream,
                      vraudio::ResonanceAudioApi* resonance,
                      vraudio::RenderingMode rendering_mode);

  ~AudioStreamRenderer();

  // Starts rendering of source stream.
  void Resume();

  // Stops rendering of source stream.
  void Pause();

  // Shuts down source stream. Note that the audio stream is first faded-out
  // before removing it from Resonance.
  void Shutdown();

  // Sets volume of source stream.
  void SetVolume(float volume);

  // Returns the handle to the underlying vraudio source.
  vraudio::SourceId GetSourceId() const { return source_id_; }

  // Writes a new chunk of audio data to the vraudio source. This method needs
  // to be called before a new output audio buffer is requested from Resonance.
  bool Render();

  // Flags the renderer as having been added to the asynchronous block streaming
  // queue by the AudioStreamManager.
  void SetPrestockServicePending(bool pending);

  // Checks if the stream source requires block pre-processing to continue the
  // flow of audio data. Used by the AudioStreamManager to check if the renderer
  // should be added to the asynchronous block streaming thread.
  bool IsPrestockServiceNeeded() const;

  // Runs any processing needed by the source stream to continue data flow.
  // This function should only be called by the AudioStreamManager. Note that
  // this function may be expensive, so invoking it from the audio thread
  // should be avoided.
  void ServicePrestock();

 private:
  // The stream source from which to render audio data.
  std::unique_ptr<AudioSourceStream> stream_;

  // Pointer to the Resonance Audio API.
  vraudio::ResonanceAudioApi* resonance_;

  // Handle to the sound source in Resonance.
  vraudio::SourceId source_id_;

  // Indicates if the AudioSourceStream is paused. When audio streams are
  // stopped they are first faded-out before this flag is set to true.
  bool is_paused_;

  // Indicates a Shutdown call.
  bool shutdown_triggered_;

  // Volume of audio stream.
  float stream_volume_;

  // Number of silence buffers to be triggered before pausing the stream.
  size_t fade_out_count_down_;

  // Indicates whether this renderer has already been staged for prestock.
  // This flag is used only by the AudioStreamManager to co-ordinate the running
  // of th prestock service.
  std::atomic<bool> prestock_service_pending_;

  // Buffer to hold pointers to individual output channels. We need this buffer
  // to bridge the API between vraudio::AudioBuffer and
  // ResonanceAudioApi::SetPlanarBuffer.
  std::vector<const float*> output_channels_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_STREAM_RENDERER_H_
