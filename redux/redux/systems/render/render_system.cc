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

#include "redux/systems/render/render_system.h"

#include <stdint.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "redux/engines/render/mesh.h"
#include "redux/engines/render/mesh_factory.h"
#include "redux/engines/render/render_engine.h"
#include "redux/engines/render/render_scene.h"
#include "redux/engines/render/renderable.h"
#include "redux/engines/render/shader.h"
#include "redux/engines/render/shader_factory.h"
#include "redux/engines/render/texture.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/transform.h"
#include "redux/modules/math/vector.h"
#include "redux/systems/render/render_def_generated.h"
#include "redux/systems/rig/rig_system.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

RenderSystem::RenderSystem(Registry* registry) : System(registry) {
  RegisterDef(&RenderSystem::SetFromRenderDef);
  RegisterDependency<TransformSystem>(this);
  registry->RegisterDependency<RenderEngine>(this, true);
}

void RenderSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<RenderEngine>();
  CHECK(engine_);

  auto choreo = registry_->Get<Choreographer>();
  choreo->Add<&RenderSystem::PrepareToRender>(Choreographer::Stage::kRender)
      .Before<&RenderEngine::Render>();
}

static std::vector<mat4> TransformBones(
    absl::Span<const mat4> bones, absl::Span<const mat4> inverse_bind_pose,
    absl::Span<const uint16_t> shader_indices) {
  CHECK(bones.size() == inverse_bind_pose.size() || inverse_bind_pose.empty());
  CHECK_GE(bones.size(), shader_indices.size());

  std::vector<mat4> final_pose;
  final_pose.reserve(bones.size());

  if (shader_indices.empty()) {
    CHECK(!inverse_bind_pose.empty());
    for (size_t i = 0; i < bones.size(); ++i) {
      const mat4& transform = bones[i];
      const mat4& inverse = inverse_bind_pose[i];
      final_pose.push_back(transform * inverse);
    }
  } else if (inverse_bind_pose.empty()) {
    CHECK(!shader_indices.empty());
    for (size_t i = 0; i < shader_indices.size(); ++i) {
      const uint16_t bone_index = shader_indices[i];
      CHECK(bone_index < bones.size());
      final_pose.push_back(bones[bone_index]);
    }
  } else {
    for (size_t i = 0; i < shader_indices.size(); ++i) {
      const uint16_t bone_index = shader_indices[i];
      CHECK(bone_index < bones.size());
      const mat4& transform = bones[bone_index];
      const mat4& inverse = inverse_bind_pose[bone_index];
      final_pose.push_back(transform * inverse);
    }
  }
  return final_pose;
}

void RenderSystem::PrepareToRender() {
  auto rig_system = registry_->Get<RigSystem>();
  auto transform_system = registry_->Get<TransformSystem>();

  for (const auto& iter : renderables_) {
    std::vector<mat4> bones;

    const auto& shader_indices = iter.second.shader_indices;
    const auto& inverse_bind_pose = iter.second.inverse_bind_pose;
    if (!shader_indices.empty() || !inverse_bind_pose.empty()) {
      absl::Span<const mat4> pose = rig_system->GetPose(iter.first);
      bones = TransformBones(pose, inverse_bind_pose, shader_indices);
    }

    const Transform transform = transform_system->GetTransform(iter.first);
    const mat4 matrix = TransformMatrix(transform);
    for (const auto& iter2 : iter.second.renderable_parts) {
      iter2.second->PrepareToRender(matrix);
      if (!bones.empty()) {
        const std::byte* bytes =
            reinterpret_cast<const std::byte*>(bones.data());
        const size_t num_bytes = sizeof(mat4) * bones.size();
        iter2.second->SetProperty(ConstHash("Bones"),
                                  MaterialPropertyType::Float4x4,
                                  {bytes, num_bytes});
      }
    }
  }
}

void RenderSystem::SetFromRenderDef(Entity entity, const RenderDef& def) {
  if (!def.shading_model.empty()) {
    SetShadingModel(entity, def.shading_model);
  }

  auto* texture_factory = engine_->GetTextureFactory();
  for (const auto& texture_def : def.textures) {
    TextureUsage usage({MaterialTextureType::BaseColor});
    if (!texture_def.usage.empty()) {
      usage = TextureUsage(texture_def.usage);
    }
    auto texture = texture_factory->LoadTexture(texture_def.uri, {});
    SetTexture(entity, usage, texture);
  }

  for (const auto& property_def : def.properties) {
    const HashValue key = Hash(property_def.name);
    if (const float* value = property_def.value.Get<float>()) {
      SetMaterialProperty(entity, key, *value);
    } else if (const vec2* value = property_def.value.Get<vec2>()) {
      SetMaterialProperty(entity, key, *value);
    } else if (const vec3* value = property_def.value.Get<vec3>()) {
      SetMaterialProperty(entity, key, *value);
    } else if (const vec4* value = property_def.value.Get<vec4>()) {
      SetMaterialProperty(entity, key, *value);
    } else {
      LOG(FATAL) << "Unknown type for property: " << property_def.name;
    }
  }
}

