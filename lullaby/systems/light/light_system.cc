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

#include "lullaby/systems/light/light_system.h"

#include "lullaby/events/render_events.h"
#include "lullaby/systems/render/render_system.h"

namespace lull {
namespace {
HashValue RenderPassNameFromEntityAndLightGroup(Entity entity,
                                                HashValue group) {
  return entity + group;
}

void RemoveLightableFromShadowPass(RenderSystem* render_system, Entity entity,
                                   HashValue pass) {
  // Destroy the entity using the pass hash value as the component identifier.
  render_system->Destroy(entity, pass);
}

bool HasShadows(const PointLightDefT& light) { return false; }
bool HasShadows(const AmbientLightDefT& light) { return false; }
bool HasShadows(const DirectionalLightDefT& light) {
  return light.shadow_def.type() != ShadowDefT::kNone;
}

void AddEmptyShadow(PointLightDefT* light) {}
void AddEmptyShadow(AmbientLightDefT* light) {}
void AddEmptyShadow(DirectionalLightDefT* light) {
  light->shadow_def.template set<ShadowMapDefT>();
}
}  // namespace

static Sqt GetWorldFromEntitySqt(const TransformSystem* transform_system,
                                 Entity entity) {
  const mathfu::mat4* matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  return CalculateSqtFromMatrix(matrix);
}

LightSystem::LightSystem(Registry* registry) : System(registry) {
  RegisterDef(this, ConstHash("AmbientLightDef"));
  RegisterDef(this, ConstHash("DirectionalLightDef"));
  RegisterDef(this, ConstHash("PointLightDef"));
  RegisterDef(this, ConstHash("LightableDef"));
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void LightSystem::SetDepthShader(const ShaderPtr& depth_shader) {
  depth_shader_ = depth_shader;
}

void LightSystem::CreateLight(Entity entity, const AmbientLightDefT& data) {
  groups_[data.group].AddLight(entity, data);
  ambients_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const DirectionalLightDefT& data) {
  LightGroup& group = groups_[data.group];
  directionals_.insert(entity);
  entity_to_group_map_[entity] = data.group;

  auto* transform_system = registry_->Get<lull::TransformSystem>();
  group.AddLight(transform_system, entity, data);

  if (HasShadows(data)) {
    CreateShadowRenderPass(entity, data);

    const HashValue pass =
        RenderPassNameFromEntityAndLightGroup(entity, data.group);
    group.CreateShadowPass(pass);

    auto* render_system = registry_->Get<RenderSystem>();
    auto* dispatcher_system = registry_->Get<lull::DispatcherSystem>();
    const auto& lightables = group.GetLightables();
    for (const auto& lightable : lightables) {
      AddLightableToShadowPass(render_system, dispatcher_system,
                               lightable.first, pass, lightable.second);
    }
  }
}

void LightSystem::CreateLight(Entity entity, const PointLightDefT& data) {
  auto* transform_system = registry_->Get<lull::TransformSystem>();
  groups_[data.group].AddLight(transform_system, entity, data);
  points_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const LightableDefT& data) {
  LightGroup& group = groups_[data.group];
  group.AddLightable(entity, data);
  entity_to_group_map_[entity] = data.group;

  if (data.shadow_interaction == ShadowInteraction_CastAndReceive) {
    auto* render_system = registry_->Get<RenderSystem>();
    auto* dispatcher_system = registry_->Get<lull::DispatcherSystem>();
    const auto& passes = group.GetShadowPasses();
    for (HashValue pass : passes) {
      AddLightableToShadowPass(render_system, dispatcher_system, entity, pass,
                               data);
    }
  }
}

void LightSystem::PostCreateComponent(Entity entity,
                                      const Blueprint& blueprint) {
  if (entity_to_group_map_.count(entity)) {
    LOG(DFATAL) << "Entity already has a light.";
    return;
  }

  if (blueprint.Is<AmbientLightDefT>()) {
    AmbientLightDefT light;
    blueprint.Read(&light);
    CreateLight(entity, light);
  } else if (blueprint.Is<PointLightDefT>()) {
    PointLightDefT light;
    blueprint.Read(&light);
    CreateLight(entity, light);
  } else if (blueprint.Is<DirectionalLightDefT>()) {
    DirectionalLightDefT light;
    blueprint.Read(&light);
    CreateLight(entity, light);
  } else if (blueprint.Is<LightableDefT>()) {
    LightableDefT lightable;
    blueprint.Read(&lightable);
    CreateLight(entity, lightable);
  } else {
    LOG(DFATAL) << "Invalid light type.";
  }
}

void LightSystem::Destroy(Entity entity) {
  auto iter = entity_to_group_map_.find(entity);
  if (iter == entity_to_group_map_.end()) {
    return;
  }

  auto group = groups_.find(iter->second);
  if (group != groups_.end()) {
    group->second.Remove(registry_, entity);
    if (group->second.Empty()) {
      groups_.erase(group);
    }
  }

  entity_to_group_map_.erase(entity);
  ambients_.erase(entity);
  directionals_.erase(entity);
  points_.erase(entity);
}

void LightSystem::AdvanceFrame() {
  auto* transform_system = registry_->Get<lull::TransformSystem>();
  UpdateLightTransforms(transform_system, directionals_);
  UpdateLightTransforms(transform_system, points_);

  auto* render_system = registry_->Get<RenderSystem>();
  for (auto& iter : groups_) {
    iter.second.Update(render_system);
  }
}

void LightSystem::UpdateLightTransforms(
    TransformSystem* transform_system,
    const std::unordered_set<Entity>& entities) {
  for (Entity entity : entities) {
    const HashValue group = entity_to_group_map_[entity];
    DCHECK_NE(group, 0);
    auto iter = groups_.find(group);
    if (iter != groups_.end()) {
      iter->second.UpdateLight(transform_system, entity);
    }
  }
}

void LightSystem::LightGroup::AddLight(Entity entity,
                                       const AmbientLightDefT& light) {
  auto& new_light = ambients_[entity];
  new_light = light;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLight(TransformSystem* transform_system,
                                       Entity entity,
                                       const DirectionalLightDefT& light) {
  const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
  auto& new_light = directionals_[entity];
  new_light = light;
  new_light.rotation = sqt.rotation;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLight(TransformSystem* transform_system,
                                       Entity entity,
                                       const PointLightDefT& light) {
  const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
  auto& new_light = points_[entity];
  new_light = light;
  new_light.position = sqt.translation;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLightable(Entity entity,
                                           const LightableDefT& lightable) {
  auto& new_lightable = lightables_[entity];
  new_lightable = lightable;
  dirty_lightables_.insert(entity);
}

void LightSystem::LightGroup::Remove(Registry* registry, Entity entity) {
  if (lightables_.erase(entity)) {
    auto* render_system = registry->Get<RenderSystem>();
    for (const auto& pass : shadow_passes_) {
      RemoveLightableFromShadowPass(render_system, entity, pass);
    }
  }

  if (ambients_.erase(entity)) {
    dirty_ = true;
  }

  auto directionals_iterator = directionals_.find(entity);
  if (directionals_iterator != directionals_.end()) {
    dirty_ = true;

    auto* render_system = registry->Get<RenderSystem>();
    DestroyShadowPass(render_system,
                      RenderPassNameFromEntityAndLightGroup(
                          entity, directionals_iterator->second.group));
    directionals_.erase(directionals_iterator);
  }
  if (points_.erase(entity)) {
    dirty_ = true;
  }
}

void LightSystem::LightGroup::Update(RenderSystem* render_system) {
  if (dirty_) {
    for (auto& lightable : lightables_) {
      UpdateLightable(render_system, lightable.first, lightable.second);
    }
    dirty_ = false;
  } else {
    for (Entity dirty_lightable : dirty_lightables_) {
      auto lightable = lightables_.find(dirty_lightable);
      if (lightable != lightables_.end()) {
        UpdateLightable(render_system, lightable->first, lightable->second);
      }
    }
  }
  dirty_lightables_.clear();
}

const std::vector<HashValue>& LightSystem::LightGroup::GetShadowPasses() const {
  return shadow_passes_;
}

const std::unordered_map<Entity, LightableDefT>&
LightSystem::LightGroup::GetLightables() const {
  return lightables_;
}

template <typename T>
void LightSystem::UpdateUniforms(UniformData* uniforms, const T& light,
                                 int max_allowed, int* current_index) {
  DCHECK(current_index);

  if (*current_index >= max_allowed) {
#ifndef _DEBUG
    LOG(WARNING) << "Entity has a maximum of " << max_allowed << " "
                 << GetTypeName<T>() << " lights, however there are currently"
                 << *current_index + 1 << " defined lights.";
#endif
    return;
  }
  uniforms->Add(light);
  ++(*current_index);
}

template <typename T>
void LightSystem::UpdateUniforms(UniformData* uniforms,
                                 const std::unordered_map<Entity, T>& lights,
                                 int max_allowed, int max_shadows) {
  int count = 0;
  int shadow_count = 0;
  for (const auto& light : lights) {
    if (HasShadows(light.second)) {
      UpdateUniforms(uniforms, light.second, max_shadows, &shadow_count);
    } else {
      UpdateUniforms(uniforms, light.second, max_allowed, &count);
    }
  }
  while (count < max_allowed) {
    const T blacklight;
    uniforms->Add(blacklight);
    ++count;
  }
  while (shadow_count < max_shadows) {
    T blacklight;
    AddEmptyShadow(&blacklight);
    uniforms->Add(blacklight);
    ++count;
  }
}

void LightSystem::LightGroup::UpdateLightable(RenderSystem* render_system,
                                              Entity entity,
                                              const LightableDefT& data) {
  UniformData uniforms;
  UpdateUniforms(&uniforms, ambients_, data.max_ambient_lights, 0);
  UpdateUniforms(
      &uniforms, directionals_, data.max_directional_lights,
      (data.shadow_interaction == ShadowInteraction_CastAndReceive) ? 1 : 0);
  UpdateUniforms(&uniforms, points_, data.max_point_lights, 0);
  uniforms.Apply(render_system, entity);
}

void LightSystem::LightGroup::UpdateLight(TransformSystem* transform_system,
                                          Entity entity) {
  auto directional_iter = directionals_.find(entity);
  if (directional_iter != directionals_.end()) {
    auto& light = directional_iter->second;
    const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
    if (light.rotation.scalar() != sqt.rotation.scalar() ||
        light.rotation.vector() != sqt.rotation.vector()) {
      light.rotation = sqt.rotation;
      dirty_ = true;
    }
  }

  auto point_iter = points_.find(entity);
  if (point_iter != points_.end()) {
    auto& light = point_iter->second;
    const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
    if (light.position != sqt.translation) {
      light.position = sqt.translation;
      dirty_ = true;
    }
  }
}

bool LightSystem::LightGroup::Empty() const {
  return ambients_.empty() && directionals_.empty() && points_.empty() &&
         lightables_.empty();
}

void LightSystem::LightGroup::CreateShadowPass(HashValue pass) {
  DCHECK(std::find(shadow_passes_.cbegin(), shadow_passes_.cend(), pass) ==
         shadow_passes_.cend());
  shadow_passes_.push_back(pass);
}

void LightSystem::LightGroup::DestroyShadowPass(RenderSystem* render_system,
                                                HashValue pass) {
  auto iter = std::find(shadow_passes_.cbegin(), shadow_passes_.cend(), pass);
  if (iter == shadow_passes_.cend()) {
    return;
  }

  for (auto& lightable : lightables_) {
    RemoveLightableFromShadowPass(render_system, lightable.first, pass);
  }
  shadow_passes_.erase(iter);
}

void LightSystem::UniformData::Clear() { buffers.clear(); }

void LightSystem::UniformData::Add(const AmbientLightDefT& light) {
  auto& colors = buffers["light_ambient_color"];
  colors.dimension = 3;

  const mathfu::vec4 data = Color4ub::ToVec4(light.color);
  colors.data.push_back(data.x);
  colors.data.push_back(data.y);
  colors.data.push_back(data.z);
}

void LightSystem::UniformData::Add(const DirectionalLightDefT& light) {
  static constexpr char kColorUniformName[] = "light_directional_color";
  static constexpr char kDirectionUniformName[] = "light_directional_dir";
  static constexpr char kExponentUniformName[] = "light_directional_exponent";

  static constexpr char kShadowColorUniformName[] =
      "light_directional_shadow_color";
  static constexpr char kShadowDirectionUniformName[] =
      "light_directional_shadow_dir";
  static constexpr char kShadowExponentUniformName[] =
      "light_directional_shadow_exponent";

  const bool has_shadow = HasShadows(light);
  auto& colors =
      buffers[has_shadow ? kShadowColorUniformName : kColorUniformName];
  colors.dimension = 3;

  const mathfu::vec4 data = Color4ub::ToVec4(light.color);
  colors.data.push_back(data.x);
  colors.data.push_back(data.y);
  colors.data.push_back(data.z);

  auto& directions =
      buffers[has_shadow ? kShadowDirectionUniformName : kDirectionUniformName];
  directions.dimension = 3;

  const mathfu::vec3 light_dir = light.rotation * -mathfu::kAxisZ3f;
  directions.data.push_back(light_dir.x);
  directions.data.push_back(light_dir.y);
  directions.data.push_back(light_dir.z);

  auto& exponents =
      buffers[has_shadow ? kShadowExponentUniformName : kExponentUniformName];
  exponents.dimension = 1;
  exponents.data.push_back(light.exponent);
}

void LightSystem::UniformData::Add(const PointLightDefT& light) {
  auto& colors = buffers["light_point_color"];
  colors.dimension = 3;

  const mathfu::vec4 data = Color4ub::ToVec4(light.color);
  colors.data.push_back(data.x);
  colors.data.push_back(data.y);
  colors.data.push_back(data.z);

  auto& positions = buffers["light_point_pos"];
  positions.dimension = 3;
  positions.data.push_back(light.position.x);
  positions.data.push_back(light.position.y);
  positions.data.push_back(light.position.z);

  auto& exponents = buffers["light_point_exponent"];
  exponents.dimension = 1;
  exponents.data.push_back(light.exponent);

  auto& intensities = buffers["light_point_intensity"];
  intensities.dimension = 1;
  intensities.data.push_back(light.intensity);
}

void LightSystem::UniformData::Apply(RenderSystem* render_system,
                                     Entity entity) const {
  for (const auto& it : buffers) {
    const auto& buffer = it.second;
    const int count = static_cast<int>(buffer.data.size()) / buffer.dimension;
    render_system->SetUniform(entity, it.first.c_str(), buffer.data.data(),
                              buffer.dimension, count);
  }
}

void LightSystem::RenderShadowMaps() {
  auto* render_system = registry_->Get<RenderSystem>();

  for (const auto& shadow_pass : shadow_passes_) {
    render_system->Render(&shadow_pass.view, 1,
                          static_cast<RenderPass>(shadow_pass.pass));
  }
}

void LightSystem::AddLightableToShadowPass(RenderSystem* render_system,
                                           DispatcherSystem* dispatcher_system,
                                           Entity entity, HashValue pass,
                                           const LightableDefT& lightable) {
  if (!depth_shader_) {
    LOG(FATAL) << "Must have a depth shader to use shadows.";
    return;
  }

  render_system->Create(entity, pass, static_cast<RenderPass>(pass));
  // Retrieve the mesh of the default entity.
  // TODO(b/65262474): Currently there is no GetMesh function to return the mesh
  // of the default entity so we cheat by supplying component id of 0.
  render_system->SetMesh(entity, pass, render_system->GetMesh(entity, 0));
  render_system->SetShader(entity, pass, depth_shader_);
  render_system->SetTexture(entity, 0, lightable.shadow_sampler,
                            render_system->GetTexture(pass));

  static constexpr char kLightMatrixUniformName[] =
      "directional_light_shadow_matrix";
  for (const auto& shadow_pass : shadow_passes_) {
    if (shadow_pass.pass == pass) {
      render_system->SetUniform(entity, kLightMatrixUniformName,
                                &shadow_pass.view.clip_from_world_matrix[0],
                                16 /*=dimensions*/, 1 /*=count*/);
    }
  }

  dispatcher_system->Connect(
      entity, this,
      [render_system, entity, pass](const MeshChangedEvent& event) {
        if (event.component_id == 0) {
          render_system->SetMesh(entity, pass,
                                 render_system->GetMesh(entity, 0));
        }
      });
}

void LightSystem::CreateShadowRenderPass(Entity entity,
                                         const DirectionalLightDefT& data) {
  const ShadowMapDefT* shadow_def = data.shadow_def.get<ShadowMapDefT>();
  if (!shadow_def) {
    return;
  }

  // Create the render target.
  const HashValue pass =
      RenderPassNameFromEntityAndLightGroup(entity, data.group);
  const mathfu::vec2i shadow_map_resolution(shadow_def->shadow_resolution,
                                            shadow_def->shadow_resolution);
  auto* render_system = registry_->Get<RenderSystem>();
  render_system->CreateRenderTarget(pass, shadow_map_resolution,
                                    TextureFormat_R8,
                                    DepthStencilFormat_Depth16);

  // Set the render target for the pass.
  render_system->SetRenderTarget(pass, pass);

  // Create the viewport for rendering the shadow pass.
  ShadowPassData shadow_pass_data;
  shadow_pass_data.pass = pass;
  shadow_pass_data.view.viewport.x = 0;
  shadow_pass_data.view.viewport.y = 0;
  shadow_pass_data.view.dimensions = shadow_map_resolution;

  // Construct the view and projection matrices.
  auto* transform_system = registry_->Get<lull::TransformSystem>();
  const mathfu::mat4* entity_world_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (entity_world_matrix) {
    shadow_pass_data.view.world_from_eye_matrix = *entity_world_matrix;
  } else {
    shadow_pass_data.view.world_from_eye_matrix = mathfu::mat4::Identity();
    LOG(ERROR) << "Directional light entity " << entity
               << " lacks a transform component.";
  }
  const float half_shadow_volume = shadow_def->shadow_volume * 0.5f;
  shadow_pass_data.view.clip_from_eye_matrix = mathfu::mat4::Ortho(
      -half_shadow_volume, half_shadow_volume, -half_shadow_volume,
      half_shadow_volume, shadow_def->shadow_min_distance,
      shadow_def->shadow_max_distance, 1.0f);
  shadow_pass_data.view.clip_from_world_matrix =
      shadow_pass_data.view.clip_from_eye_matrix *
      shadow_pass_data.view.world_from_eye_matrix.Inverse();

  // Add the shadow pass.
  shadow_passes_.push_back(std::move(shadow_pass_data));
}

}  // namespace lull
