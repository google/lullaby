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

#include "filament/RenderableManager.h"
#include "filament/TransformManager.h"
#include "utils/EntityManager.h"
#include "redux/engines/render/filament/filament_mesh.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_shader.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/engines/render/filament/filament_utils.h"

namespace redux {

static constexpr HashValue kRootPart = HashValue(0);

static filament::RenderableManager::PrimitiveType ToFilament(
    MeshPrimitiveType type) {
  switch (type) {
    case MeshPrimitiveType::Triangles:
      return filament::RenderableManager::PrimitiveType::TRIANGLES;
    case MeshPrimitiveType::Points:
      return filament::RenderableManager::PrimitiveType::POINTS;
    case MeshPrimitiveType::Lines:
      return filament::RenderableManager::PrimitiveType::LINES;
    default:
      LOG(FATAL) << "Unsupported primitive type: " << ToString(type);
  }
}

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

FilamentRenderable::~FilamentRenderable() { DestroyParts(); }

void FilamentRenderable::PrepareToRender(const mat4& transform) {
  auto& tm = fengine_->getTransformManager();
  auto& rm = fengine_->getRenderableManager();

  const uint8_t global_visbility = IsHidden(kRootPart) ? 0x00 : 0xff;
  const auto mx = ToFilament(transform);
  for (auto& iter : parts_) {
    auto ti = tm.getInstance(iter.second.fentity);
    auto ri = rm.getInstance(iter.second.fentity);
    tm.setTransform(ti, mx);

    const uint8_t local_visibility = IsHidden(iter.first) ? 0x00 : 0xff;
    const uint8_t visibility = global_visbility & local_visibility;
    rm.setLayerMask(ri, 0xff, visibility);

    if (iter.second.finstance) {
      const bool updated = ApplyProperties(iter.first, iter.second);
      if (updated) {
        rm.setMaterialInstanceAt(ri, 0, iter.second.finstance.get());
      }
    }
  }
}

bool FilamentRenderable::ApplyProperties(HashValue part_name,
                                         PartInstance& part) {
  bool updated = false;

  if (is_skinned_) {
    const MaterialProperty* bones =
        GetMaterialProperty(kRootPart, ConstHash("Bones"));
    if (bones) {
      CHECK(bones->type == MaterialPropertyType::Float4x4);
      const auto bytes = bones->data.GetByteSpan();
      const auto* data =
          reinterpret_cast<const filament::math::mat4f*>(bytes.data());
      const size_t num_bones = bytes.size() / sizeof(filament::math::mat4f);
      auto& rm = fengine_->getRenderableManager();
      auto ri = rm.getInstance(part.fentity);
      rm.setBones(ri, data, num_bones);
    }
  }
  {
    const MaterialProperty* property =
        GetMaterialProperty(part_name, ConstHash("Scissor"));
    if (property) {
      CHECK(property->type == MaterialPropertyType::Int4);
      const vec4i* scissor =
          reinterpret_cast<const vec4i*>(property->data.GetByteSpan().data());
      if (scissor->x < 0 || scissor->y < 0 || scissor->z < 0 ||
          scissor->w < 0) {
        part.finstance->unsetScissor();
      } else {
        part.finstance->setScissor(scissor->x, scissor->y, scissor->z,
                                   scissor->w);
      }
    }
  }
  {
    const MaterialProperty* property =
        GetMaterialProperty(part_name, ConstHash("PolygonOffset"));
    if (property) {
      CHECK(property->type == MaterialPropertyType::Float2);
      const vec2* offset =
          reinterpret_cast<const vec2*>(property->data.GetByteSpan().data());
      part.finstance->setPolygonOffset(offset->x, offset->y);
    }
  }

  FilamentShader* shader = static_cast<FilamentShader*>(part.shader.get());
  shader->ForEachParameter(
      part.variant_id, [&](const FilamentShader::ParameterInfo& param) {
        const MaterialProperty* property =
            GetMaterialProperty(part_name, param.key);
        if (property == nullptr) {
          return;
        }
        if (property->generation == part.property_generations[param.key]) {
          return;
        }

        part.property_generations[param.key] = property->generation;
        if (property->texture) {
          FilamentShader::SetParameter(part.finstance.get(), param.name.c_str(),
                                       property->type, property->texture);
        } else {
          FilamentShader::SetParameter(part.finstance.get(), param.name.c_str(),
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
    for (auto& part : parts_) {
      fscene->addEntity(part.second.fentity);
    }
  }
}

void FilamentRenderable::RemoveFromScene(FilamentRenderScene* scene) const {
  auto fscene = scene->GetFilamentScene();
  if (scenes_.contains(fscene)) {
    for (auto& part : parts_) {
      fscene->remove(part.second.fentity);
    }
    scenes_.erase(fscene);
  }
}

MeshPtr FilamentRenderable::GetMesh() const { return mesh_; }

void FilamentRenderable::SetMesh(MeshPtr mesh) {
  if (mesh_ != mesh) {
    mesh_ = mesh;
    CreateParts();
    RebuildConditions();
  }
}

void FilamentRenderable::CreateParts() {
  DestroyParts();
  if (mesh_ == nullptr) {
    return;
  }

  FilamentMesh* impl = static_cast<FilamentMesh*>(mesh_.get());
  const auto vbuffer = impl->GetFilamentVertexBuffer();
  const auto ibuffer = impl->GetFilamentIndexBuffer();

  const size_t count = mesh_->GetNumSubmeshes();
  for (size_t i = 0; i < count; ++i) {
    const Mesh::SubmeshData& submesh = mesh_->GetSubmeshData(i);
    CHECK(count == 1 || submesh.name.get()) << "Submeshes must have names.";

    // Ensure we always have a material for every part.
    materials_[submesh.name];

    is_skinned_ |= submesh.vertex_format.GetAttributeWithUsage(
                       VertexUsage::BoneWeights) != nullptr;

    // Create a filament Renderable for each part.
    PartInstance& part = parts_[submesh.name];
    filament::RenderableManager::Builder builder(1);
    const auto type = ToFilament(submesh.primitive_type);
    builder.boundingBox(ToFilament(submesh.box));
    builder.geometry(0, type, vbuffer, ibuffer, submesh.range_start,
                     submesh.range_end - submesh.range_start);

    if (is_skinned_) {
      builder.skinning(255);
    }

    part.fentity = utils::EntityManager::get().create();
    builder.build(*fengine_, part.fentity);

    // Add the part to all the scenes to which this belongs.
    for (filament::Scene* scene : scenes_) {
      scene->addEntity(part.fentity);
    }
  }
}

void FilamentRenderable::DestroyParts() {
  auto& tm = fengine_->getTransformManager();
  auto& rm = fengine_->getRenderableManager();
  for (auto& iter : parts_) {
    for (filament::Scene* scene : scenes_) {
      scene->remove(iter.second.fentity);
    }
    rm.destroy(iter.second.fentity);
    tm.destroy(iter.second.fentity);
    utils::EntityManager::get().destroy(iter.second.fentity);
  }
  parts_.clear();
  is_skinned_ = false;
}

void FilamentRenderable::SetShader(ShaderPtr shader,
                                   std::optional<HashValue> part) {
  Material& material = materials_[part ? part.value() : kRootPart];
  material.shader = std::move(shader);
  ReacquireInstance(part ? part.value() : kRootPart);
}

void FilamentRenderable::Show(std::optional<HashValue> part) {
  Material& material = materials_[part ? part.value() : kRootPart];
  material.visible = true;
}

void FilamentRenderable::Hide(std::optional<HashValue> part) {
  Material& material = materials_[part ? part.value() : kRootPart];
  material.visible = false;
}

bool FilamentRenderable::IsHidden(std::optional<HashValue> part) const {
  auto iter = materials_.find(part ? part.value() : kRootPart);
  return iter != materials_.end() ? !iter->second.visible : true;
}

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
  Material& material = materials_[kRootPart];
  MaterialProperty& property = material.properties[key];
  property.data.Clear();
  property.texture = texture;
  property.type = MaterialPropertyTypeFromTextureTarget(texture->GetTarget());

  FilamentTexture* impl = static_cast<FilamentTexture*>(texture.get());
  impl->OnReady([=]() { RebuildConditions(); });
  ++property.generation;
}

TexturePtr FilamentRenderable::GetTexture(TextureUsage usage) const {
  const HashValue key = usage.Hash();
  const MaterialProperty* property = GetMaterialProperty(kRootPart, key);
  return property ? property->texture : nullptr;
}

void FilamentRenderable::SetProperty(HashValue name, MaterialPropertyType type,
                                     absl::Span<const std::byte> data) {
  SetProperty(name, kRootPart, type, data);
}

void FilamentRenderable::SetProperty(HashValue name, HashValue part,
                                     MaterialPropertyType type,
                                     absl::Span<const std::byte> data) {
  Material& material = materials_[part];
  if (type == MaterialPropertyType::Feature) {
    CHECK(data.size() == sizeof(bool));
    const bool enable = *reinterpret_cast<const bool*>(data.data());
    if (enable) {
      material.features.emplace(name);
    } else {
      material.features.erase(name);
    }
    ReacquireInstance(part);
  } else {
    MaterialProperty& property = material.properties[name];
    property.texture = nullptr;
    property.data.Assign(data);
    property.type = type;
    ++property.generation;
  }
}

const FilamentRenderable::MaterialProperty*
FilamentRenderable::GetMaterialProperty(HashValue part, HashValue name) const {
  bool is_root = part == kRootPart;
  auto part_iter = materials_.find(part);
  if (part_iter == materials_.end()) {
    is_root = true;
    part_iter = materials_.find(kRootPart);
  }
  if (part_iter == materials_.end()) {
    return nullptr;
  }

  const Material* material = &part_iter->second;
  auto property_iter = material->properties.find(name);
  if (property_iter == material->properties.end() && !is_root) {
    is_root = true;
    part_iter = materials_.find(kRootPart);
    material = &part_iter->second;
    property_iter = material->properties.find(name);
  }

  return property_iter != material->properties.end() ? &property_iter->second
                                                     : nullptr;
}

void FilamentRenderable::RebuildConditions() {
  conditions_.clear();
  if (mesh_ == nullptr || mesh_->GetNumSubmeshes() == 0) {
    return;
  }

  const VertexFormat& format = mesh_->GetSubmeshData(0).vertex_format;
  for (size_t i = 0; i < format.GetNumAttributes(); ++i) {
    const VertexAttribute* attrib = format.GetAttributeAt(i);
    if (!disabled_vertices_.contains(attrib->usage)) {
      conditions_.insert(Hash(attrib->usage));
    }
  }
  const Material& root_material = materials_[kRootPart];
  for (const auto& iter : root_material.properties) {
    if (iter.second.texture != nullptr) {
      FilamentTexture* impl =
          static_cast<FilamentTexture*>(iter.second.texture.get());
      if (impl->IsReady()) {
        conditions_.insert(iter.first);
      }
    }
  }
  for (const auto& iter : parts_) {
    ReacquireInstance(iter.first);
  }
}

void FilamentRenderable::ReacquireInstance(HashValue part) {
  auto part_iter = parts_.find(part);
  auto material_iter = materials_.find(part);
  if (part_iter == parts_.end() || material_iter == materials_.end()) {
    return;
  }

  Material& part_material = material_iter->second;
  Material& root_material = materials_[kRootPart];

  // Use the root shader if the part does not have its own shader specified.
  ShaderPtr shader = part_material.shader;
  if (shader == nullptr) {
    shader = root_material.shader;
  }

  // Acquire the material instance from the shader.
  if (shader) {
    // Combine the features from the root and part into a single set.
    absl::btree_set<HashValue> features;
    for (auto& f : part_material.features) {
      features.emplace(f);
    }
    for (auto& f : root_material.features) {
      features.emplace(f);
    }

    // Find a shader material instance that fulfills the requirements.
    FilamentShader* impl = static_cast<FilamentShader*>(shader.get());
    const auto variant = impl->DetermineVariantId(conditions_, features);

    // If the updated set of requirements requires a new variant instance,
    // abandon the old one and create a new one.
    PartInstance& part_instance = part_iter->second;
    if (part_instance.variant_id != variant) {
      part_instance.shader = shader;
      part_instance.variant_id = variant;
      part_instance.property_generations.clear();

      const filament::Material* fmaterial = impl->GetFilamentMaterial(variant);
      filament::MaterialInstance* finstance = fmaterial->createInstance();
      part_instance.finstance = MakeFilamentResource(finstance, fengine_);
    }
  }

  // If we're changing the root shader, then we also need to update any parts
  // that don't have their own shader, but instead use the root shader.
  if (part == kRootPart) {
    for (const auto& iter : materials_) {
      if (iter.first != kRootPart && iter.second.shader == nullptr) {
        ReacquireInstance(iter.first);
      }
    }
  }
}

}  // namespace redux
