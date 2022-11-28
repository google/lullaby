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

#include "redux/engines/audio/resonance/audio_asset_manager.h"

#include <memory>
#include <utility>

#include "redux/engines/audio/resonance/resonance_audio_asset.h"
#include "redux/modules/audio/audio_reader.h"
#include "redux/modules/audio/opus_reader.h"
#include "redux/modules/audio/vorbis_reader.h"
#include "redux/modules/audio/wav_reader.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/logging.h"
#include "resonance_audio/dsp/resampler.h"

namespace redux {

AudioAssetManager::AudioAssetManager(Registry* registry, SpeakerProfile profile)
    : registry_(registry), speaker_profile_(profile) {
}

AudioAssetPtr AudioAssetManager::CreateAudioAsset(
    std::string_view uri, AudioEngine::StreamingPolicy policy) {
  const HashValue key = Hash(uri);
  AudioAssetPtr asset = audio_assets_.Find(key);
  if (asset == nullptr) {
    AudioAsset::Id id = LoadAudioAsset(uri, policy);
    asset = GetAssetById(id);
    audio_assets_.Register(key, asset);
  }
  return asset;
}

AudioAsset::Id AudioAssetManager::LoadAudioAsset(
    std::string_view uri, AudioEngine::StreamingPolicy policy) {
  auto asset_loader = registry_->Get<AssetLoader>();

  const AudioAsset::Id id = asset_id_counter_++;
  auto asset = std::make_shared<ResonanceAudioAsset>(
      id, policy == AudioEngine::kStreamIntoMemory);
  asset_map_.insert(std::make_pair(id, asset));
  asset_uris_.insert(std::make_pair(id, std::string(uri)));

  auto on_open = [=](AssetLoader::StatusOrReader& reader) {
    if (reader.ok()) {
      std::unique_ptr<AudioReader> audio_reader = CreateReader(reader.value());
      if (policy == AudioEngine::kPreloadIntoMemory) {
        auto data =
            AudioPlanarData::FromReader(*audio_reader, speaker_profile_);
        asset->SetAudioPlanarData(std::move(data));
      } else {
        asset->SetAudioReader(std::move(audio_reader));
      }
    } else {
      LOG(ERROR) << reader.status().message();
      asset->SetAudioReader(nullptr);
    }
  };

  asset_loader->OpenAsync(uri, on_open, nullptr);
  return id;
}

AudioAssetPtr AudioAssetManager::FindAudioAsset(HashValue key) {
  return audio_assets_.Find(key);
}

std::shared_ptr<ResonanceAudioAsset> AudioAssetManager::GetAssetById(
    AudioAsset::Id asset_id) {
  const auto iter = asset_map_.find(asset_id);
  if (iter == asset_map_.end()) {
    return nullptr;
  }

  std::shared_ptr<ResonanceAudioAsset> asset = iter->second;
  DCHECK(asset);
  return asset;
}

void AudioAssetManager::UnloadAudioAsset(HashValue key) {
  auto asset = FindAudioAsset(key);
  asset_map_.erase(asset->GetId());
  asset_uris_.erase(asset->GetId());
  audio_assets_.Erase(key);
}

std::shared_ptr<ResonanceAudioAsset> AudioAssetManager::GetAssetForPlayback(
    AudioAsset::Id asset_id) {
  std::shared_ptr<ResonanceAudioAsset> asset = GetAssetById(asset_id);
  if (asset == nullptr) {
    LOG(ERROR) << "Attempt to stream invalid asset";
    return nullptr;
  }

  asset->WaitForInitialization();

  if (asset->IsActivelyStreaming()) {
    // Another AudioAssetStream is streaming this asset, so we need to create
    // a temporary, stream-only asset from the same URI. If this is an issue
    // consider loading the asset into memory so its always available.
    asset = CreateTemporaryAudioAsset(asset_uris_[asset_id]);
  }

  if (!asset->IsValid()) {
    LOG(ERROR) << "Attempt to stream uninitialized asset.";
    return nullptr;
  }

  // This is a streaming-only asset (i.e. it's StreamingPolicy was
  // kStreamAndClose), so let's forget about it here.
  if (asset->GetPlanarData() == nullptr &&
      asset->ShouldStreamIntoMemory() == false) {
    // UnloadAudioAsset doesn't actually destroy the AudioAsset; it just stops
    // owning the shared_ptr.
    UnloadAudioAsset(Hash(asset_uris_[asset_id]));
  }

  return asset;
}

std::shared_ptr<ResonanceAudioAsset>
AudioAssetManager::CreateTemporaryAudioAsset(std::string_view uri) {
  auto asset_loader = registry_->Get<AssetLoader>();
  auto reader = asset_loader->OpenNow(uri);
  if (!reader.ok()) {
    return nullptr;
  }

  const AudioAsset::Id new_asset_id = asset_id_counter_++;
  auto asset = std::make_shared<ResonanceAudioAsset>(new_asset_id);
  asset->SetAudioReader(CreateReader(reader.value()));
  CHECK(asset->IsValid()) << "Failed to acquire reader for AudioAssetStream.";
  return asset;
}

std::unique_ptr<AudioReader> AudioAssetManager::CreateReader(
    DataReader& src) {
  std::unique_ptr<AudioReader> reader;
  if (WavReader::CheckHeader(src)) {
    reader = std::make_unique<WavReader>(std::move(src));
  } else if (OpusReader::CheckHeader(src)) {
    reader = std::make_unique<OpusReader>(std::move(src));
  } else if (VorbisReader::CheckHeader(src)) {
    reader = std::make_unique<VorbisReader>(std::move(src));
  }
  if (reader == nullptr) {
    LOG(ERROR) << "Unable to determine audio format.";
    return nullptr;
  }

  const int source_sample_rate_hz = reader->GetSampleRateHz();
  const int target_sample_rate_hz = speaker_profile_.sample_rate_hz;

  if (source_sample_rate_hz!= target_sample_rate_hz) {
    const bool supported = vraudio::Resampler::AreSampleRatesSupported(
          source_sample_rate_hz, target_sample_rate_hz);
    if (!supported) {
      LOG(ERROR) << "Unsupported sampling rate: " << source_sample_rate_hz
                 << ". System sample rate: " << target_sample_rate_hz;
      return nullptr;
    }
  }
  return reader;
}

}  // namespace redux
