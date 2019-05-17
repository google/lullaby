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

#include "lullaby/util/math.h"

#include <cmath>

#include "lullaby/util/logging.h"
#include "mathfu/io.h"

namespace lull {

std::ostream& operator<<(std::ostream& os, const lull::Sqt& sqt) {
  return os << "Sqt: S" << sqt.scale << " Q" << sqt.rotation.ToEulerAngles()
            << " T" << sqt.translation;
}

std::ostream& operator<<(std::ostream& os, const lull::Ray& ray) {
  return os << "Ray: dir" << ray.direction << " orig" << ray.origin;
}

std::ostream& operator<<(std::ostream& os, const lull::Plane& plane) {
  return os << "Plane: norm" << plane.normal << " orig"
            << (plane.distance * plane.normal);
}

std::ostream& operator<<(std::ostream& os, const lull::Aabb& aabb) {
  return os << "Aabb: min" << aabb.min << " max" << aabb.max;
}

std::ostream& operator<<(std::ostream& os, const lull::Sphere& sphere) {
  return os << "Sphere: position" << sphere.position << " radius"
            << sphere.radius;
}

mathfu::vec3 EvaluateCubicSpline(float t, const mathfu::vec3& control_point1,
                                 const mathfu::vec3& control_point2,
                                 const mathfu::vec3& control_point3,
                                 const mathfu::vec3& control_point4) {
  // We calculate the slope at control_point2 and control_point3, as we would in
  // conventional Catmull-Rom
  const mathfu::vec3 m0 = (control_point3 - control_point1) / 2.0f;
  const mathfu::vec3 m1 = (control_point4 - control_point2) / 2.0f;

  // However, since we want to interpolate between the last two control points,
  // we need to estimate the slope at control_point4. We can do this very
  // roughly by assuming a constant rate of change in the slopes of the control
  // points.
  const mathfu::vec3 m2 = m1 - m0 + m1;

  float t2 = t * t;
  float t3 = t2 * t;

  const mathfu::vec3 a = ((2.0f * t3) - (3.0f * t2) + 1.0f) * control_point3;
  const mathfu::vec3 b = (t3 - (2.0f * t2) + t) * m1;
  const mathfu::vec3 c = (((-2.0f * t3) + (3.0f * t2)) * control_point4);
  const mathfu::vec3 d = (t3 - t2) * m2;

  mathfu::vec3 pt = a + b + c + d;
  return pt;
}

mathfu::mat4 CalculateTransformMatrix(const mathfu::vec3& position,
                                      const mathfu::quat& rotation,
                                      const mathfu::vec3& scale) {
  return mathfu::mat4::Transform(position, rotation.ToMatrix(), scale);
}

mathfu::mat4 CalculateTransformMatrix(const Sqt& sqt) {
  return CalculateTransformMatrix(sqt.translation, sqt.rotation, sqt.scale);
}

mathfu::mat4 CalculateRelativeMatrix(const mathfu::mat4& world_to_a_matrix,
                                     const mathfu::mat4& world_to_b_matrix) {
  return world_to_a_matrix.Inverse() * world_to_b_matrix;
}

mathfu::mat4 CalculateCylinderDeformedTransformMatrix(
    const mathfu::mat4& undeformed_mat, const float deform_radius,
    const float clamp_angle) {
  mathfu::vec3 orig_pos = undeformed_mat.TranslationVector3D();
  mathfu::mat4 result =
      mathfu::mat4::FromTranslationVector(-1.f * orig_pos) * undeformed_mat;

  const float self_radius = std::abs(deform_radius - orig_pos.z);
  float self_angle = -orig_pos.x / deform_radius;
  if (clamp_angle > kDefaultEpsilon) {
    self_angle = mathfu::Clamp(self_angle, -clamp_angle, clamp_angle);
  }

  mathfu::quat rot = mathfu::quat::FromAngleAxis(self_angle, mathfu::kAxisY3f);
  mathfu::vec3 pos(-sinf(self_angle) * self_radius, orig_pos.y,
                   -cosf(self_angle) * self_radius + deform_radius);

  return mathfu::mat4::FromTranslationVector(pos) * rot.ToMatrix4() * result;
}

mathfu::mat4 CalculateCylinderUndeformedTransformMatrix(
    const mathfu::mat4& deformed_mat, const float deform_radius,
    const float clamp_angle) {
  mathfu::vec3 deformed_pos = deformed_mat.TranslationVector3D();
  mathfu::mat4 original_rotation =
      mathfu::mat4::FromTranslationVector(-1.f * deformed_pos) * deformed_mat;

  // Calc angle from axis to deformed_pos.
  float angle = atan2f(deformed_pos.x, (deform_radius - deformed_pos.z));

  mathfu::vec3 undeformed_pos;
  if (clamp_angle > kDefaultEpsilon && std::abs(angle) > clamp_angle) {
    // Deformed points should stop at the clamp angle.  For points beyond that
    // angle, calculate the closest point on the vertical plane defined by the
    // clamp angle.
    float normal_angle = 0.0f;
    if (angle > 0) {
      normal_angle = clamp_angle + kPi / 2.0f;
    } else {
      normal_angle = -clamp_angle - kPi / 2.0f;
    }
    const mathfu::vec3 normal(sinf(normal_angle), 0, -cosf(normal_angle));
    const Plane clamp_plane(1.0f * mathfu::kAxisZ3f * deform_radius, normal);

    deformed_pos = ProjectPointOntoPlane(clamp_plane, deformed_pos);
    angle = clamp_angle;
  }
  // UndeformPoint assumes 0,0,0 is on the axis of the cylinder, not the
  // surface of it.
  undeformed_pos =
      UndeformPoint(deformed_pos - mathfu::kAxisZ3f * deform_radius,
                    deform_radius) +
      mathfu::kAxisZ3f * deform_radius;

  mathfu::quat rot = mathfu::quat::FromAngleAxis(angle, mathfu::kAxisY3f);
  return mathfu::mat4::FromTranslationVector(undeformed_pos) * rot.ToMatrix4() *
         original_rotation;
}

mathfu::mat4 CalculateLookAtMatrixFromDir(const mathfu::vec3& eye,
                                          const mathfu::vec3& dir,
                                          const mathfu::vec3& up) {
  if (mathfu::vec3::CrossProduct(dir, up).LengthSquared() < kDefaultEpsilon) {
    DLOG(ERROR)
        << "CalculateLookAtMatrixFromDir received front and up vectors that"
        << " have either zero length or are parallel to each other. "
        << "[dir: " << dir << " up: " << up << "]";
    return mathfu::mat4::Identity();
  }

  const mathfu::vec3 front = dir.Normalized();
  const mathfu::vec3 right = mathfu::vec3::CrossProduct(front, up).Normalized();
  const mathfu::vec3 new_up =
      mathfu::vec3::CrossProduct(right, front).Normalized();
  mathfu::mat4 mat(right[0], new_up[0], -front[0], 0, right[1], new_up[1],
                   -front[1], 0, right[2], new_up[2], -front[2], 0, 0, 0, 0, 1);

  return mat * mathfu::mat4::FromTranslationVector(-eye);
}

mathfu::mat4 CalculatePerspectiveMatrixFromFrustum(float x_left, float x_right,
                                                   float y_bottom, float y_top,
                                                   float z_near, float z_far) {
  if (AreNearlyEqual(x_left, x_right) || AreNearlyEqual(y_bottom, y_top) ||
      AreNearlyEqual(z_near, z_far) || z_near <= 0 || z_far <= 0) {
    DLOG(ERROR)
        << "CalculatePerspectiveMatrixFromFrustum received invalid frustum"
        << " dimensions. Defaulting to the identity matrix.";
    return mathfu::mat4::Identity();
  }

  const float x = (2 * z_near) / (x_right - x_left);
  const float y = (2 * z_near) / (y_top - y_bottom);
  const float a = (x_right + x_left) / (x_right - x_left);
  const float b = (y_top + y_bottom) / (y_top - y_bottom);
  const float c = (z_near + z_far) / (z_near - z_far);
  const float d = (2 * z_near * z_far) / (z_near - z_far);

  return mathfu::mat4(x, 0, 0, 0, 0, y, 0, 0, a, b, c, -1, 0, 0, d, 0);
}

mathfu::mat4 CalculatePerspectiveMatrixFromView(float fovy, float aspect,
                                                float z_near, float z_far) {
  if (fovy <= 0 || aspect <= 0 || z_near <= 0 || z_far <= 0 ||
      AreNearlyEqual(z_near, z_far)) {
    DLOG(ERROR) << "CalculatePerspectiveMatrixFromView received invalid view"
                << " parameters. Defaulting to the identity matrix.";
    return mathfu::mat4::Identity();
  }

  const float tan_fov = static_cast<float>(std::tan(fovy / 2)) * z_near;
  const float x_left = -tan_fov * aspect;
  const float x_right = tan_fov * aspect;
  const float y_bottom = -tan_fov;
  const float y_top = tan_fov;
  return CalculatePerspectiveMatrixFromFrustum(x_left, x_right, y_bottom, y_top,
                                               z_near, z_far);
}

mathfu::mat4 CalculatePerspectiveMatrixFromView(const mathfu::rectf& fov,
                                                float z_near, float z_far) {
  const float x_left = -tanf(fov.pos[0]) * z_near;
  const float x_right = tanf(fov.pos[1]) * z_near;
  const float y_bottom = -tanf(fov.size[0]) * z_near;
  const float y_top = tanf(fov.size[1]) * z_near;
  return CalculatePerspectiveMatrixFromFrustum(x_left, x_right, y_bottom, y_top,
                                               z_near, z_far);
}

mathfu::mat3 ComputeNormalMatrix(const mathfu::mat4& mat) {
  // Compute the normal matrix. This is the transposed matrix of the inversed
  // world position. This is done to avoid non-uniform scaling of the normal.
  // A good explanation of this can be found here:
  // http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
  return mathfu::mat4::ToRotationMatrix(mat).Inverse().Transpose();
}

mathfu::vec3 CalculateCameraDirection(const mathfu::mat4& eye_matrix) {
  return -GetMatrixColumn3D(eye_matrix, 2);
}

float CalculateDeterminant3x3(const mathfu::mat4& m) {
  const float sub11 = m[5] * m[10] - m[6] * m[9];
  const float sub12 = m[1] * m[10] - m[2] * m[9];
  const float sub13 = m[1] * m[6] - m[2] * m[5];
  return (m[0] * sub11 - m[4] * sub12 + m[8] * sub13);
}

Sqt CalculateSqtFromMatrix(const mathfu::mat4* mat) {
  return mat == nullptr ? Sqt() : CalculateSqtFromMatrix(*mat);
}

Sqt CalculateSqtFromMatrix(const mathfu::mat4& mat) {
  const mathfu::vec3 c0(mat(0, 0), mat(1, 0), mat(2, 0));
  const mathfu::vec3 c1(mat(0, 1), mat(1, 1), mat(2, 1));
  const mathfu::vec3 c2(mat(0, 2), mat(1, 2), mat(2, 2));
  const float scale_x = c0.Length();
  const float scale_y = c1.Length();
  const float scale_z = c2.Length();
  const mathfu::mat3 rot(c0.x / scale_x, c0.y / scale_x, c0.z / scale_x,
                         c1.x / scale_y, c1.y / scale_y, c1.z / scale_y,
                         c2.x / scale_z, c2.y / scale_z, c2.z / scale_z);
  Sqt result(mat.TranslationVector3D(), mathfu::quat::FromMatrix(rot),
             mathfu::vec3(scale_x, scale_y, scale_z));
  return result;
}

Sqt CalculateSqtFromAffineTransform(const mathfu::AffineTransform& mat) {
  return CalculateSqtFromMatrix(mathfu::mat4::FromAffineTransform(mat));
}

mathfu::mat4 CalculateRotateAroundMatrix(const mathfu::vec3& point,
                                         const mathfu::vec3& axis,
                                         float angle) {
  const mathfu::quat rotation = mathfu::quat::FromAngleAxis(angle, axis);
  return mathfu::mat4::FromTranslationVector(point) * rotation.ToMatrix4() *
         mathfu::mat4::FromTranslationVector(-point);
}

mathfu::quat FromEulerAnglesYXZ(const mathfu::vec3& euler) {
  const mathfu::quat X = mathfu::quat::FromAngleAxis(euler.x, mathfu::kAxisX3f);
  const mathfu::quat Y = mathfu::quat::FromAngleAxis(euler.y, mathfu::kAxisY3f);
  const mathfu::quat Z = mathfu::quat::FromAngleAxis(euler.z, mathfu::kAxisZ3f);

  return Y * X * Z;
}

float GetPitchRadians(const mathfu::quat& rotation) {
  // Apply the rotation to the negative z-axis to get the rotated direction.
  mathfu::vec3 gaze = rotation * -mathfu::kAxisZ3f;

  // Use the resulting y value to calculate the pitch.
  return asinf(gaze.y);
}

float GetHeadingRadians(const mathfu::quat& rotation) {
  // Apply the rotation to the negative z-axis to get the rotated direction.
  mathfu::vec3 gaze = rotation * -mathfu::kAxisZ3f;

  if (AreNearlyEqual(1.0f, gaze.y)) {
    // When the gaze is almost directly up, we use the negative y vector to
    // calculate heading (which way is your chin pointed).
    gaze = rotation * -mathfu::kAxisY3f;
  } else if (AreNearlyEqual(-1.0f, gaze.y)) {
    // When the gaze is almost directly down, we use the positive y vector to
    // calculate heading (which way the top of your head points).
    gaze = rotation * mathfu::kAxisY3f;
  }

  // Get the angle on the x-z plane. Note that relative to normal atan2, the
  // -Z axis in 3d space corresponds to x, and the -X axis corresponds to y.
  return atan2f(-gaze.x, -gaze.z);
}

float GetYawFromQuat(const mathfu::quat& q) {
  mathfu::vec3 dir = q * -mathfu::kAxisZ3f;
  float yaw = std::atan2(dir[2], dir[0]);
  if (!std::isfinite(yaw)) {
    return 0.0f;
  }
  return yaw;
}

Sqt GetHeading(const Sqt& sqt) {
  const float heading_radians = GetHeadingRadians(sqt.rotation);

  // Construct a new Sqt representing with only a rotation around the Y-axis.
  const mathfu::quat updated_rotation =
      mathfu::quat::FromAngleAxis(heading_radians, mathfu::kAxisY3f);
  return Sqt(sqt.translation, updated_rotation, sqt.scale);
}

mathfu::vec3 ProjectPositionToVicinity(const mathfu::vec3& pos,
                                       const mathfu::vec3& target,
                                       float max_offset) {
  if (max_offset < kDefaultEpsilon) {
    return target;
  }

  const mathfu::vec3 target_to_pos = pos - target;
  const float dist_sqr = target_to_pos.LengthSquared();
  if (dist_sqr < max_offset * max_offset) {
    return pos;
  }

  return target + target_to_pos * (max_offset / std::sqrt(dist_sqr));
}

mathfu::quat ProjectRotationToVicinity(const mathfu::quat& rot,
                                       const mathfu::quat& target,
                                       float max_offset_rad) {
  if (max_offset_rad < kDefaultEpsilon) {
    return target;
  }

  float angle = 0.f;
  mathfu::vec3 axis;
  const mathfu::quat rot_to_target = rot.Inverse() * target;
  rot_to_target.ToAngleAxis(&angle, &axis);

  if (angle < max_offset_rad) {
    return rot;
  }

  angle -= max_offset_rad;

  return rot * mathfu::quat::FromAngleAxis(angle, axis);
}

Ray CalculateRayFromCamera(const mathfu::vec3& camera_pos,
                           const mathfu::mat4& inverse_view_projection_mat,
                           const mathfu::vec2& point) {
  const mathfu::vec3 start = camera_pos;
  // Note: z value here doesn't matter as long as you divide by w.
  mathfu::vec4 end =
      inverse_view_projection_mat * mathfu::vec4(point.x, point.y, 1.0f, 1.0f);
  end = end / end.w;
  const mathfu::vec3 direction =
      (mathfu::vec3(end.x, end.y, end.z) - start).Normalized();
  return Ray(start, direction);
}

Ray CalculateRayFromCamera(const mathfu::vec3& camera_pos,
                           const mathfu::quat& camera_rot,
                           const mathfu::mat4& inverse_projection_mat,
                           const mathfu::vec2& point) {
  // Calculate the inverse view matrix.
  const mathfu::mat4 world_from_camera = mathfu::mat4::Transform(
      camera_pos, camera_rot.ToMatrix(), mathfu::kOnes3f);
  return CalculateRayFromCamera(
      camera_pos, world_from_camera * inverse_projection_mat, point);
}

Ray TransformRay(const mathfu::mat4& mat, const Ray& ray) {
  // Extend ray.direction with a fourth homogeneous coordinate of 0 in order to
  // perform a vector-like transformation rather than a point-like
  // transformation.
  return Ray(mat * ray.origin, (mat * mathfu::vec4(ray.direction, 0.0f)).xyz());
}

Ray NegativeZAxisRay(const Sqt& sqt) {
  return Ray(sqt.translation, sqt.rotation * -mathfu::kAxisZ3f);
}

float CosAngleFromRay(const Ray& ray, const mathfu::vec3& point) {
  return mathfu::vec3::DotProduct((point - ray.origin).Normalized(),
                                  ray.direction.Normalized());
}

float ProjectPointOntoRay(const Ray& ray, const mathfu::vec3 point) {
  return mathfu::vec3::DotProduct(point - ray.origin,
                                  ray.direction.Normalized());
}

float CheckRayTriangleCollision(const Ray& ray, const Triangle& triangle) {
  // Möller–Trumbore intersection.
  const mathfu::vec3 edge12 = triangle.v2 - triangle.v1;
  const mathfu::vec3 edge13 = triangle.v3 - triangle.v1;

  const mathfu::vec3 r = mathfu::cross(ray.direction, edge13);
  const float det = mathfu::dot(edge12, r);
  if (IsNearlyZero(det)) {
    // The ray is parallel to the triangle plane.
    return kNoHitDistance;
  }

  const float inv_det = 1.f / det;

  const mathfu::vec3 p = ray.origin - triangle.v1;
  const float u = mathfu::dot(p, r) * inv_det;
  if (u < 0.f || u > 1.f) {
    return kNoHitDistance;
  }

  const mathfu::vec3 q = mathfu::cross(p, edge12);
  const float v = mathfu::dot(ray.direction, q) * inv_det;
  if (v < 0.f || u + v > 1.f) {
    return kNoHitDistance;
  }

  const float t = dot(edge13, q) * inv_det;
  if (IsNearlyZero(t)) {
    return kNoHitDistance;
  }

  return t;
}

mathfu::vec3 DeformPoint(const mathfu::vec3& point, float radius) {
  // The farther a point is from the yz-plane, the more it will be wrapped
  // around. Calculate the number of revolutions (in radians) the line of length
  // |point.x| would reach around a circle of radius |radius|.
  const float angle = point.x / radius;
  // Wrap the point by that number of revolutions onto the vertical cylinder
  // about the space's origin with radius |point.z|.
  return mathfu::vec3(-point.z * sinf(angle), point.y, point.z * cosf(angle));
}

mathfu::vec3 UndeformPoint(const mathfu::vec3& point, float radius) {
  const float angle = atan2f(point.x, -point.z);
  const float cos_angle = cosf(angle);
  // There is a numerical instability where cos(angle) is close to 0.
  // In those cases, we should recover the z from the sin(angle) instead.
  float z;
  if (std::abs(cos_angle) > kDefaultEpsilon) {
    z = point.z / cos_angle;
  } else {
    z = -point.x / sinf(angle);
  }

  return mathfu::vec3(angle * radius, point.y, z);
}

bool ComputeLocalRayAABBCollision(const Ray& ray, const Aabb& aabb,
                                  bool collision_on_exit, mathfu::vec3* out) {
  float tmin = -1 * std::numeric_limits<float>::infinity();
  float tmax = std::numeric_limits<float>::infinity();

  // Second, run a fast AABB collision algorithm (Slab method).
  // Checking where the ray intersects the x planes:

  if (ray.direction.x != 0.f) {
    const float tx1 = (aabb.min.x - ray.origin.x) / ray.direction.x;
    const float tx2 = (aabb.max.x - ray.origin.x) / ray.direction.x;

    tmin = std::min<float>(tx1, tx2);
    tmax = std::max<float>(tx1, tx2);
  } else if (ray.origin.x > aabb.max.x || ray.origin.x < aabb.min.x) {
    return false;
  }

  // Check if the ray intersects the y planes inside the range it intersects the
  // x planes:
  if (ray.direction.y != 0.f) {
    const float ty1 = (aabb.min.y - ray.origin.y) / ray.direction.y;
    const float ty2 = (aabb.max.y - ray.origin.y) / ray.direction.y;

    tmin = std::max<float>(tmin, std::min<float>(ty1, ty2));
    tmax = std::min<float>(tmax, std::max<float>(ty1, ty2));
  } else if (ray.origin.y > aabb.max.y || ray.origin.y < aabb.min.y) {
    return false;
  }

  // Early exit if the region the ray overlaps the y planes is outside the
  // region the ray overlaps the x planes.
  if (tmax < tmin) {
    return false;
  }

  // Check if the ray intersects the z planes inside the range it intersects the
  // x and y planes:
  if (ray.direction.z != 0.f) {
    const float tz1 = (aabb.min.z - ray.origin.z) / ray.direction.z;
    const float tz2 = (aabb.max.z - ray.origin.z) / ray.direction.z;

    tmin = std::max<float>(tmin, std::min<float>(tz1, tz2));
    tmax = std::min<float>(tmax, std::max<float>(tz1, tz2));
  } else if (ray.origin.z > aabb.max.z || ray.origin.z < aabb.min.z) {
    return false;
  }

  if (tmax < tmin) {
    return false;
  }

  if (tmin < 0) {
    if (tmax < 0) {
      return false;
    } else {
      // Bounding box encloses ray origin, so return the distance to where the
      // ray exits the box.
      tmin = tmax;
    }
  }

  if (out) {
    *out = ray.origin + ray.direction * (collision_on_exit ? tmax : tmin);
  }
  return true;
}

bool ComputeLocalRayOBBCollision(const Ray& ray, const mathfu::mat4& world_mat,
                                 const Aabb& aabb, bool collision_on_exit,
                                 mathfu::vec3* out) {
  // First transform the ray into the OBB's space.
  mathfu::mat4 inverse_world_mat;
  bool invertible_world_mat = world_mat.InverseWithDeterminantCheck(
      &inverse_world_mat, kDeterminantThreshold);
  if (!invertible_world_mat) {
    return false;
  }
  const Ray local = TransformRay(inverse_world_mat, ray);
  return ComputeLocalRayAABBCollision(local, aabb, collision_on_exit, out);
}

float CheckRayAABBCollision(const Ray& ray, const Aabb& aabb,
                            bool collision_on_exit) {
  mathfu::vec3 local_collision;
  if (!ComputeLocalRayAABBCollision(ray, aabb, collision_on_exit,
                                    &local_collision)) {
    return kNoHitDistance;
  }
  return (local_collision - ray.origin).Length();
}

float CheckRayOBBCollision(const Ray& ray, const mathfu::mat4& world_mat,
                           const Aabb& aabb, bool collision_on_exit) {
  mathfu::vec3 local_collision;
  if (!ComputeLocalRayOBBCollision(ray, world_mat, aabb, collision_on_exit,
                                   &local_collision)) {
    return kNoHitDistance;
  }
  // The Mathfu Mat4x4 * Vec3 code includes code for xyz() / w(). That should
  // never be needed when dealing with world matrices, so using the
  // Mat4x4 * Vec4 function is safer and saves us 3 divides.
  const mathfu::vec3 world_collision =
      (world_mat * mathfu::vec4(local_collision, 1.0f)).xyz();
  return (world_collision - ray.origin).Length();
}

bool CheckPointOBBCollision(const mathfu::vec3& point,
                            const mathfu::mat4& world_from_object_matrix,
                            const Aabb& aabb) {
  // First transform the point into the OBB's space.
  const mathfu::vec3 local = world_from_object_matrix.Inverse() * point;
  return CheckPointAABBCollision(local, aabb);
}

bool CheckPointAABBCollision(const mathfu::vec3& point, const Aabb& aabb) {
  return (point.x >= aabb.min.x && point.x <= aabb.max.x &&
          point.y >= aabb.min.y && point.y <= aabb.max.y &&
          point.z >= aabb.min.z && point.z <= aabb.max.z);
}

mathfu::vec3 ProjectPointOntoPlane(const Plane& plane,
                                   const mathfu::vec3& point) {
  const mathfu::vec3 diff = point - plane.Origin();
  return point - dot(diff, plane.normal) * plane.normal;
}

bool ComputeRayPlaneCollision(const Ray& ray, const Plane& plane,
                              mathfu::vec3* out_hit, float* out_hit_distance) {
  const mathfu::vec3 origin_diff = plane.Origin() - ray.origin;
  float numerator = dot(origin_diff, plane.normal);
  float denominator = dot(ray.direction, plane.normal);
  if (IsNearlyZero(denominator)) {
    return false;  // ray is parallel to plane.
  }
  float t = numerator / denominator;
  if (t < -1.0 * kDefaultEpsilon) {
    return false;  // plane is behind the ray.
  }
  if (out_hit) {
    *out_hit = ray.origin + t * ray.direction.Normalized();
  }
  if (out_hit_distance) {
    *out_hit_distance = t;
  }
  return true;
}

bool ComputeRaySphereCollision(const Ray& ray, const mathfu::vec3& center,
                               const float radius, mathfu::vec3* out) {
  // Using algorithm adapted from:
  // http://www.lighthouse3d.com/tutorials/maths/ray-sphere-intersection/
  // First check the distance between the sphere and the line defined by the
  // ray.
  const mathfu::vec3 ray_to_sphere = center - ray.origin;
  const float rts_len_squared = ray_to_sphere.LengthSquared();
  const float rad_squared = radius * radius;
  const float dot_product = mathfu::dot(ray.direction, ray_to_sphere);
  if (dot_product < 0) {
    // Center of sphere is behind the ray.
    if (rts_len_squared > rad_squared) {
      // No intersection
      return false;
    } else if (rts_len_squared == rad_squared) {
      // Start of ray is on surface of sphere.
      *out = ray.origin;
      return true;
    } else {
      // Ray is inside sphere.
      const mathfu::vec3 closest_point = ProjectPointOntoLine(ray, center);
      const mathfu::vec3 center_to_closest = center - closest_point;
      const float dist_from_closest =
          std::sqrt(rad_squared - center_to_closest.LengthSquared());
      *out = closest_point + ray.direction * dist_from_closest;
      return true;
    }
  } else {
    // Center of sphere is in front of the ray origin.
    const mathfu::vec3 closest_point = ProjectPointOntoLine(ray, center);
    const mathfu::vec3 center_to_closest = center - closest_point;
    if (center_to_closest.LengthSquared() > rad_squared) {
      // Center of Sphere is more than radius away from the ray.
      return false;
    } else {
      const float dist_from_closest =
          std::sqrt(rad_squared - center_to_closest.LengthSquared());
      if (rts_len_squared > rad_squared) {
        // Origin of ray is outside sphere, take the first intersection.
        *out = closest_point - dist_from_closest * ray.direction;
      } else {
        // Origin is inside sphere, take the second intersection.
        *out = closest_point + dist_from_closest * ray.direction;
      }
      return true;
    }
  }
}

mathfu::vec3 ProjectPointOntoLine(const Line& line, const mathfu::vec3& point) {
  const Ray line_as_ray(line.origin, line.direction.Normalized());
  const float distance = ProjectPointOntoRay(line_as_ray, point);
  return line_as_ray.origin + distance * line_as_ray.direction;
}

bool ComputeClosestPointBetweenLines(const Line& line_a, const Line& line_b,
                                     mathfu::vec3* out_a, mathfu::vec3* out_b) {
  // Find the points along each line with minimum distance from each other.
  // See:
  // http://geomalgorithms.com/a07-_distance.html

  const mathfu::vec3 u_hat = line_a.direction.Normalized();
  const mathfu::vec3 v_hat = line_b.direction.Normalized();
  const mathfu::vec3 w_0 = line_b.origin - line_a.origin;
  const float b = dot(u_hat, v_hat);
  const float b_sqr = b * b;

  // Bail early if lines are parallel;
  if ((1.0f - b_sqr) < kDefaultEpsilon) {
    return false;
  }

  const float d = dot(u_hat, w_0);
  const float e = dot(v_hat, w_0);
  const float s = (d - e * b) / (1.0f - b_sqr);
  const float t = (d * b - e) / (1.0f - b_sqr);

  if (out_a != nullptr) {
    *out_a = line_a.origin + s * u_hat;
  }
  if (out_b != nullptr) {
    *out_b = line_b.origin + t * v_hat;
  }
  return true;
}

void CalculateViewFrustum(
    const mathfu::mat4& clip_from_world_matrix,
    mathfu::vec4 frustum_clipping_planes[kNumFrustumPlanes]) {
  // Extract the six planes (near, far, right, left, top and bottom) of the view
  // frustum from the perspective projection matrix.
  // See:
  // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
  // and: http://www.crownandcutlass.com/features/technicaldetails/frustum.html

  // Right clipping plane.
  frustum_clipping_planes[kRightFrustumPlane] =
      mathfu::vec4(clip_from_world_matrix[3] - clip_from_world_matrix[0],
                   clip_from_world_matrix[7] - clip_from_world_matrix[4],
                   clip_from_world_matrix[11] - clip_from_world_matrix[8],
                   clip_from_world_matrix[15] - clip_from_world_matrix[12]);
  // Left clipping plane.
  frustum_clipping_planes[kLeftFrustumPlane] =
      mathfu::vec4(clip_from_world_matrix[3] + clip_from_world_matrix[0],
                   clip_from_world_matrix[7] + clip_from_world_matrix[4],
                   clip_from_world_matrix[11] + clip_from_world_matrix[8],
                   clip_from_world_matrix[15] + clip_from_world_matrix[12]);
  // Bottom clipping plane.
  frustum_clipping_planes[kBottomFrustumPlane] =
      mathfu::vec4(clip_from_world_matrix[3] + clip_from_world_matrix[1],
                   clip_from_world_matrix[7] + clip_from_world_matrix[5],
                   clip_from_world_matrix[11] + clip_from_world_matrix[9],
                   clip_from_world_matrix[15] + clip_from_world_matrix[13]);
  // Top clipping plane.
  frustum_clipping_planes[kTopFrustumPlane] =
      mathfu::vec4(clip_from_world_matrix[3] - clip_from_world_matrix[1],
                   clip_from_world_matrix[7] - clip_from_world_matrix[5],
                   clip_from_world_matrix[11] - clip_from_world_matrix[9],
                   clip_from_world_matrix[15] - clip_from_world_matrix[13]);
  // Far clipping plane.
  frustum_clipping_planes[kFarFrustumPlane] =
      mathfu::vec4(clip_from_world_matrix[3] - clip_from_world_matrix[2],
                   clip_from_world_matrix[7] - clip_from_world_matrix[6],
                   clip_from_world_matrix[11] - clip_from_world_matrix[10],
                   clip_from_world_matrix[15] - clip_from_world_matrix[14]);
  // Near clipping plane.
  frustum_clipping_planes[kNearFrustumPlane] =
      mathfu::vec4(clip_from_world_matrix[3] + clip_from_world_matrix[2],
                   clip_from_world_matrix[7] + clip_from_world_matrix[6],
                   clip_from_world_matrix[11] + clip_from_world_matrix[10],
                   clip_from_world_matrix[15] + clip_from_world_matrix[14]);

  // Normalize the planes to allow calculating distance to points.
  for (int i = 0; i < kNumFrustumPlanes; i++) {
    frustum_clipping_planes[i] /=
        std::max(kDefaultEpsilon, frustum_clipping_planes[i].xyz().Length());
  }
}

bool CheckSphereInFrustum(
    const mathfu::vec3& center, const float radius,
    const mathfu::vec4 frustum_clipping_planes[kNumFrustumPlanes]) {
  // A sphere lies outside the frustum if its center is on the wrong side of
  // at least one plane and the distance to the plane is greater than the
  // radius of the sphere.
  // TODO This can be further optimized by un-rolling the loop and
  // returning the smallest distance to any frustum plane.
  for (int i = 0; i < kNumFrustumPlanes; i++) {
    // Calculate the signed distance of the center from the clipping plane.
    const mathfu::vec4& plane = frustum_clipping_planes[i];
    // TODO: Use Plane::SignedDistanceToPoint when implemented.
    const float distance =
        mathfu::vec4::DotProduct(plane, mathfu::vec4(center, 1));
    if (distance < -radius) {
      // Sphere lies outside the plane.
      return false;
    }
  }
  // Sphere lies on the inside of all planes.
  return true;
}

mathfu::vec2 EvalPointUvFromAabb(const Aabb& aabb, float x, float y) {
  const float width = aabb.max.x - aabb.min.x;
  const float height = aabb.max.y - aabb.min.y;
  if (width < kDefaultEpsilon || height < kDefaultEpsilon)
    return mathfu::kZeros2f;

  float u = (x - aabb.min.x) / width;
  float v = (y - aabb.min.y) / height;

  u = mathfu::Clamp(u, 0.f, 1.f);
  v = mathfu::Clamp(v, 0.f, 1.f);
  return mathfu::vec2(u, v);
}

float DampedDriveEase(float t) {
  if (t <= 0) {
    return 0;
  } else if (t >= 1) {
    return 1;
  }
  const float c0 = std::exp(t * std::log(1e-4f));
  return 1 - c0;
}

bool AreNearlyEqual(float a, float b, float epsilon /* = kDefaultEpsilon */) {
  return IsNearlyZero(a - b, epsilon);
}

bool IsNearlyZero(float n, float epsilon /* = kDefaultEpsilon */) {
  CHECK_GE(epsilon, 0);
  return (std::abs(n) < epsilon);
}

bool AreNearlyEqual(const mathfu::quat& one, const mathfu::quat& two,
                    float epsilon) {
  return std::abs(mathfu::quat::DotProduct(one, two)) > 1.f - epsilon;
}

bool AreNearlyEqual(const mathfu::vec4& one, const mathfu::vec4& two,
                    float epsilon) {
  for (int i = 0; i < 4; i++) {
    if (!AreNearlyEqual(one[i], two[i], epsilon)) {
      return false;
    }
  }
  return true;
}

bool AreNearlyEqual(const mathfu::vec3& one, const mathfu::vec3& two,
                    float epsilon) {
  for (int i = 0; i < 3; i++) {
    if (!AreNearlyEqual(one[i], two[i], epsilon)) {
      return false;
    }
  }
  return true;
}

bool AreNearlyEqual(const mathfu::vec2& one, const mathfu::vec2& two,
                    float epsilon) {
  for (int i = 0; i < 2; i++) {
    if (!AreNearlyEqual(one[i], two[i], epsilon)) {
      return false;
    }
  }
  return true;
}

bool AreNearlyEqual(const mathfu::vec2_packed& one,
                    const mathfu::vec2_packed& two, float epsilon) {
  for (int i = 0; i < 2; i++) {
    if (!AreNearlyEqual(one.data_[i], one.data_[i], epsilon)) {
      return false;
    }
  }
  return true;
}

bool AreNearlyEqual(const mathfu::mat4& one, const mathfu::mat4& two,
                    float epsilon) {
  for (int i = 0; i < 16; i++) {
    if (!AreNearlyEqual(one[i], two[i], epsilon)) {
      return false;
    }
  }
  return true;
}

bool AreNearlyEqual(const Aabb& one, const Aabb& two, float epsilon) {
  return AreNearlyEqual(one.min, two.min, epsilon) &&
         AreNearlyEqual(one.max, two.max, epsilon);
}

mathfu::vec3 GetMatrixColumn3D(const mathfu::mat4& mat, int index) {
  DCHECK(index >= 0 && index < 4);
  return mathfu::vec3(mat(0, index), mat(1, index), mat(2, index));
}

void GetTransformedBoxCorners(const Aabb& box, const mathfu::mat4& transform,
                              mathfu::vec3 out_corners[8]) {
  const mathfu::vec3 center = transform.TranslationVector3D();
  const mathfu::vec3 min_x = box.min.x * GetMatrixColumn3D(transform, 0);
  const mathfu::vec3 min_y = box.min.y * GetMatrixColumn3D(transform, 1);
  const mathfu::vec3 min_z = box.min.z * GetMatrixColumn3D(transform, 2);
  const mathfu::vec3 max_x = box.max.x * GetMatrixColumn3D(transform, 0);
  const mathfu::vec3 max_y = box.max.y * GetMatrixColumn3D(transform, 1);
  const mathfu::vec3 max_z = box.max.z * GetMatrixColumn3D(transform, 2);

  // Could optimize this to recognize flatness along each axis.
  out_corners[0] = center + min_x + min_y + min_z;
  out_corners[1] = center + min_x + min_y + max_z;
  out_corners[2] = center + min_x + max_y + min_z;
  out_corners[3] = center + min_x + max_y + max_z;
  out_corners[4] = center + max_x + min_y + min_z;
  out_corners[5] = center + max_x + min_y + max_z;
  out_corners[6] = center + max_x + max_y + min_z;
  out_corners[7] = center + max_x + max_y + max_z;
}

Aabb GetBoundingBox(const mathfu::vec3* points, int num_points) {
  CHECK(points && num_points > 0);

  Aabb box;
  box.min = box.max = points[0];

  for (int i = 1; i < num_points; ++i) {
    box.min = mathfu::vec3::Min(box.min, points[i]);
    box.max = mathfu::vec3::Max(box.max, points[i]);
  }

  return box;
}

Aabb TransformAabb(const Sqt& sqt, const Aabb& aabb) {
  return TransformAabb(CalculateTransformMatrix(sqt), aabb);
}

Aabb TransformAabb(const mathfu::mat4& transform, const Aabb& aabb) {
  mathfu::vec3 corners[8];
  GetTransformedBoxCorners(aabb, transform, corners);
  return GetBoundingBox(corners, sizeof(corners) / sizeof(corners[0]));
}

Aabb MergeAabbs(const Aabb& a, const Aabb& b) {
  Aabb merged;
  merged.min = mathfu::vec3::Min(a.min, b.min);
  merged.max = mathfu::vec3::Max(a.max, b.max);
  return merged;
}

mathfu::vec3 ProjectHomogeneous(const mathfu::vec4& a) { return a.xyz() / a.w; }

void FindPositionBetweenPoints(const float current_point,
                               const std::vector<float>& points,
                               size_t* min_index, size_t* max_index,
                               float* match_percent) {
  *max_index = 0;
  while (*max_index < points.size() && points[*max_index] < current_point) {
    *max_index += 1;
  }
  if (*max_index == 0) {
    *min_index = *max_index;
    *match_percent = 1.0f;
  } else if (*max_index == points.size()) {
    *max_index -= 1;
    *min_index = *max_index;
    *match_percent = 1.0f;
  } else {
    *min_index = *max_index - 1;
    *match_percent = (current_point - points[*min_index]) /
                     (points[*max_index] - points[*min_index]);
  }
}

float GetPercentageOfLineClosestToPoint(const mathfu::vec3& start_position,
                                        const mathfu::vec3& end_position,
                                        const mathfu::vec3& test_position) {
  const mathfu::vec3 line_diff = end_position - start_position;
  const float line_seg_sqr_length = line_diff.LengthSquared();
  const mathfu::vec3 line_to_point = test_position - start_position;
  const float dot_product = mathfu::vec3::DotProduct(line_diff, line_to_point);
  if (line_seg_sqr_length < kDefaultEpsilon) {
    return dot_product / kDefaultEpsilon;
  }
  return dot_product / line_seg_sqr_length;
}

std::vector<mathfu::vec3> GetAabbCorners(const Aabb& aabb) {
  // Do not reorder without checking client code.
  return {mathfu::vec3(aabb.min.x, aabb.min.y, aabb.min.z),
          mathfu::vec3(aabb.max.x, aabb.min.y, aabb.min.z),
          mathfu::vec3(aabb.min.x, aabb.max.y, aabb.min.z),
          mathfu::vec3(aabb.max.x, aabb.max.y, aabb.min.z),
          mathfu::vec3(aabb.min.x, aabb.min.y, aabb.max.z),
          mathfu::vec3(aabb.max.x, aabb.min.y, aabb.max.z),
          mathfu::vec3(aabb.min.x, aabb.max.y, aabb.max.z),
          mathfu::vec3(aabb.max.x, aabb.max.y, aabb.max.z)};
}

Aabb ScaledAabb(const Aabb& aabb, const mathfu::vec3& scale) {
  const mathfu::vec3 center = aabb.Center();
  return Aabb(scale * (aabb.min - center) + center,
              scale * (aabb.max - center) + center);
}

bool PointInAabb(const mathfu::vec3& point, const Aabb& aabb) {
  return point.x >= aabb.min.x && point.x <= aabb.max.x &&
         point.y >= aabb.min.y && point.y <= aabb.max.y &&
         point.z >= aabb.min.z && point.z <= aabb.max.z;
}

bool AabbsIntersect(const Aabb& aabb1, const Aabb& aabb2) {
  return aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x &&
         aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y &&
         aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z;
}

static const mathfu::vec4 GetRow(const mathfu::mat4& mat, int row) {
  return mathfu::vec4(mat(row, 0), mat(row, 1), mat(row, 2), mat(row, 3));
}

static mathfu::vec4 NormalizeBoxPlane(const mathfu::vec4& p) {
  const float len = p.xyz().Length();
  const float recip_len = len > 0.0f ? 1.0f / len : 0.0f;
  return p * recip_len;
}

mathfu::mat4 GetBoxMatrix(const lull::Aabb& aabb, const mathfu::mat4& mat) {
  const mathfu::vec3 center = aabb.Center();
  const mathfu::vec3 extent = 0.5f * aabb.Size();
  const mathfu::mat4 scale_mat = mathfu::mat4::FromScaleVector(extent);
  const mathfu::mat4 trans_mat = mathfu::mat4::FromTranslationVector(center);
  return mat * trans_mat * scale_mat;
}

BoxPlanes GetBoxPlanes(const mathfu::mat4& world_to_box_mat) {
  const mathfu::vec4 axis0 = GetRow(world_to_box_mat, 0);
  const mathfu::vec4 axis1 = GetRow(world_to_box_mat, 1);
  const mathfu::vec4 axis2 = GetRow(world_to_box_mat, 2);
  const mathfu::vec4 axis3 = GetRow(world_to_box_mat, 3);

  BoxPlanes planes;
  planes.v[kFaceXN] = NormalizeBoxPlane(axis3 - axis0);
  planes.v[kFaceXP] = NormalizeBoxPlane(axis3 + axis0);
  planes.v[kFaceYN] = NormalizeBoxPlane(axis3 - axis1);
  planes.v[kFaceYP] = NormalizeBoxPlane(axis3 + axis1);
  planes.v[kFaceZN] = NormalizeBoxPlane(axis3 - axis2);
  planes.v[kFaceZP] = NormalizeBoxPlane(axis3 + axis2);
  return planes;
}

IntersectBoxResult IsObbInFrustum(const mathfu::mat4& obb_mat,
                                  const BoxPlanes& frustum_planes) {
  const mathfu::vec3 obb_center = obb_mat.GetColumn(3).xyz();
  const mathfu::vec3 obb_axis0 = obb_mat.GetColumn(0).xyz();
  const mathfu::vec3 obb_axis1 = obb_mat.GetColumn(1).xyz();
  const mathfu::vec3 obb_axis2 = obb_mat.GetColumn(2).xyz();

  // Check OBB against each frustum plane.
  size_t in_count = 0;
  for (const mathfu::vec4& plane : frustum_planes.v) {
    // Get the signed distance from the OBB center to the frustum plane.
    const mathfu::vec3 normal = plane.xyz();
    const float plane_dist =
        mathfu::vec3::DotProduct(obb_center, normal) + plane.w;

    // Choose the OBB corner with diagonal most aligned to this frustum plane
    // and get its distance from the OBB center projected onto the plane normal.
    // This acts as our determinant by comparing it with the distance from the
    // OBB center to the plane (plane_dist).
    const float x = std::abs(mathfu::vec3::DotProduct(obb_axis0, normal));
    const float y = std::abs(mathfu::vec3::DotProduct(obb_axis1, normal));
    const float z = std::abs(mathfu::vec3::DotProduct(obb_axis2, normal));
    const float corner_dist = x + y + z;

    // If the nearest corner of the OBB is outside the frustum plane, the OBB is
    // fully outside the frustum.
    if (corner_dist < -plane_dist) {
      return kIntersectBoxMiss;
    }

    // If the farthest corner of the OBB is inside the frustum plane, the OBB is
    // fully inside this plane.
    if (corner_dist < plane_dist) {
      ++in_count;
    }
  }

  // If the OBB is fully inside all 6 planes, it is fully inside the frustum.
  // * In the indefinite case, we could refine the result by testing the frustum
  //   against the OBB's bounding sphere or by reversing the box test.  Except
  //   in cases where the OBB is large relative to the frustum, it's rare for an
  //   indefinite result to be a miss, so this is likely overkill for culling.
  return in_count == kFaceCount ? kIntersectBoxHit : kIntersectBoxIndefinite;
}

float GetSignedAngle(const mathfu::vec3& v1, const mathfu::vec3& v2,
                     const mathfu::vec3& axis) {
  // Use slightly larger epsilon because LengthSquared() instead of Length().
  DCHECK(lull::AreNearlyEqual(axis.LengthSquared(), 1.0f, 2e-5f));

  // Project v1 and v2 to the plane defined by axis.
  const mathfu::vec3 pv1 = v1 - mathfu::vec3::DotProduct(v1, axis) * axis;
  const mathfu::vec3 pv2 = v2 - mathfu::vec3::DotProduct(v2, axis) * axis;

  // For a discussion of atan vs asin+acos, in a very similar context,
  // see Kahan pp 46-47 http://people.eecs.berkeley.edu/~wkahan/Mindless.pdf

  // Both these values are scaled by ||pv1|| * ||pv2||. Because atan2
  // only cares about the ratio of the arguments, we don't have to bother
  // removing the scaling.
  const float scaled_cos_angle = mathfu::vec3::DotProduct(pv1, pv2);
  const float scaled_sin_angle =
      mathfu::vec3::DotProduct(mathfu::vec3::CrossProduct(pv1, pv2), axis);
  return std::atan2(scaled_sin_angle, scaled_cos_angle);
}

float EulerDistance(const mathfu::vec3& a, const mathfu::vec3& b) {
  return std::fabs(a.x - b.x) + std::fabs(a.y - b.y) + std::fabs(a.z - b.z);
}

float EulerNormalize(float target, float value) {
  // Ensure the difference is slightly larger than pi to avoid infinite looping.
  while (fabs(target - value) > kPi + kDefaultEpsilon) {
    if (target < value) {
      value -= kTwoPi;
    } else {
      value += kTwoPi;
    }
  }
  return value;
}

mathfu::vec3 EulerFilter(const mathfu::vec3& value, const mathfu::vec3& prev) {
  // Filter the original |value| to be within pi of |prev|.
  const mathfu::vec3 filtered_value(EulerNormalize(prev.x, value.x),
                                    EulerNormalize(prev.y, value.y),
                                    EulerNormalize(prev.z, value.z));

  // Compute the "Euler flipped" equivalent of |filtered_values|.
  const mathfu::vec3 euler_flipped(
      EulerNormalize(prev.x, filtered_value.x + kPi),
      EulerNormalize(prev.y, filtered_value.y * -1.f + kPi),
      EulerNormalize(prev.z, filtered_value.z + kPi));

  // Return whichever is "closer" to |prev|.
  if (EulerDistance(filtered_value, prev) >
      EulerDistance(euler_flipped, prev)) {
    return euler_flipped;
  } else {
    return filtered_value;
  }
}

mathfu::vec4 OrientationForTbn(const mathfu::vec3& normal,
                               const mathfu::vec3& tangent) {
  const mathfu::vec3 bitangent = cross(normal, tangent);
  mathfu::mat3 tbn_mat;
  tbn_mat.data_[0] = tangent.Normalized();
  tbn_mat.data_[1] = bitangent.Normalized();
  tbn_mat.data_[2] = normal.Normalized();
  const mathfu::quat quat = mathfu::quat::FromMatrix(tbn_mat);
  return mathfu::vec4(quat.vector(), quat.scalar());
}

}  // namespace lull
