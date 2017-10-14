/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_SYSTEMS_AUDIO_SOUND_ASSET_MANAGER_H_
#define LULLABY_SYSTEMS_AUDIO_SOUND_ASSET_MANAGER_H_

#include <unordered_set>

#include "lullaby/generated/audio_playback_types_generated.h"
#include "lullaby/systems/audio/sound_asset.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"
#include "vr/gvr/capi/include/gvr_audio.h"

namespace lull {

// Loads and manages sound assets for the AudioSystem using the gvr::AudioApi
// to perform loading and decoding of sound files. Dispatches AudioLoadedEvents
// when sounds have completed loading.
class SoundAssetManager {
 public:
  // The gvr::AudioApi passed to this SoundAssetManager has it's lifecycle
  // managed by the AudioSystem.
  explicit SoundAssetManager(Registry* registry,
                             std::weak_ptr<gvr::AudioApi> audio);

  // Process completed audio loading tasks.
  void ProcessTasks();

  // Get the SoundAsset for the given |sound_hash|. Note that the asset may
  // be deleted at any time by another caller. The caller should only store a
  // SoundAssetWeakPtr, else assets may not be properly cleaned up.
  SoundAssetPtr GetSoundAsset(HashValue sound_hash);

  // Create and load a new sound asset from a file. |type| denotes how this
  // sound should be retrieved and played. |entity| denotes an entity to
  // send an AudioLoadedEvent to when the sound is finished loading - this
  // event will be sent immediately if the sound is streamed.
  void CreateSoundAsset(const std::string& filename, AudioPlaybackType type,
                        Entity entity = kNullEntity);

  // Release and unload an existing sound asset for |sound_hash|. If the asset
  // is currently in use, the memory will be freed as soon as playback stops. If
  // the asset is currently loading, it will be unloaded when loading completes.
  void ReleaseSoundAsset(HashValue sound_hash);

 private:
  // Send an AudioLoadedEvent to |entity|.
  void SendAudioLoadedEvent(Entity entity);

  Registry* registry_;
  std::weak_ptr<gvr::AudioApi> audio_handle_;
  AsyncProcessor<SoundAssetWeakPtr> processor_;
  ResourceManager<SoundAsset> assets_;
  std::unordered_set<HashValue> assets_to_unload_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_AUDIO_SOUND_ASSET_MANAGER_H_
