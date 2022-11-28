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
#include "redux/engines/audio/resonance/audio_asset_stream.h"
#include "redux/modules/audio/wav_reader.h"
#include "redux/modules/testing/testing.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Gt;
using ::testing::Ne;

static constexpr auto kDataPath =
    "redux/modules/audio/test_data";

static std::unique_ptr<WavReader> CreateWavReader(std::string_view uri) {
  const std::string fullpath = ResolveTestFilePath(kDataPath, uri);
  FILE* file = fopen(fullpath.c_str(), "rb");
  return std::make_unique<WavReader>(DataReader::FromCFile(file));
}

static SpeakerProfile InitSpeakerProfile(const AudioReader* reader = nullptr) {
  SpeakerProfile profile;
  profile.frames_per_buffer = 256;
  profile.num_channels = reader ? reader->GetNumChannels() : 2;
  profile.sample_rate_hz = reader ? reader->GetSampleRateHz() : 48000;
  return profile;
}

static uint64_t RoundToFullFrames(uint64_t total_frames,
                                  const SpeakerProfile& profile) {
  const uint64_t frames_per_buffer = profile.frames_per_buffer;
  const float num_buffers =
      static_cast<float>(total_frames) / static_cast<float>(frames_per_buffer);
  const uint64_t rounded = static_cast<uint64_t>(num_buffers + 0.5f);
  return rounded * frames_per_buffer;
}

std::unique_ptr<AudioAssetStream> CreateAudioAssetStreamForStreaming(
    std::unique_ptr<WavReader> reader,
    std::shared_ptr<ResonanceAudioAsset> asset) {
  auto profile = InitSpeakerProfile(reader.get());

  asset->SetAudioReader(std::move(reader));
  auto stream = std::make_unique<AudioAssetStream>(asset, profile);
  EXPECT_TRUE(stream->Initialize());
  return stream;
}

std::unique_ptr<AudioAssetStream> CreateAudioAssetStreamFromMemory(
    std::unique_ptr<WavReader> reader,
    std::shared_ptr<ResonanceAudioAsset> asset) {
  auto profile = InitSpeakerProfile(reader.get());

  asset->SetAudioPlanarData(AudioPlanarData::FromReader(*reader, profile));
  auto stream = std::make_unique<AudioAssetStream>(asset, profile);
  EXPECT_TRUE(stream->Initialize());
  return stream;
}

TEST(AudioAssetStreamTest, BasicInfo) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  auto asset = std::make_shared<ResonanceAudioAsset>(1);
  auto stream = CreateAudioAssetStreamForStreaming(std::move(reader), asset);

  EXPECT_THAT(stream->GetNumChannels(), Eq(profile.num_channels));
  EXPECT_THAT(stream->GetSampleRateHz(), Eq(profile.sample_rate_hz));
}

TEST(AudioAssetStreamTest, ReadFromMemory) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  const uint64_t total_frames = reader->GetTotalFrameCount();
  const uint64_t expected_frames = RoundToFullFrames(total_frames, profile);

  auto asset = std::make_shared<ResonanceAudioAsset>(1);
  auto stream = CreateAudioAssetStreamFromMemory(std::move(reader), asset);
  EXPECT_FALSE(stream->IsPrestockServiceNeeded());

  vraudio::AudioBuffer buffer(profile.num_channels, profile.frames_per_buffer);
  uint64_t frames_read = 0;
  while (true) {
    const vraudio::AudioBuffer* ptr = &buffer;
    if (stream->GetNextAudioBuffer(&ptr)) {
      frames_read += ptr->num_frames();
    } else {
      break;
    }
  }

  EXPECT_THAT(frames_read, Eq(expected_frames));
}

TEST(AudioAssetStreamTest, ReadFromStreaming) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  const uint64_t total_frames = reader->GetTotalFrameCount();
  const uint64_t expected_frames = RoundToFullFrames(total_frames, profile);

  auto asset = std::make_shared<ResonanceAudioAsset>(1);
  auto stream = CreateAudioAssetStreamForStreaming(std::move(reader), asset);

  vraudio::AudioBuffer buffer(profile.num_channels, profile.frames_per_buffer);
  uint64_t frames_read = 0;
  while (true) {
    while (stream->IsPrestockServiceNeeded()) {
      stream->ServicePrestock();
    }

    const vraudio::AudioBuffer* ptr = &buffer;
    if (stream->GetNextAudioBuffer(&ptr)) {
      frames_read += ptr->num_frames();
    } else {
      break;
    }
  }

  EXPECT_THAT(frames_read, Eq(expected_frames));
}

