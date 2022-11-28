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
#include "redux/modules/testing/testing.h"
#include "redux/modules/audio/wav_reader.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Le;

static constexpr auto kDataPath =
    "redux/modules/audio/test_data";

static DataReader CreateDataReader(std::string_view uri) {
  const std::string fullpath = ResolveTestFilePath(kDataPath, uri);
  FILE* file = fopen(fullpath.c_str(), "rb");
  return DataReader::FromCFile(file);
}

static std::unique_ptr<WavReader> CreateWavsReader(std::string_view uri) {
  return std::make_unique<WavReader>(CreateDataReader(uri));
}

TEST(WavReaderTests, BasicInfo) {
  auto reader = CreateWavsReader("speech.wav");

  EXPECT_TRUE(reader->IsValid());
  EXPECT_THAT(reader->GetNumChannels(), Eq(1));
  EXPECT_THAT(reader->GetSampleRateHz(), Eq(8000));
  EXPECT_THAT(reader->GetTotalFrameCount(), Eq(23728L));
  EXPECT_THAT(reader->GetReadFramePosition(), Eq(0));
}

TEST(WavReaderTests, Decode) {
  auto reader = CreateWavsReader("speech.wav");

  uint64_t frame_count = 0;
  const uint64_t num_frames_to_read = 2048;
  while (!reader->IsAtEndOfStream()) {
    auto buffer = reader->ReadFrames(num_frames_to_read);
    const uint64_t num_read = buffer.size() / reader->GetNumBytesPerFrame();
    EXPECT_THAT(num_read, Le(num_frames_to_read));
    frame_count += num_read;
  }

  EXPECT_TRUE(reader->IsAtEndOfStream());
  EXPECT_THAT(frame_count, Eq(reader->GetTotalFrameCount()));
}

TEST(WavReaderTests, DecodeAmbisonic) {
  auto reader = CreateWavsReader("ambisonic.wav");

  uint64_t frame_count = 0;
  const uint64_t num_frames_to_read = 2048;
  while (!reader->IsAtEndOfStream()) {
    auto buffer = reader->ReadFrames(num_frames_to_read);
    const uint64_t num_read = buffer.size() / reader->GetNumBytesPerFrame();
    EXPECT_THAT(num_read, Le(num_frames_to_read));
    frame_count += num_read;
  }

  EXPECT_TRUE(reader->IsAtEndOfStream());
  EXPECT_THAT(frame_count, Eq(reader->GetTotalFrameCount()));
}

TEST(WavReaderTests, CheckHeader) {
  auto reader1 = CreateDataReader("speech.wav");
  EXPECT_TRUE(WavReader::CheckHeader(reader1));

  auto reader2 = CreateDataReader("speech.ogg");
  EXPECT_FALSE(WavReader::CheckHeader(reader2));
}

}  // namespace
}  // namespace redux