void RenderSystem::OnDestroy(Entity entity) { renderables_.erase(entity); }

void RenderSystem::AddToScene(Entity entity, HashValue scene) {
  auto iter = renderables_.find(entity);
  if (iter == renderables_.end()) {
    return;
  }

  if (!iter->second.scenes.contains(scene)) {
    iter->second.scenes.insert(scene);
    RenderScenePtr scene_ptr = engine_->GetRenderScene(scene);
    CHECK(scene_ptr != nullptr);
    for (auto& r : iter->second.renderable_parts) {
      scene_ptr->Add(*r.second);
    }
  }
}

void RenderSystem::RemoveFromScene(Entity entity, HashValue scene) {
  auto iter = renderables_.find(entity);
  if (iter == renderables_.end()) {
    return;
  }

  if (iter->second.scenes.contains(scene)) {
    iter->second.scenes.erase(scene);
    RenderScenePtr scene_ptr = engine_->GetRenderScene(scene);
    CHECK(scene_ptr != nullptr);
    for (auto& r : iter->second.renderable_parts) {
      scene_ptr->Remove(*r.second);
    }
  }
}

void RenderSystem::Hide(Entity entity) {
  ApplyToRenderables(entity, std::nullopt, [](Renderable& r) { r.Hide(); });
}

void RenderSystem::Show(Entity entity) {
  ApplyToRenderables(entity, std::nullopt, [](Renderable& r) { r.Show(); });
}

bool RenderSystem::IsHidden(Entity entity) const {
  bool hidden = true;
  ApplyToRenderables(entity, std::nullopt,
                     [&](Renderable& r) { hidden &= r.IsHidden(); });
  return hidden;
}

void RenderSystem::Hide(Entity entity, HashValue part) {
  ApplyToRenderables(entity, part, [](Renderable& r) { r.Hide(); });
}

void RenderSystem::Show(Entity entity, HashValue part) {
  ApplyToRenderables(entity, part, [](Renderable& r) { r.Show(); });
}

bool RenderSystem::IsHidden(Entity entity, HashValue part) const {
  bool hidden = true;
  ApplyToRenderables(entity, part,
                     [&](Renderable& r) { hidden &= r.IsHidden(); });
  return hidden;
}

void RenderSystem::SetMesh(Entity entity, MeshData mesh) {
  SetMesh(entity, engine_->GetMeshFactory()->CreateMesh(std::move(mesh)));
}

void RenderSystem::SetMesh(Entity entity, const MeshPtr& mesh) {
  EnsureRenderable(entity);
  RenderableComponent& rc = renderables_[entity];

  for (size_t i = 0; i < mesh->GetNumParts(); ++i) {
    const HashValue name = mesh->GetPartName(i);
    EnsureRenderable(entity, name);
    rc.renderable_parts[name]->SetMesh(mesh, i);
  }
  rc.mesh = mesh;
}

MeshPtr RenderSystem::GetMesh(Entity entity) const {
  auto iter = renderables_.find(entity);
  return iter != renderables_.end() ? iter->second.mesh : nullptr;
}

void RenderSystem::SetTexture(Entity entity, TextureUsage usage,
                              const TexturePtr& texture) {
  EnsureRenderable(entity);
  ApplyToRenderables(entity, std::nullopt,
                     [&](Renderable& r) { r.SetTexture(usage, texture); });
}

void RenderSystem::SetTexture(Entity entity, HashValue part, TextureUsage usage,
                              const TexturePtr& texture) {
  EnsureRenderable(entity, part);
  ApplyToRenderables(entity, part,
                     [&](Renderable& r) { r.SetTexture(usage, texture); });
}

TexturePtr RenderSystem::GetTexture(Entity entity, TextureUsage usage) const {
  std::vector<TexturePtr> textures;
  ApplyToRenderables(entity, std::nullopt, [&](Renderable& r) {
    auto texture = r.GetTexture(usage);
    if (texture) {
      textures.emplace_back(std::move(texture));
    }
  });
  CHECK_LE(textures.size(), 1);
  return textures.empty() ? nullptr : textures.front();
}

TexturePtr RenderSystem::GetTexture(Entity entity, HashValue part,
                                    TextureUsage usage) const {
  std::vector<TexturePtr> textures;
  ApplyToRenderables(entity, part, [&](Renderable& r) {
    auto texture = r.GetTexture(usage);
    if (texture) {
      textures.emplace_back(std::move(texture));
    }
  });
  CHECK_LE(textures.size(), 1);
  return textures.empty() ? nullptr : textures.front();
}

