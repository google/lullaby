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

#include "redux/modules/graphics/camera_ops.h"

#include "redux/modules/math/transform.h"

namespace redux {

CameraOps::CameraOps(const vec3& position, const quat& rotation,
                     const mat4& projection, const Bounds2i& viewport)
    : position_(position),
      rotation_(rotation),
      clip_from_camera_(projection),
      viewport_(viewport) {}

const vec3& CameraOps::WorldPosition() const { return position_; }

const quat& CameraOps::WorldRotation() const { return rotation_; }

const Bounds2i& CameraOps::Viewport() const { return viewport_; }

mat4 CameraOps::ClipFromWorld() const {
  return ClipFromCamera() * CameraFromWorld();
}

mat4 CameraOps::WorldFromClip() const { return ClipFromWorld().Inversed(); }

mat4 CameraOps::ClipFromCamera() const { return clip_from_camera_; }

mat4 CameraOps::CameraFromClip() const { return clip_from_camera_.Inversed(); }

mat4 CameraOps::CameraFromWorld() const {
  return TransformMatrix(position_, rotation_, vec3::One());
}

mat4 CameraOps::WorldFromCamera() const { return CameraFromWorld().Inversed(); }

Ray CameraOps::WorldRayFromClipPoint(const vec3& clip_point) const {
  // Note: z value here doesn't matter as long as you divide by w.
  vec4 end = WorldFromClip() * vec4(clip_point.x, clip_point.y, 1.0f, 1.0f);
  end = end / end.w;
  const vec3 direction = (vec3(end.x, end.y, end.z) - position_).Normalized();
  return Ray(position_, direction);
}

Ray CameraOps::WorldRayFromUv(const vec2& uv) const {
  return WorldRayFromClipPoint(ClipFromUv(uv));
}

std::optional<Ray> CameraOps::WorldRayFromPixel(const vec2& pixel) const {
  std::optional<vec3> clip = ClipFromPixel(pixel);
  return clip ? WorldRayFromClipPoint(*clip) : std::optional<Ray>();
}

std::optional<vec2> CameraOps::PixelFromWorldPoint(
    const vec3& world_point) const {
  return PixelFromClip(ClipFromWorldPoint(world_point));
}

vec3 CameraOps::WorldPointFromClip(const vec3& clip_point) const {
  return WorldFromClip() * clip_point;
}

vec3 CameraOps::ClipFromWorldPoint(const vec3& world_point) const {
  return ClipFromWorld() * world_point;
}

vec2 CameraOps::UvFromWorldPoint(const vec3& world_point) const {
  return UvFromClip(ClipFromWorldPoint(world_point));
}

std::optional<vec3> CameraOps::ClipFromPixel(const vec2& pixel) const {
  std::optional<vec2> uv = UvFromPixel(pixel);
  return uv ? ClipFromUv(*uv) : std::optional<vec3>();
}

std::optional<vec2> CameraOps::PixelFromClip(const vec3& clip_point) const {
  return PixelFromUv(UvFromClip(clip_point));
}

std::optional<vec2> CameraOps::UvFromPixel(const vec2& pixel) const {
  if (viewport_.Size().x <= 0 || viewport_.Size().y <= 0) {
    return std::optional<vec2>();
  }
  // Convert pixel to [0,1].
  return vec2((pixel.x - viewport_.min.x) / viewport_.Size().x,
              (pixel.y - viewport_.min.y) / viewport_.Size().y);
}

std::optional<vec2> CameraOps::PixelFromUv(const vec2& uv) const {
  if (viewport_.Size().x <= 0 || viewport_.Size().y <= 0) {
    return std::optional<vec2>();
  }
  // Convert from [0,1] to [pos, pos+size].
  return vec2(viewport_.min.x + uv.x * viewport_.Size().x,
              viewport_.min.y + uv.y * viewport_.Size().y);
}

vec3 CameraOps::ClipFromUv(const vec2& uv) {
  // Convert to [-1,1].  Also flip y, so that +y is up.
  return vec3(2.0f * (uv.x - 0.5f), -2.0f * (uv.y - 0.5f), 0.0f);
}

vec2 CameraOps::UvFromClip(const vec3& clip_point) {
  // Convert from [-1,1] to [0,1].
  // Also flip y, so that +y is down (0,0 is top left pixel)
  return vec2(0.5f + (clip_point.x * 0.5f), 0.5f - (clip_point.y * 0.5f));
}

}  // namespace redux
