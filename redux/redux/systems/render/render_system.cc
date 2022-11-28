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

#include <utility>

#include "redux/engines/render/mesh_factory.h"
#include "redux/engines/render/shader_factory.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/choreographer.h"
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

  static int counter = 0;
  ++counter;

  for (const auto& iter : renderables_) {
    const auto& shader_indices = iter.second.shader_indices;
    const auto& inverse_bind_pose = iter.second.inverse_bind_pose;
    if (!shader_indices.empty() || !inverse_bind_pose.empty()) {
      absl::Span<const mat4> pose = rig_system->GetPose(iter.first);
      const std::vector<mat4> bones =
          TransformBones(pose, inverse_bind_pose, shader_indices);
      const std::byte* bytes = reinterpret_cast<const std::byte*>(bones.data());
      const size_t num_bytes = sizeof(mat4) * bones.size();
      iter.second.renderable->SetProperty(ConstHash("Bones"),
                                          MaterialPropertyType::Float4x4,
                                          {bytes, num_bytes});
    }

    const Transform transform = transform_system->GetTransform(iter.first);
    const mat4 matrix = TransformMatrix(transform);
    iter.second.renderable->PrepareToRender(matrix);
  }
}

void RenderSystem::SetFromRenderDef(Entity entity, const RenderDef& def) {
  if (!def.shading_model.empty()) {
    SetShadingModel(entity, def.shading_model);
  }
}

void RenderSystem::OnDestroy(Entity entity) { renderables_.erase(entity); }

void RenderSystem::AddToScene(Entity entity, HashValue scene) {
  RenderScenePtr scene_ptr = engine_->GetRenderScene(scene);
  if (scene_ptr) {
    Renderable& renderable = GetRenderable(entity, scene);
    scene_ptr->Add(renderable);
  }
}

void RenderSystem::RemoveFromScene(Entity entity, HashValue scene) {
  auto iter = renderables_.find(entity);
  if (iter == renderables_.end()) {
    return;
  }

  RenderableComponent* c = &iter->second;
  CHECK(c->renderable);

  auto scene_ptr = engine_->GetRenderScene(scene);
  CHECK(scene_ptr);
  scene_ptr->Remove(*c->renderable.get());
}

bool RenderSystem::IsReadyToRender(Entity entity) const {
  CHECK(false);
  return false;
}

void RenderSystem::Hide(Entity entity) {
  Renderable* r = TryGetRenderable(entity);
  if (r) {
    r->Hide();
  }
}

void RenderSystem::Show(Entity entity) {
  Renderable* r = TryGetRenderable(entity);
  if (r) {
    r->Show();
  }
}

bool RenderSystem::IsHidden(Entity entity) const {
  const Renderable* r = TryGetRenderable(entity);
  return r ? r->IsHidden() : true;
}

void RenderSystem::Hide(Entity entity, HashValue part) {
  Renderable* r = TryGetRenderable(entity);
  if (r) {
    r->Hide(part);
  }
}

void RenderSystem::Show(Entity entity, HashValue part) {
  Renderable* r = TryGetRenderable(entity);
  if (r) {
    r->Show(part);
  }
}

bool RenderSystem::IsHidden(Entity entity, HashValue part) const {
  const Renderable* r = TryGetRenderable(entity);
  return r ? r->IsHidden(part) : true;
}

void RenderSystem::SetMesh(Entity entity, MeshData mesh) {
  SetMesh(entity, engine_->GetMeshFactory()->CreateMesh(std::move(mesh)));
}

void RenderSystem::SetMesh(Entity entity, const MeshPtr& mesh) {
  Renderable& r = GetRenderable(entity);
  if (mesh != r.GetMesh()) {
    r.SetMesh(mesh);
    // OnMeshUpdated();
  }
}

MeshPtr RenderSystem::GetMesh(Entity entity) const {
  const Renderable* r = TryGetRenderable(entity);
  return r ? r->GetMesh() : nullptr;
}

void RenderSystem::SetTexture(Entity entity, TextureUsage usage,
                              const TexturePtr& texture) {
  Renderable& renderable = GetRenderable(entity);
  renderable.SetTexture(usage, texture);
}

TexturePtr RenderSystem::GetTexture(Entity entity, TextureUsage usage) const {
  const Renderable* renderable = TryGetRenderable(entity);
  return renderable ? renderable->GetTexture(usage) : nullptr;
}

void RenderSystem::SetShadingModel(Entity entity, std::string_view model) {
  ShaderPtr shader = engine_->GetShaderFactory()->CreateShader(model);
  CHECK(shader);

  Renderable& r = GetRenderable(entity);
  r.SetShader(shader);
}

void RenderSystem::SetShadingModel(Entity entity, HashValue part,
                                   std::string_view model) {
  ShaderPtr shader = engine_->GetShaderFactory()->CreateShader(model);
  CHECK(shader);

  Renderable& r = GetRenderable(entity);
  r.SetShader(shader, part);
}

void RenderSystem::SetInverseBindPose(Entity entity,
                                      absl::Span<const mat4> pose) {
  renderables_[entity].inverse_bind_pose.assign(pose.data(),
                                                pose.data() + pose.size());
}

void RenderSystem::SetBoneShaderIndices(Entity entity,
                                        absl::Span<const uint16_t> indices) {
  renderables_[entity].shader_indices.assign(indices.data(),
                                             indices.data() + indices.size());
}

void RenderSystem::EnableShadingFeature(Entity entity, HashValue feature) {
  const bool value = true;
  const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
  SetMaterialProperty(entity, feature, MaterialPropertyType::Feature,
                      {bytes, sizeof(value)});
}

void RenderSystem::DisableShadingFeature(Entity entity, HashValue feature) {
  const bool value = false;
  const std::byte* bytes = reinterpret_cast<const std::byte*>(&value);
  SetMaterialProperty(entity, feature, MaterialPropertyType::Feature,
                      {bytes, sizeof(value)});
}

void RenderSystem::SetMaterialProperty(Entity entity, HashValue name,
                                       MaterialPropertyType type,
                                       absl::Span<const std::byte> data) {
  Renderable& r = GetRenderable(entity);
  r.SetProperty(name, type, data);
}

Renderable& RenderSystem::GetRenderable(Entity entity,
                                        std::optional<HashValue> scene_name) {
  auto iter = renderables_.find(entity);
  if (iter != renderables_.end()) {
    const RenderablePtr& renderable = iter->second.renderable;
    CHECK(renderable);
    return *renderable;
  }

  RenderableComponent& c = renderables_[entity];
  c.renderable = engine_->CreateRenderable();
  CHECK(c.renderable);

  RenderScenePtr scene;
  if (scene_name) {
    if (scene_name.has_value()) {
      scene = engine_->GetRenderScene(scene_name.value());
      CHECK(scene);
    }
  } else {
    scene = engine_->GetDefaultRenderScene();
    CHECK(scene);
  }
  if (scene) {
    scene->Add(*c.renderable);
  }

  return *c.renderable;
}

Renderable* RenderSystem::TryGetRenderable(Entity entity) {
  auto iter = renderables_.find(entity);
  return iter != renderables_.end() ? iter->second.renderable.get() : nullptr;
}

const Renderable* RenderSystem::TryGetRenderable(Entity entity) const {
  auto iter = renderables_.find(entity);
  return iter != renderables_.end() ? iter->second.renderable.get() : nullptr;
}

}  // namespace redux
