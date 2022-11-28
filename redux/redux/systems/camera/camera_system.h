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

#ifndef REDUX_SYSTEMS_CAMERA_CAMERA_SYSTEM_H_
#define REDUX_SYSTEMS_CAMERA_CAMERA_SYSTEM_H_

#include <optional>

#include "redux/engines/render/render_engine.h"
#include "redux/engines/render/render_layer.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/graphics/camera_ops.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/ray.h"
#include "redux/systems/camera/camera_def_generated.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

// Associates Camera properties with an Entity which will be used to update
// the relevant settings of a RenderLayer.
//
// Note: see RenderSystem for more information about render layers.
class CameraSystem : public System {
 public:
  explicit CameraSystem(Registry* registry);

  void OnRegistryInitialize();

  // Sets the name of the RenderLayer which will be updated based on the camera
  // properties on the Entity.
  void SetRenderLayer(Entity entity, HashValue render_layer_name);

  // Sets the viewport into which the camera will render. The bounds should be
  // specified in the range (0,0) (bottom-left) to (1,1) (top-right).
  void SetViewport(Entity entity, const Bounds2f& viewport);

  // Sets the near and far clip planes that will be used for the rendering
  // projection matrix.
  void SetClipPlanes(Entity entity, float near_plane, float far_plane);

  // Sets the horizontal field-of-view angle for perspective projection. The
  // angle should be specified in radians.
  void SetHorizontalFieldOfViewAngle(Entity entity, float horizontal_fov);

  // Returns the CameraOps associated with the Entity.
  CameraOps GetCameraOps(Entity entity) const;

  // Sets the camera properties for the Entity from the CameraDef.
  void SetFromCameraDef(Entity entity, const CameraDef& def);

  // Updates RenderLayers based on the managed camera properties. Note: this
  // function is automatically bound to run before rendering if the
  // choreographer is available.
  void UpdateRenderLayers();

 private:
  struct Camera {
    HashValue layer;
    Bounds2f viewport = Bounds2f({0.0f, 0.0f}, {1.0f, 1.0f});
    float far_plane = 1000.0f;
    float near_plane = 0.01f;
    float horizontal_fov = DegreesToRadians(90.f);
  };

  void OnDestroy(Entity entity) override;

  RenderLayerPtr GetRenderLayer(HashValue key) const;

  static mat4 CalculateProjectionMatrix(const Camera& camera,
                                        const RenderLayerPtr& layer);

  absl::flat_hash_map<Entity, Camera> cameras_;
  TransformSystem* transform_system_ = nullptr;
  RenderEngine* render_engine_ = nullptr;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::CameraSystem);

#endif  // REDUX_SYSTEMS_CAMERA_CAMERA_SYSTEM_H_
