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

#ifndef REDUX_SYSTEMS_MODEL_MODEL_SYSTEM_H_
#define REDUX_SYSTEMS_MODEL_MODEL_SYSTEM_H_

#include "absl/container/flat_hash_map.h"
#include "redux/engines/physics/collision_shape.h"
#include "redux/engines/render/mesh.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/model/model_asset.h"
#include "redux/systems/model/model_def_generated.h"

namespace redux {

// Creates the mesh and surfaces in the RenderSystem for a given Entity using
// data loaded from a model asset file.
class ModelSystem : public System {
 public:
  explicit ModelSystem(Registry* registry);

  void OnRegistryInitialize();

  // Explicitly loads the specified model file and stores it in the internal
  // cache.
  void LoadModel(std::string_view uri);

  // Releases the loaded model file from the internal cache.
  void ReleaseModel(HashValue key);

 private:
  struct ModelInstance {
    MeshPtr mesh;
    CollisionShapePtr collision_shape;
    absl::flat_hash_map<HashValue, TexturePtr> textures;
  };

  struct EntitySetupInfo {
    Entity entity = kNullEntity;
    HashValue model_id = HashValue(0);
    HashValue render_scene = HashValue(0);
    bool distinct = false;
  };

  void AddFromDef(Entity entity, const ModelDef& def);

  ModelInstance GenerateModelInstance(const ModelAsset& asset);

  void FinalizeModel(HashValue key);
  void FinalizeEntity(const EntitySetupInfo& setup);

  void SetMesh(const EntitySetupInfo& setup);
  void SetMaterials(const EntitySetupInfo& setup);
  void SetRig(const EntitySetupInfo& setup);

  // The "raw" models as loaded directly off disk.
  ResourceManager<ModelAsset> models_;
  absl::flat_hash_map<HashValue, ModelInstance> instances_;
  absl::flat_hash_map<HashValue, std::vector<EntitySetupInfo>>
      pending_entities_;
  MeshPtr empty_mesh_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ModelSystem);

#endif  // REDUX_SYSTEMS_MODEL_MODEL_SYSTEM_H_
