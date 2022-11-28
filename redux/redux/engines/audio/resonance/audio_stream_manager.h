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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_STREAM_MANAGER_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_STREAM_MANAGER_H_

#include <functional>
#include <memory>
#include <thread>

#include "absl/container/flat_hash_map.h"
#include "redux/engines/audio/resonance/audio_stream_renderer.h"
#include "resonance_audio/utils/threadsafe_fifo.h"

namespace redux {

// Manages the "streaming" thread in which an AudioStreamRenderer can stream and
// decode audio data.
class AudioStreamManager {
 public:
  using SourceId = vraudio::SourceId;
  using RendererPtr = std::shared_ptr<AudioStreamRenderer>;

  AudioStreamManager();

  // Starts the asynchronous streaming thread for decoding audio.
  void Start();

  // Stops the asynchronous streaming thread. Note: buffered tasks which have
  // not begun will be discarded.
  void Stop();

  // Registers a new AudioStreamRenderer. The renderer will be automatically
  // removed once it has completed playback. Returns `false` if the renderer's
  // SourceId is already in use.
  bool AddAudioStreamRenderer(RendererPtr renderer);

  // Returns a non-owning pointer to a registered AudioStreamRenderer.
  AudioStreamRenderer* GetAudioStreamRenderer(SourceId source_id) const;

  // Triggers all Renderers to render new audio buffers with resonance.
  //
  // Any Renderers that have completed playback will be unregistered. The
  // SourceIds of these renderers will added to `disabled_renderer_ids`
  // argument, if one is provided.
  void Render(std::vector<SourceId>* disabled_renderer_ids);

 private:
  void StreamingThread();

  bool PushToStreamRendererFifo(const RendererPtr& renderer);

  RendererPtr PopFromStreamRendererFifo();

  std::thread streaming_thread_;
  std::atomic<bool> streaming_thread_running_;
  vraudio::ThreadsafeFifo<RendererPtr> stream_renderer_ptr_fifo_;
  absl::flat_hash_map<SourceId, RendererPtr> renderers_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_STREAM_MANAGER_H_
