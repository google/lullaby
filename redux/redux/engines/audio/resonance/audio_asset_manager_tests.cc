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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/testing/testing.h"
#include "redux/engines/audio/resonance/audio_asset_manager.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Ne;

static constexpr auto kDataPath =
    "redux/modules/audio/test_data";

class AudioAssetManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    SpeakerProfile profile;
    profile.num_channels = 2;
    profile.frames_per_buffer = 256;
    profile.sample_rate_hz = 48000;

    asset_loader_ = registry_.Create<AssetLoader>(&registry_);
    asset_loader_->StopAsyncOperations();

    audio_asset_manager_ =
        registry_.Create<AudioAssetManager>(&registry_, profile);
  }

  Registry registry_;
  AssetLoader* asset_loader_;
  AudioAssetManager* audio_asset_manager_;
};


TEST_F(AudioAssetManagerTest, CreateAudioAssetForStreaming) {
  const std::string uri = ResolveTestFilePath(kDataPath, "speech.wav");
  const auto asset =
      audio_asset_manager_->CreateAudioAsset(uri, AudioEngine::kStreamAndClose);
  EXPECT_TRUE(std::static_pointer_cast<ResonanceAudioAsset>(asset)->IsValid());
}

TEST_F(AudioAssetManagerTest, CreateAudioAssetPreload) {
  const std::string uri = ResolveTestFilePath(kDataPath, "speech.wav");
  const auto asset = audio_asset_manager_->CreateAudioAsset(
      uri, AudioEngine::kPreloadIntoMemory);
  EXPECT_TRUE(std::static_pointer_cast<ResonanceAudioAsset>(asset)->IsValid());
}

TEST_F(AudioAssetManagerTest, CreateAudioAssetForStreamingAndLoading) {
  const std::string uri = ResolveTestFilePath(kDataPath, "speech.wav");
  const auto asset = audio_asset_manager_->CreateAudioAsset(
      uri, AudioEngine::kStreamIntoMemory);
  EXPECT_TRUE(std::static_pointer_cast<ResonanceAudioAsset>(asset)->IsValid());
}

TEST_F(AudioAssetManagerTest, InvalidAsset) {
  const std::string uri = ResolveTestFilePath(kDataPath, "bad.wav");
  const auto asset = audio_asset_manager_->CreateAudioAsset(
      uri, AudioEngine::kStreamIntoMemory);
  EXPECT_FALSE(std::static_pointer_cast<ResonanceAudioAsset>(asset)->IsValid());
}

TEST_F(AudioAssetManagerTest, UnloadAudioAsset) {
  const std::string uri = ResolveTestFilePath(kDataPath, "speech.wav");
  auto asset =
      audio_asset_manager_->CreateAudioAsset(uri, AudioEngine::kStreamAndClose);
  EXPECT_THAT(asset, Ne(nullptr));

  audio_asset_manager_->UnloadAudioAsset(Hash(uri));
  asset = audio_asset_manager_->FindAudioAsset(Hash(uri));
  EXPECT_THAT(asset, Eq(nullptr));
}

TEST_F(AudioAssetManagerTest, GetAssetForPlayback) {
  const std::string uri = ResolveTestFilePath(kDataPath, "speech.wav");
  const auto asset =
      audio_asset_manager_->CreateAudioAsset(uri, AudioEngine::kStreamAndClose);
  const auto playback_asset =
      audio_asset_manager_->GetAssetForPlayback(asset->GetId());
  EXPECT_THAT(playback_asset, Ne(nullptr));
}

TEST_F(AudioAssetManagerTest, GetAssetForPlaybackLocked) {
  const std::string uri = ResolveTestFilePath(kDataPath, "speech.wav");
  const auto asset = audio_asset_manager_->CreateAudioAsset(
      uri, AudioEngine::kStreamIntoMemory);
  const auto asset1 = audio_asset_manager_->GetAssetForPlayback(asset->GetId());
  asset1->AcquireReader();
  const auto asset2 = audio_asset_manager_->GetAssetForPlayback(asset->GetId());
  EXPECT_THAT(asset1->GetId(), Ne(asset2->GetId()));
}

}  // namespace
}  // namespace redux
