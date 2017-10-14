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

#ifndef LULLABY_SYSTEMS_AUDIO_SOUND_ASSET_H_
#define LULLABY_SYSTEMS_AUDIO_SOUND_ASSET_H_

#include <memory>
#include <string>
#include <vector>

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/typeid.h"

namespace lull {

class SoundAsset {
 public:
  explicit SoundAsset(Entity entity, const std::string& filename);

  const std::string& GetFilename() const;

  enum LoadStatus {
    kUnloaded,
    kLoaded,
    kStreaming,
    kFailed,
  };

  void SetLoadStatus(LoadStatus status);

  LoadStatus GetLoadStatus() const;

  void AddLoadedListener(Entity entity);

  std::vector<Entity>& GetListeningEntities();

 private:
  std::string filename_;
  std::vector<Entity> entities_;
  LoadStatus load_status_;
};

// There should only be a single SoundAssetPtr owned by SoundAssetManager's
// ResourceManager. SoundAssetWeak should be used everywhere else to ensure
// unloaded sounds are properly cleaned up.
using SoundAssetPtr = std::shared_ptr<SoundAsset>;
using SoundAssetWeakPtr = std::weak_ptr<SoundAsset>;

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SoundAsset);

#endif  // LULLABY_SYSTEMS_AUDIO_SOUND_ASSET_H_
