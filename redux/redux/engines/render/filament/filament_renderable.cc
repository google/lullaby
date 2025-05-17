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

#include "redux/engines/render/filament/filament_renderable.h"

#include <stdint.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "filament/Box.h"
#include "filament/Material.h"
#include "filament/RenderableManager.h"
#include "filament/Scene.h"
#include "filament/TransformManager.h"
#include "math/mat3.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "utils/EntityManager.h"
#include "redux/engines/render/filament/filament_mesh.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_shader.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/mesh.h"
#include "redux/engines/render/renderable.h"
#include "redux/engines/render/shader.h"
#include "redux/engines/render/texture.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"

namespace redux {

static MaterialPropertyType MaterialPropertyTypeFromTextureTarget(
    TextureTarget target) {
  switch (target) {
    case TextureTarget::Normal2D:
      return MaterialPropertyType::Sampler2D;
    case TextureTarget::CubeMap:
      return MaterialPropertyType::SamplerCubeMap;
    default:
      LOG(FATAL) << "Unsupported texture target: " << ToString(target);
  }
}

FilamentRenderable::FilamentRenderable(Registry* registry) {
  fengine_ = GetFilamentEngine(registry);
}

FilamentRenderable::~FilamentRenderable() { DestroyFilamentEntity(); }

void FilamentRenderable::PrepareToRender(const mat4& transform) {
  auto& tm = fengine_->getTransformManager();
  auto& rm = fengine_->getRenderableManager();

  const auto mx = ToFilament(transform);
  auto ti = tm.getInstance(fentity_);
  auto ri = rm.getInstance(fentity_);
  tm.setTransform(ti, mx);

  const uint8_t visibility_mask = visible_ ? 0xff : 0x00;
  rm.setLayerMask(ri, 0xff, visibility_mask);

  if (finstance_) {
    const bool updated = ApplyProperties();
    if (updated || rm.getMaterialInstanceAt(ri, 0) == nullptr) {
      rm.setMaterialInstanceAt(ri, 0, finstance_.get());
    }
  }
}

bool FilamentRenderable::ApplyProperties() {
  bool updated = false;
  auto& rm = fengine_->getRenderableManager();
  auto ri = rm.getInstance(fentity_);

  if (is_skinned_) {
    const MaterialProperty* bones = GetMaterialProperty(ConstHash("Bones"));
    if (bones) {
      CHECK(bones->type == MaterialPropertyType::Float4x4);
      const auto bytes = bones->data.GetByteSpan();
      const auto* data =
          reinterpret_cast<const filament::math::mat4f*>(bytes.data());
      const size_t num_bones = bytes.size() / sizeof(filament::math::mat4f);
      rm.setBones(ri, data, num_bones);
    }
  }
  {
    const MaterialProperty* property =
        GetMaterialProperty(ConstHash("Scissor"));
    if (property) {
      CHECK(property->type == MaterialPropertyType::Int4);
      const vec4i* scissor =
          reinterpret_cast<const vec4i*>(property->data.GetByteSpan().data());
      if (scissor->x < 0 || scissor->y < 0 || scissor->z < 0 ||
          scissor->w < 0) {
        finstance_->unsetScissor();
      } else {
        finstance_->setScissor(scissor->x, scissor->y, scissor->z, scissor->w);
      }
    }
  }
  {
    const MaterialProperty* property =
        GetMaterialProperty(ConstHash("PolygonOffset"));
    if (property) {
      CHECK(property->type == MaterialPropertyType::Float2);
      const vec2* offset =
          reinterpret_cast<const vec2*>(property->data.GetByteSpan().data());
      finstance_->setPolygonOffset(offset->x, offset->y);
    }
  }
  {
    const MaterialProperty* property =
        GetMaterialProperty(ConstHash("BaseTransform"));
    if (property) {
      CHECK(property->type == MaterialPropertyType::Float4x4);
      if (property->generation !=
          property_generations_[ConstHash("BaseTransform")]) {
        const float* arr =
            reinterpret_cast<const float*>(property->data.GetByteSpan().data());
        filament::math::mat3f transform(arr[0], arr[1], arr[2],
                                        arr[4], arr[5], arr[6],
                                        arr[8], arr[9], arr[10]);
        filament::math::float3 translate(arr[12], arr[13], arr[14]);
        const auto aabb = filament::Box::transform(transform, translate, aabb_);
        rm.setAxisAlignedBoundingBox(ri, aabb);
      }
    }
  }

  FilamentShader* fshader = static_cast<FilamentShader*>(shader_.get());
  fshader->ForEachParameter(
      variant_id_, [&](const FilamentShader::ParameterInfo& param) {
        const MaterialProperty* property = GetMaterialProperty(param.key);
        if (property == nullptr) {
          return;
        }
        if (property->generation == property_generations_[param.key]) {
          return;
        }

        property_generations_[param.key] = property->generation;
        if (property->texture) {
          FilamentShader::SetParameter(finstance_.get(), param.name.c_str(),
                                       property->type, property->texture);
        } else {
          FilamentShader::SetParameter(finstance_.get(), param.name.c_str(),
                                       property->type,
                                       property->data.GetByteSpan());
        }
        updated = true;
      });
  return updated;
}

void FilamentRenderable::AddToScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  if (!scenes_.contains(fscene)) {
    scenes_.emplace(fscene);
    if (!fentity_.isNull()) {
      fscene->addEntity(fentity_);
    }
  }
}

