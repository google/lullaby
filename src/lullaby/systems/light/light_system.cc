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

namespace lull {

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
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void LightSystem::CreateLight(Entity entity, const AmbientLightDefT& data) {
  const auto* transform_system = registry_->Get<lull::TransformSystem>();
  groups_[data.group].AddLight(entity, data, transform_system);
  ambients_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const DirectionalLightDefT& data) {
  const auto* transform_system = registry_->Get<lull::TransformSystem>();
  groups_[data.group].AddLight(entity, data, transform_system);
  directionals_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const PointLightDefT& data) {
  const auto* transform_system = registry_->Get<lull::TransformSystem>();
  groups_[data.group].AddLight(entity, data, transform_system);
  points_.insert(entity);
  entity_to_group_map_[entity] = data.group;
}

void LightSystem::CreateLight(Entity entity, const LightableDefT& data) {
  groups_[data.group].AddLightable(entity, data);
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
    group->second.Remove(entity);
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

void LightSystem::LightGroup::AddLight(
    Entity entity, const AmbientLightDefT& light,
    const TransformSystem* transform_system) {
  auto& new_light = ambients_[entity];
  new_light = light;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLight(
    Entity entity, const DirectionalLightDefT& light,
    const TransformSystem* transform_system) {
  const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
  auto& new_light = directionals_[entity];
  new_light = light;
  new_light.rotation = sqt.rotation;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLight(
    Entity entity, const PointLightDefT& light,
    const TransformSystem* transform_system) {
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

void LightSystem::LightGroup::Remove(Entity entity) {
  lightables_.erase(entity);
  if (ambients_.erase(entity)) {
    dirty_ = true;
  }
  if (directionals_.erase(entity)) {
    dirty_ = true;
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

template <typename T>
void LightSystem::UpdateUniforms(UniformData* uniforms,
                                 const std::unordered_map<Entity, T>& lights,
                                 int max_allowed) {
  int count = 0;
  for (const auto& light : lights) {
    if (count >= max_allowed) {
#ifndef _DEBUG
      LOG(WARNING) << "Entity has a maximum of " << max_allowed << " "
                   << GetTypeName<T>() << " lights, however there are "
                   << lights.size() << " defined lights.";
#endif
      break;
    }
    uniforms->Add(light.second);
    ++count;
  }
  while (count < max_allowed) {
    const T blacklight;
    uniforms->Add(blacklight);
    ++count;
  }
}

void LightSystem::LightGroup::UpdateLightable(RenderSystem* render_system,
                                              Entity entity,
                                              const LightableDefT& data) {
  UniformData uniforms;
  UpdateUniforms(&uniforms, ambients_, data.max_ambient_lights);
  UpdateUniforms(&uniforms, directionals_, data.max_directional_lights);
  UpdateUniforms(&uniforms, points_, data.max_point_lights);
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
  auto& colors = buffers["light_directional_color"];
  colors.dimension = 3;

  const mathfu::vec4 data = Color4ub::ToVec4(light.color);
  colors.data.push_back(data.x);
  colors.data.push_back(data.y);
  colors.data.push_back(data.z);

  auto& directions = buffers["light_directional_dir"];
  directions.dimension = 3;

  const mathfu::vec3 light_dir = light.rotation * -mathfu::kAxisZ3f;
  directions.data.push_back(light_dir.x);
  directions.data.push_back(light_dir.y);
  directions.data.push_back(light_dir.z);

  auto& exponents = buffers["light_directional_exponent"];
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

}  // namespace lull
