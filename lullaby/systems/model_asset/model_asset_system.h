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

#ifndef LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_SYSTEM_H_
#define LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_SYSTEM_H_

#include "lullaby/generated/model_asset_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/model_asset/model_asset.h"
#include "lullaby/util/resource_manager.h"
#include "lullaby/util/string_view.h"

namespace lull {

/// Creates the mesh and surfaces in the RenderSystem for a given Entity using
/// data loaded from a model asset file.
class ModelAssetSystem : public System {
 public:
  explicit ModelAssetSystem(Registry* registry);

  ModelAssetSystem(const ModelAssetSystem&) = delete;
  ModelAssetSystem& operator=(const ModelAssetSystem&) = delete;

  /// Sets up the mesh and surface properties for an Entity using the model
  /// file references in the def.
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  /// Sets up the mesh and surface properties for an Entity using the model
  /// file specified by the filename.
  void CreateModel(Entity entity, string_view filename,
                   const ModelAssetDef* data = nullptr);

  /// Explicitly loads the specified model file and stores it in the internal
  /// cache.
  void LoadModel(string_view filename, const ModelAssetDef* data = nullptr);

  /// Releases the loaded model file from the internal cache.
  void ReleaseModel(HashValue key);

 private:
  ResourceManager<ModelAsset> models_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ModelAssetSystem);

#endif  // LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_SYSTEM_H_
