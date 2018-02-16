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

#ifndef LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_H_
#define LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_H_

#include <memory>
#include "lullaby/generated/model_asset_def_generated.h"
#include "lullaby/util/entity.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/rig/rig_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/model_def_generated.h"

namespace lull {

/// Parses a .lullmodel file and sets up the corresponding mesh and surface
/// properties for entities in the RenderSystem.
class ModelAsset : public Asset {
 public:
  ModelAsset(Registry* registry, const ModelAssetDef* asset_def);
  ~ModelAsset() override;

  /// Registers an Entity with this model. Once the model is loaded, the
  /// Entity will be updated with the information contained in the model file.
  /// If the model is already loaded, the association happens right away.
  void AddEntity(Entity entity);

  /// Extracts the data from the .lullmodel file and stores it locally.
  void OnLoad(const std::string& filename, std::string* data) override;

  /// Updates all Entities that were waiting for the model to finish loading.
  void OnFinalize(const std::string& filename, std::string* data) override;

 private:
  void SetMesh(Entity entity);
  void SetRig(Entity entity);
  void SetMaterials(Entity entity);
  void BuildMesh();
  void BuildMaterials();
  void PrepareTextures();
  std::string CreateTexture(const TextureDef* texture_def);
  MaterialInfo BuildMaterialInfo(const MaterialDef* material_def, int lod,
                                 int submesh_index);
  void ApplyUniforms(Entity entity, const MaterialInfo& material,
                     const ModelAssetMaterialDef* def);

  Registry* registry_ = nullptr;
  std::string raw_data_;
  const ModelDef* model_def_ = nullptr;
  const ModelAssetDef* asset_def_ = nullptr;
  std::vector<Entity> pending_entities_;
  std::vector<HashValue> textures_;
  std::unique_ptr<MeshData> mesh_data_;
  std::vector<MaterialInfo> materials_;
  std::vector<VariantMap> extra_material_properties_;
  MeshPtr mesh_;
  bool ready_ = false;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_H_
