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

#include "redux/systems/camera/camera_system.h"

#include <memory>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/math/matrix.h"

namespace redux {

CameraSystem::CameraSystem(Registry* registry) : System(registry) {
  RegisterDef(&CameraSystem::SetFromCameraDef);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<RenderEngine>(this);
}

void CameraSystem::OnRegistryInitialize() {
  transform_system_ = registry_->Get<TransformSystem>();
  render_engine_ = registry_->Get<RenderEngine>();

  auto choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo
        ->Add<&CameraSystem::UpdateRenderLayers>(Choreographer::Stage::kRender)
        .Before<&RenderEngine::Render>();
  }
}

void CameraSystem::SetFromCameraDef(Entity entity, const CameraDef& def) {
  SetViewport(entity, def.viewport);
  SetClipPlanes(entity, def.near_plane_distance, def.far_plane_distance);
  SetHorizontalFieldOfViewAngle(entity, def.horizontal_field_of_view_angle);
  SetExposure(entity, def.aperture, def.shutter_speed, def.iso_sensitivity);
  SetFocalDistance(entity, def.focus_distance);
}

void CameraSystem::OnDestroy(Entity entity) { cameras_.erase(entity); }

void CameraSystem::SetRenderLayer(Entity entity, HashValue render_layer_name) {
  Camera& camera = cameras_[entity];
  camera.layer = render_layer_name;

  RenderLayerPtr layer = GetRenderLayer(camera.layer);
  if (layer) {
    layer->SetViewport(camera.viewport);
  }
}

void CameraSystem::SetViewport(Entity entity, const Bounds2f& viewport) {
  Camera& camera = cameras_[entity];
  camera.viewport = viewport;

  RenderLayerPtr layer = GetRenderLayer(camera.layer);
  if (layer) {
    layer->SetViewport(camera.viewport);
  }
}

void CameraSystem::SetClipPlanes(Entity entity, float near_plane,
                                 float far_plane) {
  Camera& camera = cameras_[entity];
  camera.near_plane = near_plane;
  camera.far_plane = far_plane;
}

void CameraSystem::SetHorizontalFieldOfViewAngle(Entity entity,
                                                 float horizontal_fov) {
  Camera& camera = cameras_[entity];
  camera.horizontal_fov = horizontal_fov;
}

void CameraSystem::SetAspectRatio(Entity entity, float aspect_ratio) {
  Camera& camera = cameras_[entity];
  camera.aspect_ratio = aspect_ratio;
}

void CameraSystem::SetExposure(Entity entity, float aperture,
                               float shutter_speed, float iso_sensitivity) {
  Camera& camera = cameras_[entity];
  camera.aperture = aperture;
  camera.shutter_speed = shutter_speed;
  camera.iso_sensitivity = iso_sensitivity;
}

void CameraSystem::SetFocalDistance(Entity entity, float distance) {
  Camera& camera = cameras_[entity];
  camera.focus_distance = distance;
}

CameraOps CameraSystem::GetCameraOps(Entity entity) const {
  auto iter = cameras_.find(entity);
  if (iter != cameras_.end()) {
    RenderLayerPtr layer = GetRenderLayer(iter->second.layer);

    const Transform transform = transform_system_->GetTransform(entity);
    const vec3& position = transform.translation;
    const quat& rotation = transform.rotation;
    const mat4 projection = CalculateProjectionMatrix(iter->second);
    const Bounds2i viewport = layer->GetAbsoluteViewport();
    return CameraOps(position, rotation, projection, viewport);
  }
  return CameraOps(vec3::Zero(), quat::Identity(), mat4::Identity(),
                   Bounds2i::Empty());
}

RenderLayerPtr CameraSystem::GetRenderLayer(HashValue key) const {
  if (key == HashValue()) {
    key = render_engine_->GetDefaultRenderLayerName();
  }
  RenderLayerPtr layer = render_engine_->GetRenderLayer(key);
  CHECK(layer) << "Unable to get RenderLayer: " << key.get();
  return layer;
}

mat4 CameraSystem::CalculateProjectionMatrix(const Camera& camera) {
  return PerspectiveMatrix(camera.horizontal_fov, camera.aspect_ratio,
                           camera.near_plane, camera.far_plane);
}

void CameraSystem::UpdateRenderLayers() {
  for (const auto& iter : cameras_) {
    RenderLayerPtr layer = GetRenderLayer(iter.second.layer);

    const Transform transform = transform_system_->GetTransform(iter.first);
    const mat4 view_matrix =
        TransformMatrix(transform.translation, transform.rotation, vec3::One());
    layer->SetViewMatrix(view_matrix);

    const mat4 projection_matrix = CalculateProjectionMatrix(iter.second);
    layer->SetProjectionMatrix(projection_matrix);
    layer->SetCameraExposure(iter.second.aperture, iter.second.shutter_speed,
                             iter.second.iso_sensitivity);
    layer->SetCameraFocalDistance(iter.second.focus_distance);
  }
}

}  // namespace redux
