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

#include "lullaby/systems/model_asset/model_asset_system.h"

#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/filename.h"
#include "lullaby/generated/model_asset_def_generated.h"

namespace lull {

constexpr HashValue kModelAssetDefHash = ConstHash("ModelAssetDef");

ModelAssetSystem::ModelAssetSystem(Registry* registry)
    : System(registry),
      models_(ResourceManager<lull::ModelAsset>::kWeakCachingOnly) {
  RegisterDef(this, kModelAssetDefHash);
}

void ModelAssetSystem::PostCreateInit(Entity entity, HashValue type,
                                      const Def* def) {
  if (def != nullptr && type == kModelAssetDefHash) {
    const auto* data = ConvertDef<ModelAssetDef>(def);
    if (data->filename() != nullptr) {
      CreateModel(entity, data->filename()->c_str(), data);
    }
  }
}

void ModelAssetSystem::CreateModel(Entity entity, string_view filename,
                                   const ModelAssetDef* data) {
  LoadModel(filename, data);
  {
    std::shared_ptr<ModelAsset> asset = models_.Find(Hash(filename));
    if (asset) {
      asset->AddEntity(entity);
    }
  }
}

void ModelAssetSystem::LoadModel(string_view filename,
                                 const ModelAssetDef* data) {
  const HashValue key = Hash(filename);

  const std::string ext = GetExtensionFromFilename(filename.to_string());
  models_.Create(key, [this, filename, data]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    return asset_loader->LoadAsync<ModelAsset>(
        filename.to_string(), registry_, data);
  });
}

void ModelAssetSystem::ReleaseModel(HashValue key) {
  models_.Release(key);
}

}  // namespace lull
