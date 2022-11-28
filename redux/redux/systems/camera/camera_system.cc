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

#include "redux/engines/platform/device_manager.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/math/math.h"
#include "redux/modules/math/ray.h"

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

CameraOps CameraSystem::GetCameraOps(Entity entity) const {
  auto iter = cameras_.find(entity);
  if (iter != cameras_.end()) {
    RenderLayerPtr layer = GetRenderLayer(iter->second.layer);

    const Transform transform = transform_system_->GetTransform(entity);
    const vec3& position = transform.translation;
    const quat& rotation = transform.rotation;
    const mat4 projection = CalculateProjectionMatrix(iter->second, layer);
    const Bounds2i viewport = layer->GetAbsoluteViewport();
    return CameraOps(position, rotation, projection, viewport);
  }
  return CameraOps(vec3::Zero(), quat::Identity(), mat4::Identity(),
                   Bounds2i::Empty());
}

RenderLayerPtr CameraSystem::GetRenderLayer(HashValue key) const {
  RenderLayerPtr layer;
  if (key != HashValue()) {
    layer = render_engine_->GetRenderLayer(key);
    CHECK(layer) << "Unable to get RenderLayer: " << key.get();
  } else {
    layer = render_engine_->GetDefaultRenderLayer();
    CHECK(layer) << "Unable to get default RenderLayer.";
  }
  return layer;
}

mat4 CameraSystem::CalculateProjectionMatrix(const Camera& camera,
                                             const RenderLayerPtr& layer) {
  const vec2i size = layer->GetAbsoluteViewport().Size();
  const float width = static_cast<float>(size.x);
  const float height = static_cast<float>(size.y);
  const float aspect_ratio = height > 0.f ? width / height : 1.f;
  const mat4 projection = PerspectiveMatrix(
      camera.horizontal_fov, aspect_ratio, camera.near_plane, camera.far_plane);
  return projection;
}

void CameraSystem::UpdateRenderLayers() {
  for (const auto& iter : cameras_) {
    RenderLayerPtr layer = GetRenderLayer(iter.second.layer);

    const Transform transform = transform_system_->GetTransform(iter.first);
    const mat4 view_matrix =
        TransformMatrix(transform.translation, transform.rotation, vec3::One());
    layer->SetViewMatrix(view_matrix);

    const mat4 projection_matrix =
        CalculateProjectionMatrix(iter.second, layer);
    layer->SetProjectionMatrix(projection_matrix);
  }
}

}  // namespace redux