void RenderSystem::SetShadingModel(Entity entity, std::string_view model) {
  ShaderPtr shader = engine_->GetShaderFactory()->CreateShader(model);
  CHECK(shader);

  EnsureRenderable(entity);
  ApplyToRenderables(entity, std::nullopt,
                     [&](Renderable& r) { r.SetShader(shader); });
}

void RenderSystem::SetShadingModel(Entity entity, HashValue part,
                                   std::string_view model) {
  ShaderPtr shader = engine_->GetShaderFactory()->CreateShader(model);
  CHECK(shader);

  EnsureRenderable(entity, part);
  ApplyToRenderables(entity, part, [&](Renderable& r) { r.SetShader(shader); });
}

void RenderSystem::SetInverseBindPose(Entity entity,
                                      absl::Span<const mat4> pose) {
  EnsureRenderable(entity);
  renderables_[entity].inverse_bind_pose.assign(pose.data(),
                                                pose.data() + pose.size());
}

void RenderSystem::SetBoneShaderIndices(Entity entity,
                                        absl::Span<const uint16_t> indices) {
  EnsureRenderable(entity);
  renderables_[entity].shader_indices.assign(indices.data(),
                                             indices.data() + indices.size());
}

void RenderSystem::EnableShadingFeature(Entity entity, HashValue feature) {
  const bool value = true;
  const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
  SetMaterialProperty(entity, feature, MaterialPropertyType::Feature,
                      {bytes, sizeof(value)});
}

void RenderSystem::EnableShadingFeature(Entity entity, HashValue part,
                                        HashValue feature) {
  const bool value = true;
  const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
  SetMaterialProperty(entity, part, feature, MaterialPropertyType::Feature,
                      {bytes, sizeof(value)});
}

void RenderSystem::DisableShadingFeature(Entity entity, HashValue feature) {
  const bool value = false;
  const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
  SetMaterialProperty(entity, feature, MaterialPropertyType::Feature,
                      {bytes, sizeof(value)});
}

void RenderSystem::DisableShadingFeature(Entity entity, HashValue part,
                                         HashValue feature) {
  const bool value = false;
  const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
  SetMaterialProperty(entity, part, feature, MaterialPropertyType::Feature,
                      {bytes, sizeof(value)});
}

void RenderSystem::SetMaterialProperty(Entity entity, HashValue name,
                                       MaterialPropertyType type,
                                       absl::Span<const std::byte> data) {
  EnsureRenderable(entity);
  ApplyToRenderables(entity, std::nullopt,
                     [&](Renderable& r) { r.SetProperty(name, type, data); });
}

void RenderSystem::SetMaterialProperty(Entity entity, HashValue part,
                                       HashValue name,
                                       MaterialPropertyType type,
                                       absl::Span<const std::byte> data) {
  EnsureRenderable(entity, part);
  ApplyToRenderables(entity, part,
                     [&](Renderable& r) { r.SetProperty(name, type, data); });
}

void RenderSystem::EnsureRenderable(Entity entity,
                                    std::optional<HashValue> part) {
  RenderableComponent& rc = renderables_[entity];
  if (rc.global_renderable == nullptr) {
    rc.global_renderable = engine_->CreateRenderable();
    rc.scenes.insert(engine_->GetDefaultRenderSceneName());
  }

  if (part.has_value()) {
    auto& render_part = rc.renderable_parts[part.value()];
    if (render_part == nullptr) {
      render_part = engine_->CreateRenderable();
      render_part->InheritProperties(*rc.global_renderable);

      for (const HashValue scene : rc.scenes) {
        RenderScenePtr scene_ptr = engine_->GetRenderScene(scene);
        CHECK(scene_ptr != nullptr);
        scene_ptr->Add(*render_part);
      }
    }
  }
}

template <typename Fn>
void RenderSystem::ApplyToRenderables(Entity entity,
                                      std::optional<HashValue> part, Fn fn) {
  auto iter = renderables_.find(entity);
  if (iter == renderables_.end()) {
    return;
  }

  RenderableComponent& rc = iter->second;
  if (part.has_value()) {
    auto part_iter = rc.renderable_parts.find(part.value());
    if (part_iter != rc.renderable_parts.end()) {
      fn(*part_iter->second);
    }
  } else {
    fn(*rc.global_renderable);
    for (auto& parts : rc.renderable_parts) {
      fn(*parts.second);
    }
  }
}

template <typename Fn>
void RenderSystem::ApplyToRenderables(Entity entity,
                                      std::optional<HashValue> part,
                                      Fn fn) const {
  auto iter = renderables_.find(entity);
  if (iter == renderables_.end()) {
    return;
  }

  const RenderableComponent& rc = iter->second;
  if (part.has_value()) {
    auto part_iter = rc.renderable_parts.find(part.value());
    if (part_iter != rc.renderable_parts.end()) {
      fn(*part_iter->second);
    }
  } else {
    fn(*rc.global_renderable);
    for (auto& parts : rc.renderable_parts) {
      fn(*parts.second);
    }
  }
}

}  // namespace redux
