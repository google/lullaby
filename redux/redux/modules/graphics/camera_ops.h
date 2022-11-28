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

#ifndef REDUX_MODULES_GRAPHICS_CAMERA_OPS_H_
#define REDUX_MODULES_GRAPHICS_CAMERA_OPS_H_

#include <optional>

#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/ray.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Storage for key data for a camera (ie. view and projection matrices) and
// related operations that can be performed on them.
class CameraOps {
 public:
  CameraOps(const vec3& position, const quat& rotation, const mat4& projection,
            const Bounds2i& viewport);

  // Returns the camera position.
  const vec3& WorldPosition() const;

  // Returns the camera rotation.
  const quat& WorldRotation() const;

  // Returns the viewport within which the camera operations will be performed.
  const Bounds2i& Viewport() const;

  // Calculates the view-projection matrix.
  mat4 ClipFromWorld() const;

  // Calculates the inverse view-projection matrix.
  mat4 WorldFromClip() const;

  // Calculates the projection matrix.
  mat4 ClipFromCamera() const;

  // Calculates the inverse projection matrix.
  mat4 CameraFromClip() const;

  // Calculates the view matrix.
  mat4 CameraFromWorld() const;

  // Calculates the inverse view matrix.
  mat4 WorldFromCamera() const;

  // Projects a ray from a clip coordinate into world space. |clip_point|'s
  // values should be in the range ([-1,1], [-1,1], [0,1]).
  Ray WorldRayFromClipPoint(const vec3& clip_point) const;

  // Projects a ray from a camera texture coordinate into world space. |uv|
  // should have values in the range [0,1).
  Ray WorldRayFromUv(const vec2& uv) const;

  // Projects a ray from a pixel into world space. |pixel| should have values in
  // the viewport.  Returns an unset Optional if the viewport is not set up.
  std::optional<Ray> WorldRayFromPixel(const vec2& pixel) const;

  // Converts a point in world space to a pixel.  If the |world_point| is
  // outside the view frustrum, returned pixel may have NaN values.  Returns an
  // unset Optional if the viewport is not set up.
  std::optional<vec2> PixelFromWorldPoint(const vec3& world_point) const;

  // Converts a point in clip space to world space. |clip_point|'s values should
  // be in the range ([-1,1], [-1,1], [0,1]) for a result in the view frustrum.
  vec3 WorldPointFromClip(const vec3& clip_point) const;

  // Converts a point in world space to clip space.  If |world_point| is in the
  // view frustrum, the result will be in the range ([-1,1], [-1,1], [0,1]).
  vec3 ClipFromWorldPoint(const vec3& world_point) const;

  // Converts a point in world space to camera texture space.  If |world_point|
  // is in the view frustrum, the result's values will be in the range [0,1].
  vec2 UvFromWorldPoint(const vec3& world_point) const;

  // Converts a point in pixel space to clip space.  If |pixel|'s values
  // are in the viewport, the result will be in the range ([-1,1], [-1,1], 0).
  // Returns an unset Optional if the viewport is not set up.
  std::optional<vec3> ClipFromPixel(const vec2& screen_pixel) const;

  // Converts a point in clip space to a pixel.  If |clip_points|'s
  // values are in the range ([-1,1], [-1,1], [0, 1]), the pixel will be in the
  // viewport.  Returns an unset Optional if the viewport is not set up.
  std::optional<vec2> PixelFromClip(const vec3& clip_point) const;

  // Converts a pixel to a Uv coordinate.  If the pixel is inside the viewport,
  // the result will be in the range [0, 1).  Returns an unset Optional if the
  // viewport is not set up.
  std::optional<vec2> UvFromPixel(const vec2& pixel) const;

  // Converts a uv point in camera texture space to a pixel.  If the uv
  // coordinate is in the range [0, 1), the pixel will be inside the viewport.
  // Returns an unset Optional if the viewport is not set up.
  std::optional<vec2> PixelFromUv(const vec2& uv) const;

  // Converts a uv point in camera texture space to clip space.  If the uv
  // is in the range [0, 1), the result will be in the range ([-1,1], [-1,1],
  // 0). This will flip the Y value, since texture space has y == 0 as the top,
  // and clip space has y == 1 as the top.
  static vec3 ClipFromUv(const vec2& uv);

  // Converts a point in clip space to camera texture space.  If the point is in
  // the range ([-1,1], [-1,1], 0), the result will be in the range  [0, 1).
  // This will flip the Y value, since texture space has y == 0 as the top, and
  // clip space has y == 1 as the top.
  static vec2 UvFromClip(const vec3& clip_point);

 private:
  vec3 position_;
  quat rotation_;
  mat4 clip_from_camera_;  // aka projection
  Bounds2i viewport_;
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_CAMERA_OPS_H_
