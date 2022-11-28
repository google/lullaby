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
#include "redux/engines/audio/resonance/audio_planar_data.h"
#include "redux/modules/audio/wav_reader.h"
#include "redux/modules/testing/testing.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::ElementsAreArray;

static constexpr auto kDataPath =
    "redux/modules/audio/test_data";

static std::unique_ptr<WavReader> CreateWavReader(std::string_view uri) {
  const std::string fullpath = ResolveTestFilePath(kDataPath, uri);
  FILE* file = fopen(fullpath.c_str(), "rb");
  return std::make_unique<WavReader>(DataReader::FromCFile(file));
}

TEST(AudioPlanarDataTest, Append) {
  const int num_channels = 2;
  const int num_frames_per_append = 16;

  vraudio::AudioBuffer source(num_channels, num_frames_per_append);
  source.Clear();
  for (int i = 0; i < num_channels; ++i) {
    for (int j = 0; j < num_frames_per_append; ++j) {
      source[i][j] = static_cast<float>((i * num_frames_per_append) + j);
    }
  }

  AudioPlanarData data(num_channels);
  EXPECT_THAT(data.GetNumChannels(), Eq(num_channels));

  data.AppendData(source);
  EXPECT_THAT(data.GetFrameCount(), Eq(num_frames_per_append));

  data.AppendData(source);
  EXPECT_THAT(data.GetFrameCount(), Eq(2 * num_frames_per_append));

  auto channel0 = data.GetChannelData(0);
  auto channel1 = data.GetChannelData(1);
  EXPECT_THAT(channel0.size(), Eq(2 * num_frames_per_append));
  EXPECT_THAT(channel1.size(), Eq(2 * num_frames_per_append));

  std::vector<float> expect;
  expect.insert(expect.end(), source[0].begin(), source[0].end());
  expect.insert(expect.end(), source[0].begin(), source[0].end());
  EXPECT_THAT(channel0, ElementsAreArray(expect.begin(), expect.end()));

  expect.clear();
  expect.insert(expect.end(), source[1].begin(), source[1].end());
  expect.insert(expect.end(), source[1].begin(), source[1].end());
  EXPECT_THAT(channel1, ElementsAreArray(expect.begin(), expect.end()));
}

TEST(AudioPlanarDataTest, FromReader) {
  auto reader = CreateWavReader("speech.wav");

  SpeakerProfile profile;
  profile.frames_per_buffer = 256;
  profile.num_channels = reader->GetNumChannels();
  profile.sample_rate_hz = reader->GetSampleRateHz();

  auto data = AudioPlanarData::FromReader(*reader, profile);
  EXPECT_THAT(data->GetNumChannels(), Eq(reader->GetNumChannels()));
  EXPECT_THAT(data->GetFrameCount(), Eq(reader->GetTotalFrameCount()));
}

TEST(AudioPlanarDataTest, FromReaderResampled) {
  auto reader = CreateWavReader("speech.wav");

  SpeakerProfile profile;
  profile.frames_per_buffer = 256;
  profile.num_channels = reader->GetNumChannels();
  profile.sample_rate_hz = reader->GetSampleRateHz() / 2;

  auto data = AudioPlanarData::FromReader(*reader, profile);
  EXPECT_THAT(data->GetNumChannels(), Eq(reader->GetNumChannels()));
  EXPECT_THAT(data->GetFrameCount(), Eq(reader->GetTotalFrameCount() / 2));
}
}  // namespace
}  // namespace redux