TEST(AudioAssetStreamTest, PrestockBufferUnderrun) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  const uint64_t total_frames = reader->GetTotalFrameCount();
  const uint64_t expected_frames = RoundToFullFrames(total_frames, profile);

  auto asset = std::make_shared<ResonanceAudioAsset>(1);
  auto stream = CreateAudioAssetStreamForStreaming(std::move(reader), asset);
  // Only service the prestock once so that we get an underrun.
  stream->ServicePrestock();

  vraudio::AudioBuffer buffer(profile.num_channels, profile.frames_per_buffer);

  bool underrun = false;
  uint64_t frames_read = 0;
  while (frames_read < expected_frames) {
    const vraudio::AudioBuffer* ptr = &buffer;
    stream->GetNextAudioBuffer(&ptr);
    underrun |= (ptr == nullptr);
    frames_read += buffer.num_frames();
  }

  EXPECT_TRUE(underrun);
}

TEST(AudioAssetStreamTest, ReadFromStreamingIntoMemory) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  const uint64_t total_frames = reader->GetTotalFrameCount();
  const uint64_t expected_frames = RoundToFullFrames(total_frames, profile);

  auto asset = std::make_shared<ResonanceAudioAsset>(1, true);
  auto stream = CreateAudioAssetStreamForStreaming(std::move(reader), asset);

  vraudio::AudioBuffer buffer(profile.num_channels, profile.frames_per_buffer);
  uint64_t frames_read = 0;
  while (true) {
    while (stream->IsPrestockServiceNeeded()) {
      stream->ServicePrestock();
    }

    const vraudio::AudioBuffer* ptr = &buffer;
    if (stream->GetNextAudioBuffer(&ptr)) {
      frames_read += ptr->num_frames();
    } else {
      break;
    }
  }

  EXPECT_THAT(frames_read, Eq(expected_frames));

  auto asset_data = asset->GetPlanarData();
  EXPECT_THAT(asset_data, Eq(nullptr));
  stream.reset();

  asset_data = asset->GetPlanarData();
  EXPECT_THAT(asset_data, Ne(nullptr));
  EXPECT_THAT(asset_data->GetFrameCount(), Eq(expected_frames));
}

TEST(AudioAssetStreamTest, SeekMemory) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  const uint64_t total_frames = reader->GetTotalFrameCount();
  const uint64_t expected_frames = RoundToFullFrames(total_frames, profile);

  auto asset = std::make_shared<ResonanceAudioAsset>(1);
  auto stream = CreateAudioAssetStreamFromMemory(std::move(reader), asset);

  // Skip 1 second.
  stream->Seek(1);
  const uint64_t skipped_frames =
      RoundToFullFrames(profile.sample_rate_hz, profile);

  vraudio::AudioBuffer buffer(profile.num_channels, profile.frames_per_buffer);
  uint64_t frames_read = 0;
  while (true) {
    const vraudio::AudioBuffer* ptr = &buffer;
    if (stream->GetNextAudioBuffer(&ptr)) {
      frames_read += ptr->num_frames();
    } else {
      break;
    }
  }

  EXPECT_THAT(frames_read + skipped_frames, Eq(expected_frames));
}

TEST(AudioAssetStreamTest, SeekFailsForStreamIntoMemory) {
  auto reader = CreateWavReader("speech.wav");
  auto asset = std::make_shared<ResonanceAudioAsset>(1, true);
  auto stream = CreateAudioAssetStreamForStreaming(std::move(reader), asset);
  EXPECT_FALSE(stream->Seek(0));
}

TEST(AudioAssetStreamTest, LoopMemory) {
  auto reader = CreateWavReader("speech.wav");
  auto profile = InitSpeakerProfile(reader.get());

  const uint64_t total_frames = reader->GetTotalFrameCount();
  const uint64_t expected_frames = RoundToFullFrames(total_frames, profile);
  const uint64_t num_reads = expected_frames / profile.frames_per_buffer;

  auto asset = std::make_shared<ResonanceAudioAsset>(1);
  auto stream = CreateAudioAssetStreamFromMemory(std::move(reader), asset);
  stream->EnableLooping(true);

  vraudio::AudioBuffer buffer(profile.num_channels, profile.frames_per_buffer);

  uint64_t frames_read = 0;
  // Read "beyond" the end of the data, which is okay since we're looping.
  for (int i = 0; i < num_reads + 3; ++i) {
    while (stream->IsPrestockServiceNeeded()) {
      stream->ServicePrestock();
    }

    const vraudio::AudioBuffer* ptr = &buffer;
    if (stream->GetNextAudioBuffer(&ptr)) {
      frames_read += buffer.num_frames();
    } else {
      break;
    }
  }

  EXPECT_THAT(frames_read, Gt(expected_frames));
}

}  // namespace
}  // namespace redux
