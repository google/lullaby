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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_AUDIO_ENGINE_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_AUDIO_ENGINE_H_

#include <atomic>
#include <memory>
#include <mutex> 
#include <thread>

#include "redux/engines/audio/audio_engine.h"
#include "redux/engines/audio/resonance/audio_asset_manager.h"
#include "redux/engines/audio/resonance/audio_stream_manager.h"
#include "redux/engines/audio/resonance/resonance_sound.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/modules/base/registry.h"
#include "resonance_audio/api/resonance_audio_api.h"
#include "resonance_audio/base/constants_and_types.h"
#include "resonance_audio/utils/lockless_task_queue.h"
#include "resonance_audio/utils/semi_lockless_fifo.h"

namespace redux {

// Audio engine that spatializes sound sources in 3D space, including distance
// and height cues, using the Resonance Audio library.
//
// The engine supports three types of sounds:
//
// 1. Sound Object: a sound source in 3D space. These sources, while
// spatialized, are fed with mono audio data.
//
// 2. Ambisonic Soundfield: multi-channel audio files which are spatialized all
// around the listener in 360 degrees. These can be thought of as recorded or
// prebaked soundfields. They can be of great use for background effects which
// sound perfectly spatial. Examples include rain noise, crowd noise or even the
// sound of the ocean off to one side.
//
// 3. Stereo Sounds: non-spatialized mono or stereo audio files played directly.
// This is useful for music and other such audio.
//
// Audio assets can be loaded using the AudioAssetManager. Once an asset is
// loaded, it can be used to create individual Sound instances that can be
// played.
//
// The ResonanceAudioEngine owns the "audio" thread on which audio rendering
// is performed using the AudioStreamManager.
class ResonanceAudioEngine : public AudioEngine {
 public:
  using SourceId = vraudio::SourceId;

  explicit ResonanceAudioEngine(Registry* registry);
  ~ResonanceAudioEngine();

  // Must be called from the main thread at a regular rate. It is used to
  // execute background operations outside of the audio thread.
  void Update();

  // Creates and returns a new Sound object using the asset as a source.
  //
  // For Point sounds, the asset should contain a single (mono) audio channel.
  // For ambisonic Field sounds, the asset must have 4 separate audio channels.
  SoundPtr CreateSound(AudioAssetPtr asset, const SoundPlaybackParams& params);

  // Pauses the playback of a sound.
  void PauseSound(SourceId source_id);

  // Resumes the playback of a sound.
  void ResumeSound(SourceId source_id);

  // Stops the playback of a sound and invalidates the source.
  void StopSound(SourceId source_id);

  // Repositions an existing sound object.
  void SetSoundObjectPosition(SourceId source_id, const vec3& position);

  // Sets the given sound object source's distance attenuation method with
  // minimum and maximum distances. Maximum distance must be greater than the
  // minimum distance for the method to be set.
  void SetSoundObjectDistanceRolloffModel(SourceId source_id,
                                          Sound::DistanceRolloffModel rolloff,
                                          float min_distance,
                                          float max_distance);

  // Sets the given ambisonic soundfields's rotation.
  void SetSoundfieldRotation(SourceId source, const quat& rotation);

  // Sets the given sound object's directivity pattern.
  //
  // `alpha` specifies the weighting balance between figure of eight pattern and
  // omnidirectional pattern for source emission in range [0, 1]. A value of 0.5
  // results in a cardioid pattern.
  //
  // `order`is the order applied to computed directivity. Higher values will
  // result in narrower and sharper directivity patterns. Range [1, inf).
  void SetSoundObjectDirectivity(SourceId source_id, float alpha, float order);

  // Sets the given sound object's rotation. Note: This method only has an
  // audible effect if the directivity pattern of the sound object is first set.
  void SetSoundObjectRotation(SourceId source_id, const quat& rotation);

  // Changes the volume of an existing sound.
  void SetSoundVolume(SourceId source_id, float volume);

  // Sets the global volume of the main audio output. Specify a volume (linear)
  // amplitude in range [0, 1] for attenuation, range [1, inf) for gain boost.
  void SetGlobalVolume(float volume);

  // Sets the position and rotation of the "listener" from which spatialized
  // sounds will be related.
  void SetListenerTransform(const vec3& position, const quat& rotation);

  // Turns on/off the room reverberation effect.
  void EnableRoom(const SoundRoom& room, const vec3& position,
                  const quat& rotation);
  void DisableRoom();

  // Returrns the AudioAssetManager used for managing audio assets.
  AudioAssetManager* GetAudioAssetManager() {
    return audio_asset_manager_.get();
  }

  // Gets the underlying ResonanceAudioApi object for more functionality.
  vraudio::ResonanceAudioApi* GetResonanceAudioApi() {
    return resonance_.get();
  }

 private:
  // Starts the audio playback.
  void Start();

  // Stops the audio playback.
  void Stop();

  // Callback from audio device requesting more audio data.
  void OnMoreData(int16_t* buffer_ptr, size_t num_channels, size_t num_frames);

  void ExportToStereoBuffer(const vraudio::AudioBuffer& input, int16_t* output,
                            size_t num_channels, size_t num_frames);
  void DownmixToMonoBuffer(const vraudio::AudioBuffer& input, int16_t* output,
                           size_t num_channels, size_t num_frames);
  void UpmixToSurroundBuffer(const vraudio::AudioBuffer& input, int16_t* output,
                             size_t num_channels, size_t num_frames);

  // Performs an operation on the AudioStreamRenderer on the audio thread.
  using RendererTask = std::function<void(AudioStreamRenderer&)>;
  void RunRendererTask(SourceId source_id, RendererTask fn);

  // Adds new audio buffers for each source to |ResonanceAudioApi| and triggers
  // the audio graph processing to obtain a binaurally rendered stereo
  // |AudioBuffer|.
  const vraudio::AudioBuffer* BinauralProcessingOfSoundSources();

  // Performs audio processing in a separate thread and outputs to |fifo_|.
  void AudioProcessingThread();

  std::unique_ptr<vraudio::ResonanceAudioApi> resonance_;
  std::unique_ptr<AudioAssetManager> audio_asset_manager_;
  std::unique_ptr<AudioStreamManager> audio_stream_manager_;

  std::optional<SpeakerProfile> speaker_profile_;

  absl::flat_hash_map<SourceId, std::shared_ptr<ResonanceSound>> sounds_;

  // Audio processing thread.
  std::thread audio_thread_;
  std::atomic<bool> audio_running_;

  // Sources that have been completed on the audio thread are marked for
  // deletion on the main thread.
  mutable std::mutex pending_delete_mutex_;
  std::vector<SourceId> pending_delete_;

  // Task queue of operations to be executed on the audio thread.
  mutable vraudio::LocklessTaskQueue audio_thread_task_queue_;

  // FIFO queue to transport processed buffers between the processing thread and
  // the audio thread.
  vraudio::SemiLocklessFifo<std::unique_ptr<vraudio::AudioBuffer>> fifo_;

  // Temporary AudioBuffer to for downmixing audio to mono output.
  std::unique_ptr<vraudio::AudioBuffer> mono_buffer_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_AUDIO_ENGINE_H_
