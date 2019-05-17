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

#include "lullaby/systems/gltf_asset/gltf_asset_system.h"

#include "lullaby/generated/flatbuffers/vertex_attribute_def_generated.h"
#include "lullaby/generated/gltf_asset_def_generated.h"
#include "lullaby/modules/animation_channels/skeleton_channel.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/tinygltf/tinygltf_util.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/blend_shape/blend_shape_system.h"
#include "lullaby/systems/name/name_system.h"
#include "lullaby/systems/render/mesh_factory.h"
#include "lullaby/systems/render/render_helpers.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/systems/skin/skin_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/filename.h"
#include "mathfu/constants.h"
#include "mathfu/io.h"

namespace lull {

constexpr HashValue kGltfAssetDefHash = ConstHash("GltfAssetDef");
// TODO: let this be customized per-Entity, per-mesh.
constexpr HashValue kRenderPass = ConstHash("Opaque");

GltfAssetSystem::GltfAssetInstance::GltfAssetInstance(
    Registry* registry, std::shared_ptr<GltfAsset> asset)
    : registry_(registry), gltf_asset_(std::move(asset)) {}

GltfAssetSystem::GltfAssetInstance::~GltfAssetInstance() {
  auto* texture_factory = registry_->Get<TextureFactory>();
  if (texture_factory) {
    for (auto& iter : textures_) {
      texture_factory->ReleaseTexture(iter.first);
    }
  }
}

void GltfAssetSystem::GltfAssetInstance::Finalize() {
  // Convert MeshDatas into MeshPtrs and store the pointers in the instance
  // wrapping the asset.
  auto* mesh_factory = registry_->Get<MeshFactory>();
  auto* blend_shape_system = registry_->Get<BlendShapeSystem>();
  if (mesh_factory) {
    std::vector<GltfAsset::MeshInfo>& infos =
        gltf_asset_->GetMutableMeshInfos();
    meshes_.resize(infos.size());
    for (size_t i = 0; i < meshes_.size(); ++i) {
      // Meshes with blend shapes copy the original mesh data each time the
      // asset is instantiated since it will be modified each update. Meshes
      // without blend shapes are converted to MeshPtrs here to be shared
      // between all Entities that instantiate the asset.
      if (blend_shape_system == nullptr || !infos[i].HasBlendShapes()) {
        meshes_[i] = mesh_factory->CreateMesh(std::move(infos[i].mesh_data));
      }
    }
  }

  // Create textures for each TextureInfo, which may involve using pre-loaded
  // ImageData, then store the TexturePtrs in the instance wrapping the asset.
  auto* texture_factory = registry_->Get<TextureFactory>();
  if (texture_factory) {
    for (GltfAsset::TextureInfo& info : gltf_asset_->GetMutableTextures()) {
      if (!info.data.IsEmpty()) {
        if (info.name.empty()) {
          LOG(DFATAL) << "Texture image has no name, ignoring.";
          continue;
        }

        const HashValue key = Hash(info.name);
        TexturePtr texture = texture_factory->CreateTexture(
            key, std::move(info.data), info.params);
        textures_[key] = texture;
      } else if (!info.file.empty()) {
        const HashValue key = Hash(info.file);
        TexturePtr texture =
            texture_factory->LoadTexture(info.file.c_str(), info.params);
        textures_[key] = texture;
      } else {
        LOG(DFATAL) << "Texture must have either a filename or image data.";
      }
    }
  }

  // Create animation assets for each RigAnim and store pointers in the instance
  // wrapping the asset.
  auto* animation_system = registry_->Get<AnimationSystem>();
  if (animation_system) {
    for (GltfAsset::AnimationInfo& anim_info :
         gltf_asset_->GetMutableAnimationInfos()) {
      if (anim_info.num_splines > 0) {
        const HashValue id = Hash(gltf_asset_->GetId(), ":" + anim_info.name);

        // Moving the context unique_ptr into a shared_ptr gives the shared_ptr
        // a type-specific deleter function. static_pointer_cast maintains this
        // deleter but changes the type to void. The end result is a
        // shared_ptr<void> that calls the correct delete function.
        std::shared_ptr<SkeletonChannel::AnimationContext> shared_context(
            std::move(anim_info.context));
        animations_.emplace_back(animation_system->CreateAnimation(
            id, std::move(anim_info.splines), anim_info.num_splines,
            std::static_pointer_cast<void>(shared_context)));
      }
    }
  }

  SetReady(true);
}

GltfAssetSystem::GltfAssetSystem(Registry* registry)
    : GltfAssetSystem(registry, InitParams()) {}

GltfAssetSystem::GltfAssetSystem(Registry* registry, const InitParams& params)
    : System(registry),
      gltfs_(ResourceManager<GltfAssetInstance>::kCacheFullyOnCreate),
      preserve_normal_tangent_(params.preserve_normal_tangent) {
  RegisterDef<GltfAssetDefT>(this);
  RegisterDependency<TransformSystem>(this);
}

void GltfAssetSystem::Initialize() {
  auto* mesh_factory = registry_->Get<MeshFactory>();
  if (mesh_factory) {
    empty_mesh_ = mesh_factory->EmptyMesh();
  }

  auto* animation_system = registry_->Get<AnimationSystem>();
  if (animation_system) {
    SkeletonChannel::Setup(registry_, 4);
  }
}

void GltfAssetSystem::PostCreateInit(Entity entity, HashValue type,
                                     const Def* def) {
  if (def != nullptr && type == kGltfAssetDefHash) {
    const auto* data = ConvertDef<GltfAssetDef>(def);
    if (data->filename() != nullptr) {
      CreateGltf(entity, data->filename()->c_str());
    }
  }
}

void GltfAssetSystem::CreateGltf(Entity entity, string_view filename) {
  const HashValue key = Hash(filename);
  LoadGltf(filename);

  auto instance = gltfs_.Find(key);
  if (instance == nullptr) {
    return;
  }
  if (entity == kNullEntity) {
    return;
  }

  entity_to_asset_hash_[entity] = key;

  EntitySetupInfo setup;
  setup.entity = entity;
  setup.instance = instance;

  if (instance->IsReady()) {
    FinalizeEntity(setup);
  } else {
    pending_entities_[key].push_back(setup);
  }
}

const GltfAsset* GltfAssetSystem::GetGltfAsset(Entity entity) {
  auto pair = entity_to_asset_hash_.find(entity);
  if (pair == entity_to_asset_hash_.end()) {
    return nullptr;
  }

  std::shared_ptr<GltfAssetInstance> instance = gltfs_.Find(pair->second);

  if (instance == nullptr) {
    return nullptr;
  }

  return instance->GetAsset().get();
}

void GltfAssetSystem::LoadGltf(string_view filename) {
  const HashValue key = Hash(filename);

  gltfs_.Create(key, [this, filename, key]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    auto callback = [this, key]() { Finalize(key); };
    auto gltf_asset = asset_loader->LoadAsync<GltfAsset>(
        std::string(filename), registry_, preserve_normal_tangent_, callback);
    return std::make_shared<GltfAssetInstance>(registry_, gltf_asset);
  });
}

