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

#include "lullaby/systems/light/light_system.h"

#include "lullaby/events/render_events.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/texture_factory.h"
#include "mathfu/constants.h"
#include "mathfu/utilities.h"

namespace lull {
namespace {
static constexpr char kLightMatrixUniformName[] =
    "directional_light_shadow_matrix";
static constexpr char kColorUniformName[] = "light_directional_color";
static constexpr char kDirectionUniformName[] = "light_directional_dir";
static constexpr char kExponentUniformName[] = "light_directional_exponent";

static constexpr char kShadowColorUniformName[] =
    "light_directional_shadow_color";
static constexpr char kShadowDirectionUniformName[] =
    "light_directional_shadow_dir";
static constexpr char kShadowExponentUniformName[] =
    "light_directional_shadow_exponent";

HashValue RenderPassNameFromEntityAndLightGroup(Entity entity,
                                                HashValue group) {
  return entity.AsUint32() + group;
}

void RemoveLightableFromShadowPass(RenderSystem* render_system, Entity entity,
                                   HashValue pass) {
  // Destroy the entity using the pass hash value as the component identifier.
  render_system->Destroy(entity, pass);
}

bool HasShadows(const SpotLightDefT& light) { return false; }
bool HasShadows(const PointLightDefT& light) { return false; }
bool HasShadows(const AmbientLightDefT& light) { return false; }
bool HasShadows(const DirectionalLightDefT& light) {
  return light.shadow_def.type() != ShadowDefT::kNone;
}

void AddEmptyShadow(SpotLightDefT* light) {}
void AddEmptyShadow(PointLightDefT* light) {}
void AddEmptyShadow(AmbientLightDefT* light) {}
void AddEmptyShadow(DirectionalLightDefT* light) {
  light->shadow_def.template set<ShadowMapDefT>();
}

void UpdateRenderViewTransform(const TransformSystem* transform_system,
                               Entity entity, RenderView* render_view) {
  if (!transform_system || !render_view || entity == kNullEntity) {
    return;
  }

  const mathfu::mat4* entity_world_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (entity_world_matrix) {
    render_view->world_from_eye_matrix = *entity_world_matrix;
  } else {
    render_view->world_from_eye_matrix = mathfu::mat4::Identity();
    LOG(ERROR) << "Directional light entity " << entity
               << " lacks a transform component.";
  }

  render_view->clip_from_world_matrix =
      render_view->clip_from_eye_matrix *
      render_view->world_from_eye_matrix.Inverse();
}
}  // namespace

static Sqt GetWorldFromEntitySqt(const TransformSystem* transform_system,
                                 Entity entity) {
  const mathfu::mat4* matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  return CalculateSqtFromMatrix(matrix);
}

LightSystem::LightSystem(Registry* registry) : System(registry) {
  RegisterDef<AmbientLightDefT>(this);
  RegisterDef<DirectionalLightDefT>(this);
  RegisterDef<EnvironmentLightDefT>(this);
  RegisterDef<PointLightDefT>(this);
  RegisterDef<SpotLightDefT>(this);
  RegisterDef<LightableDefT>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
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

  group.AddLight(registry_, entity, data);
}

void LightSystem::CreateLight(Entity entity, const EnvironmentLightDefT& data) {
  LightGroup& group = groups_[data.group];
  group.AddLight(registry_, entity, data);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const PointLightDefT& data) {
  auto* transform_system = registry_->Get<lull::TransformSystem>();
  groups_[data.group].AddLight(transform_system, entity, data);
  points_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const SpotLightDefT& data) {
  auto* transform_system = registry_->Get<lull::TransformSystem>();
  groups_[data.group].AddLight(transform_system, entity, data);
  spot_lights_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const LightableDefT& data) {
  LightGroup& group = groups_[data.group];
  group.AddLightable(registry_, entity, data);
  entity_to_group_map_[entity] = data.group;
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
  } else if (blueprint.Is<SpotLightDefT>()) {
    SpotLightDefT light;
    blueprint.Read(&light);
    CreateLight(entity, light);
  } else if (blueprint.Is<DirectionalLightDefT>()) {
    DirectionalLightDefT light;
    blueprint.Read(&light);
    CreateLight(entity, light);
  } else if (blueprint.Is<EnvironmentLightDefT>()) {
    EnvironmentLightDefT light;
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
  spot_lights_.erase(entity);
}

void LightSystem::AdvanceFrame() {
  auto* transform_system = registry_->Get<lull::TransformSystem>();
  UpdateLightTransforms(transform_system, directionals_);
  UpdateLightTransforms(transform_system, points_);
  UpdateLightTransforms(transform_system, spot_lights_);

  auto* render_system = registry_->Get<RenderSystem>();
  for (auto& iter : groups_) {
    iter.second.Update(transform_system, render_system);
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

void LightSystem::LightGroup::AddLight(Registry* registry, Entity entity,
                                       const DirectionalLightDefT& light) {
  auto* transform_system = registry->Get<lull::TransformSystem>();
  const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
  auto& new_light = directionals_[entity];
  new_light = light;
  new_light.rotation = sqt.rotation;
  dirty_ = true;

  if (HasShadows(light)) {
    CreateShadowPass(registry, entity, light);
  }
}

void LightSystem::LightGroup::AddLight(Registry* registry, Entity entity,
                                       const EnvironmentLightDefT& light) {
  auto* texture_factory = registry->Get<TextureFactory>();
  if (texture_factory) {
    environment_entity_ = entity;
    environment_diffuse_texture_ =
        texture_factory->CreateTexture(light.diffuse);
    if (!light.specular.file.empty() || !light.specular.data.empty()) {
      environment_specular_texture_ =
          texture_factory->CreateTexture(light.specular);
    }
    if (!light.brdf_lookup.file.empty() || !light.brdf_lookup.data.empty()) {
      environment_brdf_lookup_table_ =
          texture_factory->CreateTexture(light.brdf_lookup);
    }
  }
  environment_light_ = light;
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

void LightSystem::LightGroup::AddLight(TransformSystem* transform_system,
                                       Entity entity,
                                       const SpotLightDefT& light) {
  const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
  auto& new_light = spot_lights_[entity];
  new_light = light;
  new_light.position = sqt.translation;
  new_light.rotation = sqt.rotation;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLightable(Registry* registry, Entity entity,
                                           const LightableDefT& lightable) {
  auto& new_lightable = lightables_[entity];
  new_lightable = lightable;
  dirty_lightables_.insert(entity);

  auto* render_system = registry->Get<RenderSystem>();
  if (lightable.max_point_lights > 0) {
    render_system->RequestShaderFeature(entity, lull::ConstHash("PointLight"));
  }
  if (lightable.shadow_interaction == ShadowInteraction_CastAndReceive) {
    auto* dispatcher_system = registry->Get<lull::DispatcherSystem>();
    for (const auto& shadow_pass_data : shadow_passes_) {
      AddLightableToShadowPass(render_system, dispatcher_system, entity,
                               shadow_pass_data.pass, lightable);
    }
  }
}

void LightSystem::LightGroup::Remove(Registry* registry, Entity entity) {
  if (lightables_.erase(entity)) {
    auto* render_system = registry->Get<RenderSystem>();
    for (const auto& shadow_pass_data : shadow_passes_) {
      RemoveLightableFromShadowPass(render_system, entity,
                                    shadow_pass_data.pass);
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
  if (spot_lights_.erase(entity)) {
    dirty_ = true;
  }
  if (environment_entity_ == entity) {
    environment_diffuse_texture_ = nullptr;
    environment_specular_texture_ = nullptr;
    environment_brdf_lookup_table_ = nullptr;
    environment_entity_ = kNullEntity;
    dirty_ = true;
  }
}

void LightSystem::LightGroup::Update(TransformSystem* transform_system,
                                     RenderSystem* render_system) {
  if (dirty_) {
    for (auto& shadow_pass_data : shadow_passes_) {
      UpdateRenderViewTransform(transform_system,
                                shadow_pass_data.transform_entity,
                                &shadow_pass_data.view);
    }
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
    ++shadow_count;
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
  UpdateUniforms(&uniforms, spot_lights_, /*max_allowed=*/1, 0);

  if (data.apply_environment_light && environment_light_) {
    uniforms.Add(*environment_light_);
    render_system->SetTexture(entity, MaterialTextureUsage_DiffuseEnvironment,
                              environment_diffuse_texture_);
    if (environment_specular_texture_) {
      render_system->SetTexture(entity,
                                MaterialTextureUsage_SpecularEnvironment,
                                environment_specular_texture_);
    }
    if (environment_brdf_lookup_table_) {
      render_system->SetTexture(entity, MaterialTextureUsage_BrdfLookupTable,
                                environment_brdf_lookup_table_);
    }
  }

  uniforms.Apply(render_system, entity);

  // Special case: Also update the shadow matrices.
  for (const auto& shadow_pass : shadow_passes_) {
    render_system->SetUniform(entity, kLightMatrixUniformName,
                              &shadow_pass.view.clip_from_world_matrix[0],
                              16 /*=dimensions*/, 1 /*=count*/);
  }
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

  auto spot_iter = spot_lights_.find(entity);
  if (spot_iter != spot_lights_.end()) {
    auto& light = spot_iter->second;
    const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
    if (light.position != sqt.translation) {
      light.position = sqt.translation;
      light.rotation = sqt.rotation;
      dirty_ = true;
    }
  }
}

bool LightSystem::LightGroup::Empty() const {
  return ambients_.empty() && directionals_.empty() && points_.empty() &&
         spot_lights_.empty() && lightables_.empty();
}

void LightSystem::LightGroup::DestroyShadowPass(RenderSystem* render_system,
                                                HashValue pass) {
  auto iter = std::find_if(shadow_passes_.begin(), shadow_passes_.end(),
                           [pass](const ShadowPassData& shadow_pass_data) {
                             return pass == shadow_pass_data.pass;
                           });
  if (iter == shadow_passes_.end()) {
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
  colors.data.push_back(light.color.r);
  colors.data.push_back(light.color.g);
  colors.data.push_back(light.color.b);
}

void LightSystem::UniformData::Add(const DirectionalLightDefT& light) {
  const bool has_shadow = HasShadows(light);
  auto& colors =
      buffers[has_shadow ? kShadowColorUniformName : kColorUniformName];
  colors.dimension = 3;
  colors.data.push_back(light.color.r);
  colors.data.push_back(light.color.g);
  colors.data.push_back(light.color.b);

  auto& directions =
      buffers[has_shadow ? kShadowDirectionUniformName : kDirectionUniformName];
  directions.dimension = 3;

  const mathfu::vec3 light_dir = light.rotation * -mathfu::kAxisZ3f;
  directions.data.push_back(light_dir.x);
  directions.data.push_back(light_dir.y);
  directions.data.push_back(light_dir.z);

  if (light.exponent != 0.0f) {
    auto& exponents =
        buffers[has_shadow ? kShadowExponentUniformName : kExponentUniformName];
    exponents.dimension = 1;
    exponents.data.push_back(light.exponent);
  }
}

void LightSystem::UniformData::Add(const SpotLightDefT& light) {
  auto& colors = buffers["light_spotlight_color"];
  colors.dimension = 3;
  colors.data.push_back(light.color.r * light.intensity);
  colors.data.push_back(light.color.g * light.intensity);
  colors.data.push_back(light.color.b * light.intensity);

  auto& positions = buffers["light_spotlight_pos"];
  positions.dimension = 3;
  positions.data.push_back(light.position.x);
  positions.data.push_back(light.position.y);
  positions.data.push_back(light.position.z);

  const mathfu::vec3 light_dir = light.rotation * -mathfu::kAxisZ3f;
  auto& directions = buffers["light_spotlight_dir"];
  directions.dimension = 3;
  directions.data.push_back(light_dir.x);
  directions.data.push_back(light_dir.y);
  directions.data.push_back(light_dir.z);

  auto& decay = buffers["light_spotlight_decay"];
  decay.dimension = 1;
  decay.data.push_back(light.decay);

  const float angle_in_radians =
      std::min(light.angle, 90.0f) * kDegreesToRadians;
  auto& angle = buffers["light_spotlight_angle_cos"];
  angle.dimension = 1;
  angle.data.push_back(std::cos(angle_in_radians));

  auto& penumbra = buffers["light_spotlight_penumbra_cos"];
  penumbra.dimension = 1;
  penumbra.data.push_back(
      std::cos(mathfu::Clamp(light.penumbra, 0.0f, 1.0f) * angle_in_radians));
}

void LightSystem::UniformData::Add(const PointLightDefT& light) {
  auto& colors = buffers["light_point_color"];
  colors.dimension = 3;
  colors.data.push_back(light.color.r * light.intensity);
  colors.data.push_back(light.color.g * light.intensity);
  colors.data.push_back(light.color.b * light.intensity);

  auto& positions = buffers["light_point_pos"];
  positions.dimension = 3;
  positions.data.push_back(light.position.x);
  positions.data.push_back(light.position.y);
  positions.data.push_back(light.position.z);

  if (light.exponent != 0.0f) {
    auto& exponents = buffers["light_point_exponent"];
    exponents.dimension = 1;
    exponents.data.push_back(light.exponent);
  }
}

void LightSystem::UniformData::Add(const EnvironmentLightDefT& light) {
  auto& colors = buffers["light_environment_color_factor"];
  colors.dimension = 3;
  colors.data.push_back(light.color.r);
  colors.data.push_back(light.color.g);
  colors.data.push_back(light.color.b);

  auto& mips = buffers["num_mips"];
  mips.dimension = 1;
  mips.data.push_back(static_cast<float>(light.specular_mips));
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

  for (const auto& group : groups_) {
    group.second.RenderShadowMaps(render_system);
  }
}

void LightSystem::LightGroup::RenderShadowMaps(
    RenderSystem* render_system) const {
  for (const auto& shadow_pass : shadow_passes_) {
    render_system->Render(&shadow_pass.view, 1,
                          static_cast<RenderPass>(shadow_pass.pass));
  }
}

void LightSystem::LightGroup::AddLightableToShadowPass(
    RenderSystem* render_system, DispatcherSystem* dispatcher_system,
    Entity entity, HashValue pass, const LightableDefT& lightable) {
  if (!dispatcher_system) {
    LOG(FATAL) << "Must create the DispatcherSystem to use shadows.";
    return;
  }

  render_system->Create(entity, static_cast<RenderPass>(pass));
  render_system->SetMesh({entity, pass}, render_system->GetMesh(entity));
  if (lightable.depth_shader.empty()) {
    LOG(DFATAL) << "Missing depth shader for shadow casting entity.";
  } else {
    render_system->SetMaterial({entity, pass},
                               MaterialInfo(lightable.depth_shader));
  }
  render_system->SetTexture({entity, 0}, lightable.shadow_sampler,
                            render_system->GetTexture(pass));

  dispatcher_system->Connect(
      entity, this,
      [render_system, entity, pass](const MeshChangedEvent& event) {
        if (event.pass != pass) {
          render_system->SetMesh({entity, pass},
                                 render_system->GetMesh({entity, event.pass}));
        }
      });
  dirty_lightables_.insert(entity);
}

void LightSystem::LightGroup::CreateShadowPass(
    Registry* registry, Entity entity, const DirectionalLightDefT& data) {
  const ShadowMapDefT* shadow_def = data.shadow_def.get<ShadowMapDefT>();
  if (!shadow_def) {
    return;
  }

  // Create the render target.
  auto* render_system = registry->Get<RenderSystem>();
  const HashValue pass =
      RenderPassNameFromEntityAndLightGroup(entity, data.group);
  RenderTargetCreateParams create_params;
  create_params.dimensions = mathfu::vec2i(shadow_def->shadow_resolution,
                                           shadow_def->shadow_resolution);
  create_params.texture_format = TextureFormat_Depth16;
  create_params.depth_stencil_format = DepthStencilFormat_None;
  render_system->CreateRenderTarget(pass, create_params);

  // Set the render target for the pass.
  render_system->SetRenderTarget(pass, pass);

  // Set the render state for the pass.
  fplbase::RenderState render_state;
  render_state.depth_state.test_enabled = true;
  render_state.depth_state.write_enabled = true;
  render_state.depth_state.function = fplbase::kRenderLess;
  render_state.cull_state.enabled = true;
  render_state.cull_state.face = fplbase::CullState::kBack;
  render_system->SetRenderState(pass, render_state);

  // Set the clear params for the pass.
  RenderClearParams clear_params;
  clear_params.clear_options = RenderClearParams::kDepth;
  render_system->SetClearParams(pass, clear_params);

  // Set the sort mode for the pass.
  render_system->SetSortMode(pass, SortMode_AverageSpaceOriginFrontToBack);

  // Create the viewport for rendering the shadow pass.
  ShadowPassData shadow_pass_data;
  shadow_pass_data.transform_entity = entity;
  shadow_pass_data.pass = pass;
  shadow_pass_data.view.viewport.x = 0;
  shadow_pass_data.view.viewport.y = 0;
  shadow_pass_data.view.dimensions = create_params.dimensions;

  // Construct the view and projection matrices.
  const float half_shadow_volume = shadow_def->shadow_volume * 0.5f;
  shadow_pass_data.view.clip_from_eye_matrix = mathfu::mat4::Ortho(
      -half_shadow_volume, half_shadow_volume, -half_shadow_volume,
      half_shadow_volume, shadow_def->shadow_min_distance,
      shadow_def->shadow_max_distance, 1.0f);

  auto* transform_system = registry->Get<lull::TransformSystem>();
  UpdateRenderViewTransform(transform_system, entity, &shadow_pass_data.view);

  // Add lightables to the shadow pass.
  auto* dispatcher_system = registry->Get<lull::DispatcherSystem>();
  for (const auto& lightable : lightables_) {
    if (lightable.second.shadow_interaction ==
        ShadowInteraction_CastAndReceive) {
      AddLightableToShadowPass(render_system, dispatcher_system,
                               lightable.first, shadow_pass_data.pass,
                               lightable.second);
    }
  }

  // Add the shadow pass.
  shadow_passes_.push_back(std::move(shadow_pass_data));
}

}  // namespace lull
