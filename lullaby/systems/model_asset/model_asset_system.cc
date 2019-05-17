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

#include "lullaby/systems/model_asset/model_asset_system.h"

#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/blend_shape/blend_shape_system.h"
#include "lullaby/systems/collision/collision_provider.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/render/mesh_factory.h"
#include "lullaby/systems/render/render_helpers.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/util/filename.h"
#include "lullaby/generated/model_asset_def_generated.h"

namespace lull {

constexpr HashValue kModelAssetDefHash = ConstHash("ModelAssetDef");

ModelAssetSystem::ModelAssetInstance::ModelAssetInstance(
    Registry* registry, std::shared_ptr<ModelAsset> asset,
    bool create_distinct_meshes)
    : registry_(registry),
      model_asset_(std::move(asset)),
      create_distinct_meshes_(create_distinct_meshes) {}

ModelAssetSystem::ModelAssetInstance::~ModelAssetInstance() {
  auto* texture_factory = registry_->Get<TextureFactory>();
  if (texture_factory) {
    for (auto& iter : textures_) {
      texture_factory->ReleaseTexture(iter.first);
    }
  }
}

void ModelAssetSystem::ModelAssetInstance::Finalize() {
  auto* mesh_factory = registry_->Get<MeshFactory>();
  auto* collision_system = registry_->Get<CollisionSystem>();

  if (collision_system != nullptr &&
      collision_system->GetNumCollisionProviders() > 0) {
    model_asset_->CopyMeshToCollisionData();
  }

  // Build a single Mesh that can be shared between all Entities created from
  // this model.  However, if the model has BlendShapes, then we don't want to
  // share the mesh and, instead, we will let the BlendShapeSystem manage the
  // mesh data.
  if (mesh_factory && !create_distinct_meshes_) {
    auto* blend_shape_system = registry_->Get<BlendShapeSystem>();
    if (blend_shape_system == nullptr || !model_asset_->HasBlendShapes()) {
      MeshData& data = model_asset_->GetMutableMeshData();
      mesh_ = mesh_factory->CreateMesh(std::move(data));
    }
  }

  auto* texture_factory = registry_->Get<TextureFactory>();
  if (texture_factory) {
    for (ModelAsset::TextureInfo& info : model_asset_->GetMutableTextures()) {
      if (!info.data.IsEmpty()) {
        if (info.name.empty()) {
          LOG(ERROR) << "Texture image has no name, ignoring.";
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
        LOG(ERROR) << "Texture must have either a filename or image data.";
      }
    }
  }
}

MeshPtr ModelAssetSystem::ModelAssetInstance::GetMesh() const { return mesh_; }

ModelAssetSystem::ModelAssetSystem(Registry* registry)
    : System(registry),
      models_(ResourceManager<ModelAssetInstance>::kCacheFullyOnCreate) {
  RegisterDef<ModelAssetDefT>(this);
}

void ModelAssetSystem::Initialize() {
  auto* mesh_factory = registry_->Get<MeshFactory>();
  if (mesh_factory) {
    empty_mesh_ = mesh_factory->EmptyMesh();
  }
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
                                   const ModelAssetDef* data,
                                   HashValue override_render_pass) {
  bool create_distinct_meshes = false;
  if (data) {
    create_distinct_meshes = data->create_distinct_meshes();
  }
  LoadModel(filename, create_distinct_meshes);

  const HashValue key = Hash(filename);
  auto instance = models_.Find(key);
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
  if (data) {
    ReadFlatbuffer(&setup.def, data);
  }
  if (setup.def.pass == 0) {
    setup.def.pass = RenderSystem::kDefaultPass;
  }

  if (override_render_pass) {
    setup.def.pass = override_render_pass;
  }

  if (instance->IsReady()) {
    FinalizeEntity(setup);
  } else {
    auto* render_system = registry_->Get<RenderSystem>();
    if (render_system) {
      // We want to create the RenderComponent with an empty mesh early to make
      // sure that IsReadyToRender doesn't return true before we're ready.
      render_system->Create(setup.entity, setup.def.pass);
      render_system->SetMesh({setup.entity, setup.def.pass}, empty_mesh_);
    }
    pending_entities_[key].push_back(setup);
  }
}

const ModelAsset* ModelAssetSystem::GetModelAsset(Entity entity) {
  auto pair = entity_to_asset_hash_.find(entity);
  if (pair == entity_to_asset_hash_.end()) {
    return nullptr;
  }

  std::shared_ptr<ModelAssetInstance> instance = models_.Find(pair->second);

  if (instance == nullptr) {
    return nullptr;
  }

  return instance->GetAsset().get();
}

void ModelAssetSystem::LoadModel(string_view filename,
                                 bool create_distinct_meshes) {
  const HashValue key = Hash(filename);

  models_.Create(key, [this, filename, key, create_distinct_meshes]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    auto callback = [this, key]() { Finalize(key); };
    auto model_asset =
        asset_loader->LoadAsync<ModelAsset>(std::string(filename), callback);
    return std::make_shared<ModelAssetInstance>(registry_, model_asset,
                                                create_distinct_meshes);
  });
}

void ModelAssetSystem::ReleaseModel(HashValue key) { models_.Release(key); }

void ModelAssetSystem::Finalize(HashValue key) {
  std::shared_ptr<ModelAssetInstance> instance = models_.Find(key);
  if (instance == nullptr) {
    return;
  }

  instance->Finalize();

  auto iter = pending_entities_.find(key);
  if (iter != pending_entities_.end()) {
    for (const EntitySetupInfo& setup : iter->second) {
      FinalizeEntity(setup);
    }
    pending_entities_.erase(iter);
  }
}

// Returns the first ModelAssetMaterialDef instance in the ModelAssetDef that
// "matches" the required LOD and submesh_index.
static const ModelAssetMaterialDefT* FindMaterialDef(const ModelAssetDefT& def,
                                                     int lod,
                                                     int submesh_index) {
  for (const ModelAssetMaterialDefT& material : def.materials) {
    const bool lod_match = (material.lod == -1 || material.lod == lod);
    const bool submesh_match =
        (material.submesh == -1 || material.submesh == submesh_index);
    if (lod_match && submesh_match) {
      return &material;
    }
  }
  return nullptr;
}

void ModelAssetSystem::FinalizeEntity(const EntitySetupInfo& setup) {
  // We only support a single level of detail.
  const int lod = 0;

  // A cahce of textures created in this function.  More information below.
  std::vector<TexturePtr> local_texture_cache;

  auto asset = setup.instance->GetAsset();
  auto* blend_shape_system = registry_->Get<BlendShapeSystem>();
  auto* render_system = registry_->Get<RenderSystem>();
  auto* texture_factory = registry_->Get<TextureFactory>();
  auto* collision_system = registry_->Get<CollisionSystem>();
  if (render_system && texture_factory) {
    render_system->Create(setup.entity, setup.def.pass);

    const auto& materials = asset->GetMutableMaterials();
    for (size_t i = 0; i < materials.size(); ++i) {
      const int submesh_index = static_cast<int>(i);
      const ModelAssetMaterialDefT* def =
          FindMaterialDef(setup.def, lod, submesh_index);

      if (def) {
        // Create a copy of the reference material.  The def will be used to
        // override/extend the data in this material.
        MaterialInfo material = materials[i];

        // First, update the material shading model if it is being overridden.
        if (!def->shading_model.empty()) {
          material.SetShadingModel(def->shading_model);
        }

        // Next copy all the material properties from the def into the material.
        VariantMap properties;
        VariantMapFromVariantMapDefT(def->properties, &properties);
        for (HashValue feature : def->shading_features) {
          properties[feature] = true;
        }
        material.SetProperties(properties);

        // The def may specify its own set of textures to use, so create them
        // here. We need to keep the references to these textures "alive" long
        // enough for the RenderSystem to associate them with the Entity.
        // This mapping occurs when we call RenderSystem::SetMaterial. If we
        // don't cache these textures, the TextureFactory will "forget" about
        // them when the shared_ptr goes out-of-scope. Then, when the
        // RenderSystem attempts to map the texture to the material, it will
        // have to reload the texture using potentially incorrect settings.
        for (const ModelAssetTextureDefT& texture_def : def->textures) {
          TexturePtr texture =
              texture_factory->CreateTexture(texture_def.texture);
          if (texture) {
            material.SetTexture(texture_def.usage, texture_def.texture.file);
            local_texture_cache.emplace_back(std::move(texture));
          }
        }
        render_system->SetMaterial({setup.entity, DrawableIndex(submesh_index)},
                                   material);
        local_texture_cache.clear();

        // Finally, update uniform data for any shader uniforms specified in the
        // def.
        ApplyUniforms(setup.entity, setup.def.pass, submesh_index, material,
                      def);
      } else {
        render_system->SetMaterial({setup.entity, DrawableIndex(submesh_index)},
                                   materials[i]);
      }
    }
  }

  if (blend_shape_system && !asset->GetBlendShapeNames().empty()) {
    blend_shape_system->InitBlendShape(
        setup.entity, asset->GetBaseBlendMesh().CreateHeapCopy(),
        asset->GetBlendShapeFormat(),
        asset->GetBaseBlendShapeData().CreateHeapCopy(),
        BlendShapeSystem::BlendMode::kInterpolate);
    const Span<HashValue>& shapes = asset->GetBlendShapeNames();
    for (size_t i = 0; i < shapes.size(); ++i) {
      blend_shape_system->AddBlendShape(
          setup.entity, shapes[i],
          asset->GetBlendShapeData(i).CreateHeapCopy());
    }
  } else if (render_system) {
    MeshPtr mesh = setup.instance->GetMesh();
    if (mesh) {
      render_system->SetMesh({setup.entity, setup.def.pass}, mesh);
    } else {
      const MeshData& data = asset->GetMeshData();
      render_system->SetMesh({setup.entity, setup.def.pass}, data);
    }
  }

  if (asset->HasValidSkeleton()) {
    auto* rig_system = registry_->Get<RigSystem>();
    if (rig_system) {
      const RigSystem::BoneIndices parent_indices =
          asset->GetParentBoneIndices();
      const RigSystem::BoneIndices shader_indices =
          asset->GetShaderBoneIndices();
      const RigSystem::Pose inverse_bind_pose = asset->GetInverseBindPose();
      std::vector<std::string> bone_names = {asset->GetBoneNames().begin(),
                                             asset->GetBoneNames().end()};
      rig_system->SetRig(setup.entity, parent_indices, inverse_bind_pose,
                         shader_indices, std::move(bone_names));
    } else if (render_system) {
      const size_t num_bones = asset->GetParentBoneIndices().size();
      if (num_bones > 0) {
        ClearBoneTransforms(render_system, setup.entity,
                            static_cast<int>(num_bones));
      }
    }
  }

  if (collision_system != nullptr) {
    collision_system->ForEachCollisionProvider(
        [&setup](CollisionProvider* provider) {
          provider->CreateMeshShape(
              setup.entity, setup.instance->GetAsset()->GetId(),
              setup.instance->GetAsset()->GetCollisionData());
        });
  }

  setup.instance->SetReady(true);
}

static Span<uint8_t> GetUniformData(const ShaderUniformDefT& uniform,
                                    const MaterialInfo& material) {
  if (uniform.array_size != 0) {
    LOG(DFATAL) << "Arrays not supported.";
    return {};
  } else if (uniform.name.empty()) {
    LOG(DFATAL) << "Missing uniform name.";
    return {};
  }

  const HashValue key = Hash(uniform.name);
  const float* data = nullptr;
  size_t size = 0;

  switch (uniform.type) {
    case ShaderDataType_Float1: {
      const auto* ptr = material.GetProperty<float>(key);
      data = ptr;
      size = 1;
      break;
    }
    case ShaderDataType_Float3: {
      const auto* ptr = material.GetProperty<mathfu::vec3>(key);
      data = &ptr->x;
      size = 3;
      break;
    }
    case ShaderDataType_Float4: {
      const auto* ptr = material.GetProperty<mathfu::vec4>(key);
      data = &ptr->x;
      size = 4;
      break;
    }
    default: {
      LOG(DFATAL) << "Only support 1d, 3d, and 4d float types.";
      break;
    }
  }
  return {reinterpret_cast<const uint8_t*>(data), size * sizeof(float)};
}

void ModelAssetSystem::ApplyUniforms(Entity entity, HashValue pass,
                                     int submesh_index,
                                     const MaterialInfo& material,
                                     const ModelAssetMaterialDefT* def) {
  if (def->lod != -1 && def->lod != 0) {
    // We only support LOD 0 for now.  An LOD of -1 applies to all LODs.
    return;
  } else if (def->submesh != -1 && def->submesh != submesh_index) {
    return;
  }

  auto* render_system = registry_->Get<RenderSystem>();
  for (const ShaderUniformDefT& uniform : def->shading_uniforms) {
    const Span<uint8_t> data = GetUniformData(uniform, material);
    if (data.data()) {
      render_system->SetUniform({entity, pass, submesh_index}, uniform.name,
                                uniform.type, data);
    }
  }
}

}  // namespace lull
