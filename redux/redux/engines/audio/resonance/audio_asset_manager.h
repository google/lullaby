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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_ASSET_MANAGER_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_ASSET_MANAGER_H_

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "redux/engines/audio/audio_asset.h"
#include "redux/engines/audio/audio_engine.h"
#include "redux/engines/audio/resonance/resonance_audio_asset.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/modules/audio/audio_reader.h"
#include "redux/modules/base/data_reader.h"
#include "redux/modules/base/registry.h"

namespace redux {

// Provides methods for preloading and managing samples/sounds in memory and
// creating asset handles.
class AudioAssetManager {
 public:
  AudioAssetManager(Registry* registry, SpeakerProfile profile);

  // Initializes an audio asset by scheduling the decoding/initialization task
  // for asynchronous loading, then returns immediately.
  //
  // The hash of the URI can be used as a key for looking up the asset.
  AudioAssetPtr CreateAudioAsset(std::string_view uri,
                                 AudioEngine::StreamingPolicy policy);

  // Removes a previously initialized audio asset from the asset cache. The key
  // is the hash of the URI of the loaded asset.
  void UnloadAudioAsset(HashValue key);

  // Returns a loaded/cached audio asset, using the hash of the URI as the key.
  AudioAssetPtr FindAudioAsset(HashValue key);

  // Returns an 'AudioAsset' that can be used for playback by the engine. Note:
  // this is not necessarily the same instance as that returned by LookupAsset.
  // Specifically, if an asset is already in use for streaming, this will return
  // a new instance (since a single asset cannot support multiple streaming
  // playbacks).
  std::shared_ptr<ResonanceAudioAsset> GetAssetForPlayback(
      AudioAsset::Id asset_id);

 private:
  AudioAsset::Id LoadAudioAsset(std::string_view uri,
                                  AudioEngine::StreamingPolicy policy);

  std::shared_ptr<ResonanceAudioAsset> GetAssetById(AudioAsset::Id asset_id);

  std::shared_ptr<ResonanceAudioAsset> CreateTemporaryAudioAsset(
      std::string_view uri);

  std::unique_ptr<AudioReader> CreateReader(DataReader& src);

  Registry* registry_ = nullptr;

  SpeakerProfile speaker_profile_;
  AudioAsset::Id asset_id_counter_ = 0;

  // Maps |AudioAsset::Id| to |AudioAsset| instances. Shared pointers are used
  // to safely remove assets that are currently played back or actively decoded.
  ResourceManager<AudioAsset> audio_assets_;
  absl::flat_hash_map<AudioAsset::Id, std::string> asset_uris_;
  absl::flat_hash_map<AudioAsset::Id, std::shared_ptr<ResonanceAudioAsset>>
      asset_map_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::AudioAssetManager);

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_AUDIO_ASSET_MANAGER_H_
