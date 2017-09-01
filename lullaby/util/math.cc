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

#include "lullaby/util/math.h"

#include <cmath>

#include "mathfu/io.h"
#include "lullaby/util/logging.h"

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
  const mathfu::mat3 rot = rotation.ToMatrix();
  mathfu::vec4 c0(rot(0, 0), rot(1, 0), rot(2, 0), 0);
  mathfu::vec4 c1(rot(0, 1), rot(1, 1), rot(2, 1), 0);
  mathfu::vec4 c2(rot(0, 2), rot(1, 2), rot(2, 2), 0);
  mathfu::vec4 c3(0, 0, 0, 1);
  c0 *= scale.x;
  c1 *= scale.y;
  c2 *= scale.z;
  c3[0] = position.x;
  c3[1] = position.y;
  c3[2] = position.z;
  return mathfu::mat4(c0, c1, c2, c3);
}

mathfu::mat4 CalculateTransformMatrix(const Sqt& sqt) {
  return CalculateTransformMatrix(sqt.translation, sqt.rotation, sqt.scale);
}

mathfu::mat4 CalculateRelativeMatrix(const mathfu::mat4& world_to_a_matrix,
                                     const mathfu::mat4& world_to_b_matrix) {
  return world_to_a_matrix.Inverse() * world_to_b_matrix;
}

mathfu::mat4 CalculateCylinderDeformedTransformMatrix(
    const Sqt& sqt, const float parent_radius, const float deform_radius) {
  // TODO(b/29100730) Remove this function
  const float self_radius = std::abs(parent_radius - sqt.translation.z);
  const float self_angle = -sqt.translation.x / deform_radius;

  mathfu::quat rot =
      sqt.rotation * mathfu::quat::FromAngleAxis(self_angle, mathfu::kAxisY3f);
  mathfu::vec3 pos(-sinf(self_angle) * self_radius, sqt.translation.y,
                   -cosf(self_angle) * self_radius + parent_radius);

  return CalculateTransformMatrix(pos, rot, sqt.scale);
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

  const float tan_fov = static_cast<float>(tan(fovy / 2)) * z_near;
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
  const float angle = point.x / radius;
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

bool ComputeLocalRayOBBCollision(const Ray& ray, const mathfu::mat4& world_mat,
                                 const Aabb& aabb, bool collision_on_exit,
                                 mathfu::vec3* out) {
  // First transform the ray into the OBB's space.
  const Ray local = TransformRay(world_mat.Inverse(), ray);

  float tmin = -1 * std::numeric_limits<float>::infinity();
  float tmax = std::numeric_limits<float>::infinity();

  // Second, run a fast AABB collision algorithm (Slab method).
  // Checking where the ray intersects the x planes:

  if (local.direction.x != 0.f) {
    const float tx1 = (aabb.min.x - local.origin.x) / local.direction.x;
    const float tx2 = (aabb.max.x - local.origin.x) / local.direction.x;

    tmin = std::min<float>(tx1, tx2);
    tmax = std::max<float>(tx1, tx2);
  } else if (local.origin.x > aabb.max.x || local.origin.x < aabb.min.x) {
    return false;
  }

  // Check if the ray intersects the y planes inside the range it intersects the
  // x planes:
  if (local.direction.y != 0.f) {
    const float ty1 = (aabb.min.y - local.origin.y) / local.direction.y;
    const float ty2 = (aabb.max.y - local.origin.y) / local.direction.y;

    tmin = std::max<float>(tmin, std::min<float>(ty1, ty2));
    tmax = std::min<float>(tmax, std::max<float>(ty1, ty2));
  } else if (local.origin.y > aabb.max.y || local.origin.y < aabb.min.y) {
    return false;
  }

  // Early exit if the region the ray overlaps the y planes is outside the
  // region the ray overlaps the x planes.
  if (tmax < tmin) {
    return false;
  }

  // Check if the ray intersects the z planes inside the range it intersects the
  // x and y planes:
  if (local.direction.z != 0.f) {
    const float tz1 = (aabb.min.z - local.origin.z) / local.direction.z;
    const float tz2 = (aabb.max.z - local.origin.z) / local.direction.z;

    tmin = std::max<float>(tmin, std::min<float>(tz1, tz2));
    tmax = std::min<float>(tmax, std::max<float>(tz1, tz2));
  } else if (local.origin.z > aabb.max.z || local.origin.z < aabb.min.z) {
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
    *out = local.origin + local.direction * (collision_on_exit ? tmax : tmin);
  }
  return true;
}

float CheckRayOBBCollision(const Ray& ray, const mathfu::mat4& world_mat,
                           const Aabb& aabb, bool collision_on_exit) {
  mathfu::vec3 local_collision;
  if (!ComputeLocalRayOBBCollision(ray, world_mat, aabb, collision_on_exit,
                                   &local_collision)) {
    return kNoHitDistance;
  }
  const mathfu::vec3 world_collision = world_mat * local_collision;
  return (world_collision - ray.origin).Length();
}

bool CheckPointOBBCollision(const mathfu::vec3& point,
                            const mathfu::mat4& world_from_object_matrix,
                            const Aabb& aabb) {
  // First transform the point into the OBB's space.
  const mathfu::vec3 local = world_from_object_matrix.Inverse() * point;
  return CheckPointOBBCollision(local, aabb);
}

bool CheckPointOBBCollision(const mathfu::vec3& point, const Aabb& aabb) {
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
                              mathfu::vec3* out) {
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
  if (out) {
    *out = ray.origin + t * ray.direction.Normalized();
  }
  return true;
}

mathfu::vec3 ProjectPointOntoLine(const Line& line, const mathfu::vec3& point) {
  const Ray line_as_ray (line.origin, line.direction.Normalized());
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
  // TODO(b/30736927) This can be further optimized by un-rolling the loop and
  // returning the smallest distance to any frustum plane.
  for (int i = 0; i < kNumFrustumPlanes; i++) {
    // Calculate the signed distance of the center from the clipping plane.
    const mathfu::vec4& plane = frustum_clipping_planes[i];
    // TODO(b/29824351): Use Plane::SignedDistanceToPoint when implemented.
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

Aabb GetBoundingBox(const float* vertex_data, size_t len, size_t stride) {
  if (len < 3) {
    return Aabb();
  }

  CHECK(vertex_data != nullptr);
  CHECK_GE(stride, 3);
  CHECK(len % stride == 0) << "array size must be a multiple of stride";
  Aabb box;
  // Use the first vertex as the min and max.
  box.min = box.max =
      mathfu::vec3(vertex_data[0], vertex_data[1], vertex_data[2]);

  // Skip the first vertex, then advance by stride.
  for (size_t i = stride; i < len; i += stride) {
    mathfu::vec3 p(vertex_data[i], vertex_data[i + 1], vertex_data[i + 2]);
    box.min = mathfu::vec3::Min(box.min, p);
    box.max = mathfu::vec3::Max(box.max, p);
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
}  // namespace lull