void GltfAssetSystem::ReleaseGltf(HashValue key) { gltfs_.Release(key); }

void GltfAssetSystem::Finalize(HashValue key) {
  std::shared_ptr<GltfAssetInstance> instance = gltfs_.Find(key);
  if (instance == nullptr) {
    return;
  }

  // Finalizing the instance creates the render pointers that all Entities using
  // the asset will share.
  instance->Finalize();

  auto iter = pending_entities_.find(key);
  if (iter != pending_entities_.end()) {
    for (const EntitySetupInfo& setup : iter->second) {
      FinalizeEntity(setup);
    }
    pending_entities_.erase(iter);
  }
}

void GltfAssetSystem::FinalizeEntity(const EntitySetupInfo& setup) {
  auto asset = setup.instance->GetAsset();
  auto* entity_factory = registry_->Get<EntityFactory>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create one Entity per node and assign SQT properties.
  const Span<GltfAsset::NodeInfo> node_infos = asset->GetNodeInfos();
  std::vector<Entity> entities(node_infos.size());
  for (size_t i = 0; i < node_infos.size(); ++i) {
    const GltfAsset::NodeInfo& info = node_infos[i];
    entities[i] = entity_factory->Create();
    transform_system->Create(entities[i], info.transform);
  }

  // Assign children and parents. Do root nodes before child nodes to perform
  // as few transform updates as possible.
  const Span<int> root_nodes = asset->GetRootNodes();
  for (const int node : root_nodes) {
    transform_system->AddChild(setup.entity, entities[node]);
  }
  for (size_t i = 0; i < node_infos.size(); ++i) {
    const GltfAsset::NodeInfo& info = node_infos[i];
    for (const int child : info.children) {
      transform_system->AddChild(entities[i], entities[child]);
    }
  }

  // Assign names if the NameSystem is present.
  auto* name_system = registry_->Get<NameSystem>();
  if (name_system) {
    for (size_t i = 0; i < node_infos.size(); ++i) {
      name_system->SetName(entities[i], node_infos[i].name);
    }
  }

  // Assign meshes and materials if the RenderSystem is present, as well as
  // blend shapes if the BlendShapeSystem is present.
  auto* render_system = registry_->Get<RenderSystem>();
  if (render_system) {
    auto* blend_shape_system = registry_->Get<BlendShapeSystem>();
    for (size_t i = 0; i < node_infos.size(); ++i) {
      const Entity entity = entities[i];
      const GltfAsset::NodeInfo& node_info = node_infos[i];
      if (node_info.mesh != kInvalidTinyGltfIndex) {
        const GltfAsset::MeshInfo& mesh_info =
            asset->GetMeshInfo(node_info.mesh);
        render_system->Create(entity, kRenderPass);
        // If no blend shapes are present, use the already-created MeshPtr.
        if (blend_shape_system == nullptr || !mesh_info.HasBlendShapes()) {
          MeshPtr mesh = setup.instance->GetMesh(node_info.mesh);
          if (mesh) {
            RenderSystem::Drawable drawable(entity, kRenderPass, 0);
            render_system->SetMesh(drawable, mesh);

            // If a mesh contains bone indices but doesn't have a skin, set its
            // bone transforms to identity to ensure the base mesh renders
            // correctly.
            if (node_info.skin == kInvalidTinyGltfIndex) {
              for (size_t j = 0; j < GetNumSubmeshes(mesh); ++j) {
                const VertexFormat format = GetVertexFormat(mesh, j);
                if (format.GetAttributeWithUsage(
                        VertexAttributeUsage_BoneIndices) != nullptr) {
                  ClearBoneTransforms(render_system, entity, kMaxNumBones);
                  break;
                }
              }
            }
          } else {
            LOG(DFATAL) << "No MeshPtr for Node. Likely missing "
                        << "BlendShapeSystem.";
          }
        } else {
          // For Nodes with blend shapes, create a copy of the original MeshData
          // for BlendShapeSystem to modify, as well as copies of all blend
          // shape data since the base asset could be released.
          blend_shape_system->InitBlendShape(
              entity, mesh_info.mesh_data.CreateHeapCopy(),
              mesh_info.blend_shape_format,
              mesh_info.base_blend_shape.CreateHeapCopy(),
              preserve_normal_tangent_
                  ? BlendShapeSystem::BlendMode::kDisplacement
                  : BlendShapeSystem::BlendMode::kInterpolate);
          for (size_t j = 0; j < mesh_info.blend_shapes.size(); ++j) {
            // GLTF doesn't allow specifying the names of blend shapes, so
            // instead we add them with a HashValue equal to their original
            // index.
            blend_shape_system->AddBlendShape(
                entity, static_cast<HashValue>(j),
                mesh_info.blend_shapes[j].CreateHeapCopy());
          }

          // According to the spec, weights first come from the node, then the
          // mesh, then default to zero.
          if (!node_info.blend_shape_weights.empty()) {
            blend_shape_system->UpdateWeights(entity,
                                              node_info.blend_shape_weights);
          } else if (!mesh_info.blend_shape_weights.empty()) {
            blend_shape_system->UpdateWeights(entity,
                                              mesh_info.blend_shape_weights);
          } else {
            std::vector<float> zero_weights(mesh_info.blend_shapes.size(), 0.f);
            blend_shape_system->UpdateWeights(entity, zero_weights);
          }
        }
        if (mesh_info.material_index != kInvalidTinyGltfIndex) {
          render_system->SetMaterial(
              entity, asset->GetMaterialInfo(mesh_info.material_index));
        }
      }
    }
  }

  // Assign skins if the SkinSystem is present.
  auto* skin_system = registry_->Get<SkinSystem>();
  if (skin_system) {
    for (size_t i = 0; i < node_infos.size(); ++i) {
      const GltfAsset::NodeInfo& node_info = node_infos[i];
      if (node_info.skin != kInvalidTinyGltfIndex) {
        const GltfAsset::SkinInfo& skin_info =
            asset->GetSkinInfo(node_info.skin);

        // Map each bone index to an Entity.
        std::vector<Entity> bones(skin_info.bones.size());
        for (size_t i = 0; i < bones.size(); ++i) {
          bones[i] = entities[skin_info.bones[i]];
        }
        skin_system->SetSkin(entities[i], bones,
                             skin_info.inverse_bind_matrices);
      }
    }
  }

  // Setup the skeleton for the asset, regardless of whether or not it is
  // skinned. Use all child Entities, even though some may be irrelevant during
  // animation and skinning. Then, play a default looping animations.
  auto* animation_system = registry_->Get<AnimationSystem>();
  if (animation_system) {
    animation_system->SetSkeleton(setup.entity, entities);
    if (setup.instance->GetNumAnimations() > 0) {
      lull::PlaybackParameters params;
      params.looping = true;
      animation_system->PlayAnimation(setup.entity,
                                      SkeletonChannel::kChannelName,
                                      setup.instance->GetAnimation(0), params);
    }
  }
}

}  // namespace lull
