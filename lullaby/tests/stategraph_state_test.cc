/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/modules/stategraph/stategraph_state.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/time.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::IsNull;
using ::testing::NotNull;

Optional<HashValue> GetKey(const VariantMap& map) {
  Optional<HashValue> key;
  auto iter = map.find(ConstHash("key"));
  if (iter != map.end()) {
    const HashValue* ptr = iter->second.Get<HashValue>();
    if (ptr != nullptr) {
      key = *ptr;
    }
  }
  return key;
}

Optional<HashValue> GetKey(const StategraphTrack& track) {
  return GetKey(track.GetSelectionParams());
}

void SetKey(VariantMap* map, const char* key) {
  (*map)[Hash("key")] = Hash(key);
}

class TestTrack : public StategraphTrack {
 public:
  explicit TestTrack(const char* key) : StategraphTrack() {
    SetKey(&selection_params_, key);
  }
};

// Mock StategraphState class used for testing.
class TestState : public StategraphState {
 public:
  // Simple selector that will pick a Track based on an arbitrary key value.
  struct KeyMatchSelector : TrackSelector {
    Optional<size_t> Select(const VariantMap& args,
                            Span<Type> choices) override {
      auto key = GetKey(args);
      if (key) {
        for (size_t i = 0; i < choices.size(); ++i) {
          const VariantMap& params = choices[i]->GetSelectionParams();
          auto match = GetKey(params);
          if (key == match) {
            return i;
          }
        }
      }
      return Optional<size_t>();
    }
  };

  explicit TestState(const char* name) : StategraphState(Hash(name)) {
    SetSelector(MakeUnique<KeyMatchSelector>());
  }

  void AddTrack(std::unique_ptr<TestTrack> track) {
    StategraphState::AddTrack(
        std::unique_ptr<StategraphTrack>(track.release()));
  }
};

TEST(StategraphSignalTest, NoTracks) {
  TestState state("test");

  VariantMap args;
  const StategraphTrack* track = state.SelectTrack(args);
  EXPECT_THAT(track, IsNull());
}

TEST(StategraphSignalTest, SelectTrack) {
  TestState state("test");
  state.AddTrack(MakeUnique<TestTrack>("alpha"));
  state.AddTrack(MakeUnique<TestTrack>("beta"));
  state.AddTrack(MakeUnique<TestTrack>("gamma"));
  const StategraphTrack* track = nullptr;
  Optional<HashValue> key;

  VariantMap args;
  SetKey(&args, "alpha");
  track = state.SelectTrack(args);
  EXPECT_THAT(track, NotNull());
  key = GetKey(*track);
  EXPECT_THAT(key.value(), Hash("alpha"));

  SetKey(&args, "gamma");
  track = state.SelectTrack(args);
  EXPECT_THAT(track, NotNull());
  key = GetKey(*track);
  EXPECT_THAT(key.value(), Hash("gamma"));

  SetKey(&args, "delta");
  track = state.SelectTrack(args);
  EXPECT_THAT(track, IsNull());
}

}  // namespace
}  // namespace lull