void FilamentRenderable::RemoveFromScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  if (scenes_.contains(fscene)) {
    if (!fentity_.isNull()) {
      fscene->remove(fentity_);
    }
    scenes_.erase(fscene);
  }
}

void FilamentRenderable::SetMesh(MeshPtr mesh, size_t part_index) {
  if (mesh_ != mesh) {
    mesh_ = mesh;
    part_index_ = part_index;
    CreateFilamentEntity();
    RebuildConditions();
  }
}

void FilamentRenderable::CreateFilamentEntity() {
  DestroyFilamentEntity();
  if (mesh_ == nullptr) {
    return;
  }

  FilamentMesh* impl = static_cast<FilamentMesh*>(mesh_.get());

  filament::RenderableManager::Builder builder(1);
  impl->PreparePartRenderable(part_index_, builder);

  builder.castShadows(true);
  builder.receiveShadows(true);

  is_skinned_ = false;
  for (VertexUsage usage : impl->GetVertexUsages()) {
    is_skinned_ |= (usage == VertexUsage::BoneWeights);
  }
  if (is_skinned_) {
    builder.skinning(255);
  }

  fentity_ = utils::EntityManager::get().create();
  builder.build(*fengine_, fentity_);

  auto& rm = fengine_->getRenderableManager();
  auto ri = rm.getInstance(fentity_);
  aabb_ = rm.getAxisAlignedBoundingBox(ri);

  for (filament::Scene* scene : scenes_) {
    scene->addEntity(fentity_);
  }
}

void FilamentRenderable::DestroyFilamentEntity() {
  if (fentity_.isNull()) {
    return;
  }

  auto& tm = fengine_->getTransformManager();
  auto& rm = fengine_->getRenderableManager();
  for (filament::Scene* scene : scenes_) {
    scene->remove(fentity_);
  }
  rm.destroy(fentity_);
  tm.destroy(fentity_);
  utils::EntityManager::get().destroy(fentity_);
  fentity_ = utils::Entity();
}

void FilamentRenderable::SetShader(ShaderPtr shader) {
  shader_ = std::move(shader);
  variant_id_ = FilamentShader::kInvalidVariant;
  ReacquireInstance();
}

void FilamentRenderable::Show() { visible_ = true; }

void FilamentRenderable::Hide() { visible_ = false; }

bool FilamentRenderable::IsHidden() const { return visible_; }

