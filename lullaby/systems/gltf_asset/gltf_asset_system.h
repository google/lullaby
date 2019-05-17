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

#ifndef LULLABY_SYSTEMS_GLTF_ASSET_GLTF_ASSET_SYSTEM_H_
#define LULLABY_SYSTEMS_GLTF_ASSET_GLTF_ASSET_SYSTEM_H_

#include "lullaby/generated/gltf_asset_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/animation/animation_asset.h"
#include "lullaby/systems/gltf_asset/gltf_asset.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/resource_manager.h"
#include "lullaby/util/string_view.h"

namespace lull {

/// Creates runtime structures in a variety of systems (including Render, Skin,
/// and Animation) for a given Entity using data loaded from a GLTF asset file.
///
/// The system will create child Entities based on the hierarchy of the GLTF
/// asset. While it is possible to look up these child Entities using the names
/// of nodes in the GLTF and change their properties, changing the hierarchy of
/// these Entities may break configurations of other systems.
class GltfAssetSystem : public System {
 public:
  struct InitParams {
    /// If true, skips conversion of mesh normals and tangents to orientations.
    bool preserve_normal_tangent = false;
  };

  explicit GltfAssetSystem(Registry* registry);
  GltfAssetSystem(Registry* registry, const InitParams& params);

  GltfAssetSystem(const GltfAssetSystem&) = delete;
  GltfAssetSystem& operator=(const GltfAssetSystem&) = delete;

  void Initialize() override;
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  /// Sets up the runtime properties for an Entity using the GLTF file specified
  /// by the filename.
  void CreateGltf(Entity entity, string_view filename);

  /// Returns the GltfAsset for an entity, or nullptr if there is none.
  const GltfAsset* GetGltfAsset(Entity entity);

  /// Releases the loaded gltf file from the internal cache.
  void ReleaseGltf(HashValue key);

  /// Explicitly loads the specified GLTF file and stores it in the internal
  /// cache.
  void LoadGltf(string_view filename);

 private:
  class GltfAssetInstance {
   public:
    GltfAssetInstance(Registry* registry, std::shared_ptr<GltfAsset> asset);
    ~GltfAssetInstance();

    void Finalize();
    bool IsReady() const { return ready_; }
    void SetReady(bool b) { ready_ = b; }

    size_t GetNumMeshes() const { return meshes_.size(); }
    MeshPtr GetMesh(int index) const { return meshes_[index]; }

    size_t GetNumAnimations() const { return animations_.size(); }
    AnimationAssetPtr GetAnimation(int index) const {
      return animations_[index];
    }

    std::shared_ptr<GltfAsset> GetAsset() const { return gltf_asset_; }

   private:
    Registry* registry_;
    std::shared_ptr<GltfAsset> gltf_asset_;
    std::vector<MeshPtr> meshes_;
    std::unordered_map<HashValue, TexturePtr> textures_;
    std::vector<AnimationAssetPtr> animations_;
    HashValue anim_id_ = 0;
    bool ready_ = false;
  };

  struct EntitySetupInfo {
    Entity entity = kNullEntity;
    std::shared_ptr<GltfAssetInstance> instance;
  };

  void Finalize(HashValue key);
  void RegisterEntity(const EntitySetupInfo& setup);
  void FinalizeEntity(const EntitySetupInfo& setup);

  ResourceManager<GltfAssetInstance> gltfs_;
  std::unordered_map<HashValue, std::vector<EntitySetupInfo>> pending_entities_;
  std::unordered_map<Entity, HashValue> entity_to_asset_hash_;
  MeshPtr empty_mesh_;
  bool preserve_normal_tangent_ = false;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::GltfAssetSystem);

#endif  // LULLABY_SYSTEMS_GLTF_ASSET_GLTF_ASSET_SYSTEM_H_
