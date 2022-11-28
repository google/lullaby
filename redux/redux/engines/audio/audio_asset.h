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

#ifndef REDUX_ENGINES_AUDIO_AUDIO_ASSET_H_
#define REDUX_ENGINES_AUDIO_AUDIO_ASSET_H_

#include <memory>

namespace redux {

// An AudioAsset that provides data for sound playback.
//
// The AudioEngine uses AudioAssets to play Sounds. The same AudioAsset can be
// played by the AudioEngine multiple times resulting in multiple Sound
// instances.
class AudioAsset {
 public:
  using Id = uint64_t;
  static constexpr Id kInvalidAsset = 0;

  ~AudioAsset() = default;

  AudioAsset(const AudioAsset&) = delete;
  AudioAsset& operator=(const AudioAsset&) = delete;

  Id GetId() const { return id_; }

 protected:
  explicit AudioAsset(Id id) : id_(id) {}

 private:
  const Id id_;
};

using AudioAssetPtr = std::shared_ptr<AudioAsset>;

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_AUDIO_ASSET_H_