void FilamentRenderable::EnableVertexAttribute(VertexUsage usage) {
  disabled_vertices_.erase(usage);
  RebuildConditions();
}

void FilamentRenderable::DisableVertexAttribute(VertexUsage usage) {
  disabled_vertices_.emplace(usage);
  RebuildConditions();
}

bool FilamentRenderable::IsVertexAttributeEnabled(VertexUsage usage) const {
  return !disabled_vertices_.contains(usage);
}

void FilamentRenderable::SetTexture(TextureUsage usage,
                                    const TexturePtr& texture) {
  HashValue key = usage.Hash();
  MaterialProperty& property = properties_[key];
  property.data.Clear();
  if (texture) {
    property.texture = texture;
    property.type = MaterialPropertyTypeFromTextureTarget(texture->GetTarget());

    FilamentTexture* impl = static_cast<FilamentTexture*>(texture.get());
    impl->OnReady([this]() { RebuildConditions(); });
  }
  RebuildConditions();
  ++property.generation;
}

TexturePtr FilamentRenderable::GetTexture(TextureUsage usage) const {
  const HashValue key = usage.Hash();
  const MaterialProperty* property = GetMaterialProperty(key);
  return property ? property->texture : nullptr;
}

void FilamentRenderable::SetProperty(HashValue name, MaterialPropertyType type,
                                     absl::Span<const std::byte> data) {
  if (type == MaterialPropertyType::Feature) {
    CHECK(data.size() == sizeof(bool));
    const bool enable = *reinterpret_cast<const bool*>(data.data());
    if (enable) {
      features_.emplace(name);
    } else {
      features_.erase(name);
    }
    ReacquireInstance();
  } else {
    MaterialProperty& property = properties_[name];
    property.texture = nullptr;
    property.data.Assign(data);
    property.type = type;
    ++property.generation;
  }
}

void FilamentRenderable::InheritProperties(const Renderable& other) {
  const auto* frenderable = static_cast<const FilamentRenderable*>(&other);
  this->shader_ = frenderable->shader_;
  this->visible_ = frenderable->visible_;
  this->conditions_ = frenderable->conditions_;
  this->disabled_vertices_ = frenderable->disabled_vertices_;
  this->properties_ = frenderable->properties_;
}

const FilamentRenderable::MaterialProperty*
FilamentRenderable::GetMaterialProperty(HashValue name) const {
  auto iter = properties_.find(name);
  return iter != properties_.end() ? &iter->second : nullptr;
}

void FilamentRenderable::RebuildConditions() {
  conditions_.clear();
  if (mesh_ == nullptr) {
    return;
  }

  FilamentMesh* fmesh = static_cast<FilamentMesh*>(mesh_.get());
  for (VertexUsage usage : fmesh->GetVertexUsages()) {
    if (!disabled_vertices_.contains(usage)) {
      conditions_.insert(Hash(usage));
    }
  }
  for (const auto& iter : properties_) {
    if (iter.second.texture != nullptr) {
      FilamentTexture* impl =
          static_cast<FilamentTexture*>(iter.second.texture.get());
      if (impl->IsReady()) {
        conditions_.insert(iter.first);
      }
    }
  }
  variant_id_ = FilamentShader::kInvalidVariant;
  ReacquireInstance();
}

void FilamentRenderable::ReacquireInstance() {
  if (shader_ == nullptr) {
    return;
  }

  // Find a shader material instance that fulfills the requirements.
  FilamentShader* impl = static_cast<FilamentShader*>(shader_.get());
  const auto variant = impl->DetermineVariantId(conditions_, features_);

  // If the updated set of requirements requires a new variant instance,
  // abandon the old one and create a new one.
  if (variant_id_ != variant) {
    variant_id_ = variant;
    property_generations_.clear();

    const filament::Material* fmaterial = impl->GetFilamentMaterial(variant);
    finstance_ = MakeFilamentResource(fmaterial->createInstance(), fengine_);
  }
}

}  // namespace redux
