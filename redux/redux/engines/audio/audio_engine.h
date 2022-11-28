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

#ifndef REDUX_ENGINES_AUDIO_AUDIO_ENGINE_H_
#define REDUX_ENGINES_AUDIO_AUDIO_ENGINE_H_

#include "redux/engines/audio/audio_asset.h"
#include "redux/engines/audio/sound.h"
#include "redux/engines/audio/sound_room.h"
#include "redux/modules/audio/enums.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Responsible for managing and playing sounds.
class AudioEngine {
 public:
  static void Create(Registry* registry);
  virtual ~AudioEngine() = default;

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  // Sets the global volume of the AudioEngine itself, ranging from from 0
  // (mute) to 1 (max).
  void SetGlobalVolume(float volume);

  // Sets the position and rotation of the listener's "head".
  void SetListenerTransform(const vec3& position, const quat& rotation);

  enum StreamingPolicy {
    // Only opens the audio asset for streaming. Once playback is finished, the
    // handle to the asset is closed.
    kStreamAndClose,
    // Loads the entire audio data into memory when it is opened. The sound can
    // then be played as often as desired, until it is explicitly closed.
    kPreloadIntoMemory,
    // Opens the asset for streaming, but as it is played, will also store the
    // audio data into memory.
    kStreamIntoMemory,
  };

  // Loads an AudioAsset from the given `uri`. Future requests for this
  // AudioAsset will be cached as long as one instance is alive somewhere, or
  // users can request the asset by calling GetAudioAsset with the Hash of the
  // `uri`.
  AudioAssetPtr LoadAudioAsset(std::string_view uri, StreamingPolicy policy);

  // Returns the AudioAsset associated with the given `key` that has previously
  // been loaded and is still alive.
  AudioAssetPtr GetAudioAsset(HashValue key);

  // Unloads the AudioAsset associated with the given `key`.
  void UnloadAudioAsset(HashValue key);

  // Information about how to play a sound.
  struct SoundPlaybackParams {
    SoundType type = SoundType::Stereo;
    float volume = 1.0f;
    bool looping = false;
  };

  // Starts playing a sound using the given `asset` and play `params`.
  SoundPtr PlaySound(AudioAssetPtr asset, const SoundPlaybackParams& params);

  // Similar to PlaySound, but the Sound starts in a paused state.
  SoundPtr PrepareSound(AudioAssetPtr asset, const SoundPlaybackParams& params);

  // Updates all the active sounds.
  void Update();

  // Updates Audio rendering to simulate the listener being in an enclosed
  // space (i.e. a room).
  void EnableRoom(const SoundRoom& room, const vec3& position,
                  const quat& rotation);
  void DisableRoom();

 protected:
  explicit AudioEngine(Registry* registry) : registry_(registry) {}

  Registry* registry_ = nullptr;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::AudioEngine);

#endif  // REDUX_ENGINES_AUDIO_AUDIO_ENGINE_H_
