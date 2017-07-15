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

#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/mathfu_fb_conversions.h"

namespace lull {

static constexpr HashValue kAmbientLightDef = ConstHash("AmbientLightDef");
static constexpr HashValue kDirectionalLightDef =
    ConstHash("DirectionalLightDef");
static constexpr HashValue kLightableDef = ConstHash("LightableDef");
static constexpr HashValue kPointLightDef = ConstHash("PointLightDef");

LightSystem::LightSystem(Registry* registry) : System(registry) {
  RegisterDef(this, kAmbientLightDef);
  RegisterDef(this, kDirectionalLightDef);
  RegisterDef(this, kLightableDef);
  RegisterDef(this, kPointLightDef);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void LightSystem::Create(Entity entity, HashValue group,
                         const AmbientLight& data) {
  groups_[group].AddLight(entity, data);
  ambients_.insert(entity);
  entity_to_group_map_[entity] = group;
}

void LightSystem::Create(Entity entity, HashValue group,
                         const DirectionalLight& data) {
  DirectionalLight light = data;
  const auto* transform_system = registry_->Get<lull::TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(entity);
  if (!sqt) {
    LOG(DFATAL) << "Directional Light is missing a transform component.";
    return;
  }
  light.rotation = sqt->rotation;

  groups_[group].AddLight(entity, light);
  directionals_.insert(entity);
  entity_to_group_map_[entity] = group;
}

void LightSystem::Create(Entity entity, HashValue group,
                         const Lightable& data) {
  groups_[group].AddLightable(entity, data);
  entity_to_group_map_[entity] = group;
}

void LightSystem::Create(Entity entity, HashValue group,
                         const PointLight& data) {
  PointLight light = data;
  const auto* transform_system = registry_->Get<lull::TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(entity);
  if (!sqt) {
    LOG(DFATAL) << "Point Light is missing a transform component.";
    return;
  }
  light.position = sqt->translation;
  groups_[group].AddLight(entity, light);
  points_.insert(entity);
  entity_to_group_map_[entity] = group;
}

void LightSystem::PostCreateInit(Entity entity, HashValue type,
                                 const Def* def) {
  if (def == nullptr) {
    LOG(DFATAL) << "Def is null.";
    return;
  }
  if (entity_to_group_map_.count(entity)) {
    LOG(DFATAL) << "Entity already has a light.";
    return;
  }

  switch (type) {
    case kAmbientLightDef:
      Create(entity, *ConvertDef<AmbientLightDef>(def));
      return;
    case kDirectionalLightDef:
      Create(entity, *ConvertDef<DirectionalLightDef>(def));
      return;
    case kLightableDef:
      Create(entity, *ConvertDef<LightableDef>(def));
      return;
    case kPointLightDef:
      Create(entity, *ConvertDef<PointLightDef>(def));
      return;
    default:
      LOG(DFATAL) << "Invalid light type: " << type;
      return;
  }
}

void LightSystem::Create(Entity entity, const AmbientLightDef& data) {
  AmbientLight light;
  Color4ubFromFbColor(data.color(), &light.color);
  const HashValue group = Hash(data.group()->c_str());

  Create(entity, group, light);
}

void LightSystem::Create(Entity entity, const DirectionalLightDef& data) {
  DirectionalLight light;
  Color4ubFromFbColor(data.color(), &light.color);
  light.exponent = data.exponent();
  const HashValue group = Hash(data.group()->c_str());

  Create(entity, group, light);
}

void LightSystem::Create(Entity entity, const LightableDef& data) {
  Lightable lightable;
  lightable.max_ambient_lights = data.max_ambient_lights();
  lightable.max_directional_lights = data.max_directional_lights();
  lightable.max_point_lights = data.max_point_lights();
  const HashValue group = Hash(data.group()->c_str());

  Create(entity, group, lightable);
}

void LightSystem::Create(Entity entity, const PointLightDef& data) {
  PointLight light;
  Color4ubFromFbColor(data.color(), &light.color);
  light.exponent = data.exponent();
  light.intensity = data.intensity();
  const HashValue group = Hash(data.group()->c_str());

  Create(entity, group, light);
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

void LightSystem::LightGroup::AddLight(Entity entity,
                                       const AmbientLight& light) {
  ambients_[entity] = light;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLight(Entity entity,
                                       const DirectionalLight& light) {
  directionals_[entity] = light;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLight(Entity entity, const PointLight& light) {
  points_[entity] = light;
  dirty_ = true;
}

void LightSystem::LightGroup::AddLightable(Entity entity,
                                           const Lightable& lightable) {
  lightables_[entity] = lightable;
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
    dirty_lightables_.clear();

    for (auto it : lightables_) {
      UpdateLightable(render_system, it.first, &it.second);
    }
    dirty_ = false;
  } else {
    for (auto it : dirty_lightables_) {
      UpdateLightable(render_system, it);
    }
    dirty_lightables_.clear();
  }
}

void LightSystem::LightGroup::UpdateLightable(RenderSystem* render_system,
                                              Entity entity) {
  auto iter = lightables_.find(entity);
  if (iter == lightables_.end()) {
    return;
  }

  UpdateLightable(render_system, entity, &iter->second);
}

void LightSystem::LightGroup::UpdateLightable(RenderSystem* render_system,
                                              Entity entity,
                                              const Lightable* data) {
  UniformData uniforms;
  if (data->max_ambient_lights) {
    int count = 0;
    for (auto it : ambients_) {
      if (count >= data->max_ambient_lights) {
        LOG(WARNING)
            << "Light group has more ambient lights than entity can accept.";
        break;
      }
      uniforms.Add(it.second);
      ++count;
    }

    while (count < data->max_ambient_lights) {
      AmbientLight l;
      l.color = Color4ub(0, 0, 0, 0);
      uniforms.Add(l);
      ++count;
    }
  }

  if (data->max_directional_lights) {
    int count = 0;
    for (auto it : directionals_) {
      if (count >= data->max_directional_lights) {
        LOG(WARNING) << "Light group has more directional lights than entity "
                        "can accept.";
        break;
      }
      uniforms.Add(it.second);
      ++count;
    }

    while (count < data->max_directional_lights) {
      DirectionalLight l;
      l.color = Color4ub(0, 0, 0, 0);
      uniforms.Add(l);
      ++count;
    }
  }

  if (data->max_point_lights) {
    int count = 0;
    for (auto it : points_) {
      if (count >= data->max_point_lights) {
        LOG(WARNING)
            << "Light group has more point lights than entity can accept.";
        break;
      }
      uniforms.Add(it.second);
      ++count;
    }

    while (count < data->max_point_lights) {
      PointLight l;
      l.color = Color4ub(0, 0, 0, 0);
      uniforms.Add(l);
      ++count;
    }
  }

  uniforms.Apply(render_system, entity);
}

static Sqt GetWorldFromEntitySqt(TransformSystem* transform_system,
                                 Entity entity) {
  const mathfu::mat4* matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  return CalculateSqtFromMatrix(matrix);
}

void LightSystem::LightGroup::UpdateLight(TransformSystem* transform_system,
                                          Entity entity) {
  auto directional_iter = directionals_.find(entity);
  if (directional_iter != directionals_.end()) {
    auto light = directional_iter->second;
    const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
    if (light.rotation.scalar() != sqt.rotation.scalar() ||
        light.rotation.vector() != sqt.rotation.vector()) {
      light.rotation = sqt.rotation;
      directionals_[entity] = light;
      dirty_ = true;
    }
  }
  auto point_iter = points_.find(entity);
  if (point_iter != points_.end()) {
    auto light = point_iter->second;
    const Sqt sqt = GetWorldFromEntitySqt(transform_system, entity);
    if (light.position != sqt.translation) {
      light.position = sqt.translation;
      points_[entity] = light;
      dirty_ = true;
    }
  }
}

bool LightSystem::LightGroup::Empty() const {
  return ambients_.empty() && directionals_.empty() && points_.empty() &&
         lightables_.empty();
}

void LightSystem::UniformData::Clear() { buffers.clear(); }

void LightSystem::UniformData::Add(const AmbientLight& light) {
  auto& colors = buffers["light_ambient_color"];
  colors.dimension = 3;

  const mathfu::vec4 data = Color4ub::ToVec4(light.color);
  colors.data.push_back(data.x);
  colors.data.push_back(data.y);
  colors.data.push_back(data.z);
}

void LightSystem::UniformData::Add(const DirectionalLight& light) {
  auto& colors = buffers["light_directional_color"];
  colors.dimension = 3;

  const mathfu::vec4 data = Color4ub::ToVec4(light.color);
  colors.data.push_back(data.x);
  colors.data.push_back(data.y);
  colors.data.push_back(data.z);

  auto& directions = buffers["light_directional_dir"];
  const mathfu::vec3 light_dir = light.rotation * -mathfu::kAxisZ3f;
  directions.dimension = 3;
  directions.data.push_back(light_dir.x);
  directions.data.push_back(light_dir.y);
  directions.data.push_back(light_dir.z);

  auto& exponents = buffers["light_directional_exponent"];
  exponents.dimension = 1;
  exponents.data.push_back(light.exponent);
}

void LightSystem::UniformData::Add(const PointLight& light) {
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
  for (auto it : buffers) {
    const auto& buffer = it.second;
    const int count = static_cast<int>(buffer.data.size()) / buffer.dimension;
    render_system->SetUniform(entity, it.first.c_str(), buffer.data.data(),
                              buffer.dimension, count);
  }
}

}  // namespace lull
