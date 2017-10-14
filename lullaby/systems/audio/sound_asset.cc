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

#include "lullaby/systems/audio/sound_asset.h"

namespace lull {

SoundAsset::SoundAsset(Entity entity, const std::string& filename)
    : filename_(filename), load_status_(kUnloaded) {
  entities_.push_back(entity);
}

const std::string& SoundAsset::GetFilename() const { return filename_; }

void SoundAsset::SetLoadStatus(SoundAsset::LoadStatus status) {
  load_status_ = status;
}

SoundAsset::LoadStatus SoundAsset::GetLoadStatus() const {
  return load_status_;
}

void SoundAsset::AddLoadedListener(Entity entity) {
  if (load_status_ == kUnloaded) {
    entities_.push_back(entity);
  }
}

std::vector<Entity>& SoundAsset::GetListeningEntities() {
  return entities_;
}

}  // namespace lull
