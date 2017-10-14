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

#include "lullaby/systems/audio/sound_asset_manager.h"

#include "lullaby/events/audio_events.h"
#include "lullaby/modules/file/file.h"
#include "lullaby/modules/file/tagged_file_loader.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/util/make_unique.h"

namespace lull {

SoundAssetManager::SoundAssetManager(Registry* registry,
                                     std::weak_ptr<gvr::AudioApi> audio)
    : registry_(registry),
      audio_handle_(std::move(audio)) {
}

void SoundAssetManager::ProcessTasks() {
  SoundAssetWeakPtr asset_handle;
  while (processor_.Dequeue(&asset_handle)) {
    auto asset = asset_handle.lock();
    if (!asset) {
      continue;
    }
    const std::string& filename = asset->GetFilename();
    const HashValue sound_hash = Hash(filename);

    // If destruction was requested while preload was pending, destroy the
    // asset and don't send out any events.
    if (assets_to_unload_.find(sound_hash) != assets_to_unload_.end()) {
      assets_to_unload_.erase(sound_hash);
      ReleaseSoundAsset(sound_hash);
      continue;
    }

    const auto load_status = asset->GetLoadStatus();
    if (load_status == SoundAsset::LoadStatus::kLoaded) {
      for (Entity entity : asset->GetListeningEntities()) {
        SendAudioLoadedEvent(entity);
      }
    } else if (load_status == SoundAsset::LoadStatus::kFailed) {
      LOG(ERROR) << "Failed to load audio asset " << filename;
    }
  }
}

SoundAssetPtr SoundAssetManager::GetSoundAsset(HashValue sound_hash) {
  return assets_.Find(sound_hash);
}

void SoundAssetManager::CreateSoundAsset(const std::string& filename,
                                         AudioPlaybackType type,
                                         Entity entity) {
  if (type == AudioPlaybackType::AudioPlaybackType_External) {
    LOG(DFATAL) << "AudioPlaybackType::External is reserved exclusively for "
                   "Track(), and cannot be used for normal assets.";
    return;
  }

  const HashValue sound_hash = Hash(filename.c_str());

  auto existing_asset = assets_.Find(sound_hash);
  if (existing_asset) {
    // Another entity has already queued the loading of this file.
    if (existing_asset->GetLoadStatus() == SoundAsset::LoadStatus::kLoaded ||
        type == AudioPlaybackType::AudioPlaybackType_Stream) {
      SendAudioLoadedEvent(entity);
    } else {
      existing_asset->AddLoadedListener(entity);
    }
    return;
  }

  // Correct the file since GVR Audio's asset manager doesn't handle tags.
  std::string corrected_filename;
  TaggedFileLoader::ApplySettingsToTaggedFilename(
      filename.c_str(), &corrected_filename);

  // Create an empty asset that will be properly finalized asynchronously.
  auto asset = assets_.Create(sound_hash, [&]() {
    return std::make_shared<SoundAsset>(entity, corrected_filename);
  });

  // The following platforms use specific audio loaders to support more file
  // formats. Other platforms are not guaranteed to load any formats other than
  // ogg.
#if !defined(TARGET_OS_IPHONE) && !defined(__ANDROID__)
  if (!EndsWith(corrected_filename, ".ogg")) {
    LOG(WARNING) << "This platform only supports OGG file formats: "
                 << corrected_filename;
    return;
  }
#endif

  if (type == AudioPlaybackType::AudioPlaybackType_Stream) {
    // Streamed assets are handled differently than preloaded ones. GVR Audio
    // will automatically create an audio streamer and begin playback.
    asset->SetLoadStatus(SoundAsset::LoadStatus::kStreaming);
    SendAudioLoadedEvent(entity);
  } else {
    // Asynchronously request the loading of audio through GVR Audio, then
    // finalize the asset in ProcessTasks() when loading is complete.
    processor_.Enqueue(asset, [this](SoundAssetWeakPtr* asset_handle) {
      auto audio = audio_handle_.lock();
      auto asset = asset_handle->lock();
      if (audio && asset) {
        if (!audio->PreloadSoundfile(asset->GetFilename())) {
          asset->SetLoadStatus(SoundAsset::LoadStatus::kFailed);
        } else {
          asset->SetLoadStatus(SoundAsset::LoadStatus::kLoaded);
        }
      }
    });
  }
}

void SoundAssetManager::ReleaseSoundAsset(HashValue sound_hash) {
  auto asset = assets_.Find(sound_hash);
  if (!asset) {
    return;
  }

  switch (asset->GetLoadStatus()) {
    case SoundAsset::LoadStatus::kUnloaded:
      // Unloaded sounds will be destroyed when loading has finished for thread
      // safety reasons.
      assets_to_unload_.insert(sound_hash);
      break;
    case SoundAsset::LoadStatus::kLoaded:
    case SoundAsset::LoadStatus::kStreaming:
    case SoundAsset::LoadStatus::kFailed: {
      // Loaded sounds, regardless of how, can be unloaded and released.
      auto audio = audio_handle_.lock();
      if (audio) {
        audio->UnloadSoundfile(asset->GetFilename());
      }
      assets_.Release(sound_hash);
      break;
    }
  }
}

void SoundAssetManager::SendAudioLoadedEvent(Entity entity) {
  DispatcherSystem* dispatcher_system = registry_->Get<DispatcherSystem>();
  if (dispatcher_system) {
    dispatcher_system->Send(entity, AudioLoadedEvent());
  }
}

}  // namespace lull
