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

#ifndef LULLABY_UTIL_MATH_H_
#define LULLABY_UTIL_MATH_H_

#include <limits>
#include <vector>

#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/span.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

using mathfu::kDegreesToRadians;
using mathfu::kPi;
using mathfu::kRadiansToDegrees;
static constexpr float kDefaultEpsilon = 1.0e-5f;
static constexpr float kDefaultEpsilonSqr = 1.0e-10f;
static constexpr float kTwoPi = 2.f * kPi;

// This must match motive::kInvalidBoneIdx = 255.
static constexpr uint8_t kInvalidBoneIdx = 0xff;
// This must match motive::kMaxNumBones = 254.
static constexpr uint8_t kMaxNumBones = 0xfe;

// Mathfu uses a large default for the determinant threshold that causes matrix
// inverse to fail for matrices of about 1/200 scale.  Use this to support
// matrices with 1/1000 scales.
static constexpr float kDeterminantThreshold = 1.0e-9f;  // (1/1000)^3

struct Sqt;
struct Ray;
struct Plane;
struct Aabb;
struct Sphere;

std::ostream& operator<<(std::ostream& os, const Sqt& sqt);
std::ostream& operator<<(std::ostream& os, const Ray& ray);
std::ostream& operator<<(std::ostream& os, const Plane& plane);
std::ostream& operator<<(std::ostream& os, const Aabb& aabb);
std::ostream& operator<<(std::ostream& os, const Sphere& sphere);

struct Sqt {
  Sqt()
      : translation(mathfu::kZeros3f),
        rotation(mathfu::quat::identity),
        scale(mathfu::kOnes3f) {}

  Sqt(const mathfu::vec3& translation, const mathfu::quat& rotation,
      const mathfu::vec3& scale)
      : translation(translation), rotation(rotation), scale(scale) {}

  mathfu::vec3 translation;
  mathfu::quat rotation;
  mathfu::vec3 scale;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&translation, ConstHash("translation"));
    archive(&rotation, ConstHash("rotation"));
    archive(&scale, ConstHash("scale"));
  }
};

struct Ray {
  Ray() : origin(mathfu::kZeros3f), direction(-mathfu::kAxisZ3f) {}
  Ray(const mathfu::vec3& origin, const mathfu::vec3& direction)
      : origin(origin), direction(direction) {}

  mathfu::vec3 origin;
  mathfu::vec3 direction;

  // Return the point at |t| distance along the Ray.
  mathfu::vec3 GetPointAt(float t) const { return origin + t * direction; }

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&origin, ConstHash("origin"));
    archive(&direction, ConstHash("direction"));
  }
};

/// A line is parameterized in the same way as a ray but is conceptually
/// different in that it extends infinitely in both directions from its origin.
using Line = Ray;

struct Plane {
  Plane(float distance, const mathfu::vec3& normal)
      : distance(distance), normal(normal) {}
  Plane(const mathfu::vec3& point, const mathfu::vec3& normal)
      : distance(dot(point, normal.Normalized())), normal(normal) {}

  float distance;  // distance from world origin along the normal
  mathfu::vec3 normal;

  mathfu::vec3 Origin() const { return distance * normal; }

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&distance, ConstHash("distance"));
    archive(&normal, ConstHash("normal"));
  }
};

struct Aabb {
  Aabb() : min(mathfu::kZeros3f), max(mathfu::kZeros3f) {}
  Aabb(const mathfu::vec3& min, const mathfu::vec3& max) : min(min), max(max) {}

  mathfu::vec3 min;
  mathfu::vec3 max;

  mathfu::vec3 Size() const { return max - min; }

  mathfu::vec3 Center() const { return (max + min) * 0.5f; }

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&min, ConstHash("min"));
    archive(&max, ConstHash("max"));
  }

  void ToArray(float* array) const {
    array[0] = min.x;
    array[1] = min.y;
    array[2] = min.z;
    array[3] = max.x;
    array[4] = max.y;
    array[5] = max.z;
  }

  bool operator==(const Aabb& other) const {
    return this->min == other.min && this->max == other.max;
  }

  bool operator!=(const Aabb& other) const { return !(*this == other); }
};

struct Sphere {
  Sphere() : position(mathfu::kZeros3f), radius(0.0f) {}
  Sphere(const mathfu::vec3& position, const float radius)
      : position(position), radius(radius) {}

  mathfu::vec3 position;
  float radius;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&position, ConstHash("position"));
    archive(&radius, ConstHash("radius"));
  }
};

struct Triangle {
  Triangle(const mathfu::vec3& v1, const mathfu::vec3& v2,
           const mathfu::vec3& v3)
      : v1(v1), v2(v2), v3(v3) {}

  mathfu::vec3 v1;
  mathfu::vec3 v2;
  mathfu::vec3 v3;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&v1, ConstHash("v1"));
    archive(&v2, ConstHash("v2"));
    archive(&v3, ConstHash("v3"));
  }
};

static const float kNoHitDistance = -1.f;

// Given 4 control points, calculate a point that is the given distance along
// the curve (implemented as a modified Catmull-Rom) between the third and
// fourth control points.
mathfu::vec3 EvaluateCubicSpline(float t, const mathfu::vec3& control_point1,
                                 const mathfu::vec3& control_point2,
                                 const mathfu::vec3& control_point3,
                                 const mathfu::vec3& control_point4);

mathfu::mat4 CalculateTransformMatrix(const mathfu::vec3& position,
                                      const mathfu::quat& rotation,
                                      const mathfu::vec3& scale);

mathfu::mat4 CalculateTransformMatrix(const Sqt& sqt);

// Calculate the relative a_to_b_matrix from two world matrices
mathfu::mat4 CalculateRelativeMatrix(const mathfu::mat4& world_to_a_matrix,
                                     const mathfu::mat4& world_to_b_matrix);

// Calculate a deformed version of a transform matrix. The angle of the
// deformation will be optionally clamped at the clamp_angle for positive
// values.
mathfu::mat4 CalculateCylinderDeformedTransformMatrix(
    const mathfu::mat4& undeformed_mat, const float deform_radius,
    const float clamp_angle = 0.0f);

// Inverse of CalculateCylinderDeformedTransformMatrix.  If the position is
// beyond the bounds set by clamp_angle, it will be moved to the nearest
// valid position.
mathfu::mat4 CalculateCylinderUndeformedTransformMatrix(
    const mathfu::mat4& deformed_mat, const float deform_radius,
    const float clamp_angle = 0.0f);

// Calculate a 4x4 viewing matrix based on the given camera parameters, which
// use a view direction rather than look at center point. If the parameters
// cannot form an orthonormal basis then this returns an identity matrix.
mathfu::mat4 CalculateLookAtMatrixFromDir(const mathfu::vec3& eye,
                                          const mathfu::vec3& dir,
                                          const mathfu::vec3& up);

// Calculate a 4x4 perspective projection matrix based on the given parameters,
// which follow the conventions of the old glFrustum() function. If there are
// any problems with the parameters (such as 0 sizes in any dimension or
// non-positive near or far values), this returns an identity matrix.
mathfu::mat4 CalculatePerspectiveMatrixFromFrustum(float x_left, float x_right,
                                                   float y_bottom, float y_top,
                                                   float z_near, float z_far);

// Calculate a 4x4 perspective projection matrix based on the given parameters,
// which follow the conventions of the gluPerspective() function. If there are
// any problems with the parameters (such as non-positive values or z_near
// equal to z_far), this returns an identity matrix.
mathfu::mat4 CalculatePerspectiveMatrixFromView(float fovy, float aspect,
                                                float z_near, float z_far);

mathfu::mat4 CalculatePerspectiveMatrixFromView(const mathfu::rectf& fov,
                                                float z_near, float z_far);

// Calculate and return the normal rotation matrix for a given matrix. The
// normal matrix ensures that the direction of the normals is preserved when
// non-uniform scaling is present.
mathfu::mat3 ComputeNormalMatrix(const mathfu::mat4& mat);

// Calculate and return the camera's direction. This is the vector the camera is
// looking at.
mathfu::vec3 CalculateCameraDirection(const mathfu::mat4& eye_matrix);

// Returns the identity SQT is |mat| is null.
Sqt CalculateSqtFromMatrix(const mathfu::mat4* mat);

Sqt CalculateSqtFromMatrix(const mathfu::mat4& mat);

Sqt CalculateSqtFromAffineTransform(const mathfu::AffineTransform& mat);

// Calculates a matrix to rotate |angle| radians around |axis| with respect to
// centered around |point|.
mathfu::mat4 CalculateRotateAroundMatrix(const mathfu::vec3& point,
                                         const mathfu::vec3& axis, float angle);

// Computes the quaternion representing the rotation by the given Euler angles
// using the Y * X * Z concatenation order.
// This order of contatenation (Y * X * Z) or (Yaw * Pitch * Roll) gives a good
// natural interaction when using Euler angles where if the user has yawed and
// then tries to roll, the camera will roll properly.  If simple X * Y * Z
// ordering is used, then if the user yaws say 90 degrees left, then tries to
// roll, they will pitch.  It is a manifestation of the old gimbal lock problem.
mathfu::quat FromEulerAnglesYXZ(const mathfu::vec3& euler);

// Calculates the pitch (y) angle of a rotation. Return value ranges from -PI/2
// to PI/2.
float GetPitchRadians(const mathfu::quat& rotation);

// Calculates the heading (yaw) angle of a rotation.
// Note that this is unstable if z-axis is pointing nearly straight up or down.
float GetHeadingRadians(const mathfu::quat& rotation);

// Constructs a new Sqt that has the pitch and roll rotations removed.
// This can be used to center an entity on the user's position and heading.
Sqt GetHeading(const Sqt& sqt);

// Returns closest point to |pos| in the |max_offset| vicinity of |target|.
mathfu::vec3 ProjectPositionToVicinity(const mathfu::vec3& pos,
                                       const mathfu::vec3& target,
                                       float max_offset);

// Returns closest rotation to |rot| in the |max_offset_rad| vicinity of
// |target|.
mathfu::quat ProjectRotationToVicinity(const mathfu::quat& rot,
                                       const mathfu::quat& target,
                                       float max_offset_rad);

// Creates a Ray in world space based on a point on a screen or camera texture.
// |point| values should be in the range (-1, 1), with (-1,-1) being the bottom
// left corner and (1,1) being top right.  (See Normalized Device Cooordinates
// in OpenGL docs for more info)
Ray CalculateRayFromCamera(const mathfu::vec3& camera_pos,
                           const mathfu::mat4& inverse_view_projection_mat,
                           const mathfu::vec2& point);
Ray CalculateRayFromCamera(const mathfu::vec3& camera_pos,
                           const mathfu::quat& camera_rot,
                           const mathfu::mat4& inverse_projection_mat,
                           const mathfu::vec2& point);


// Transform a ray representing a locus of points.
Ray TransformRay(const mathfu::mat4& mat, const Ray& ray);

// Constructs a ray that goes along the negative z-axis of a transform.
// For example, NegativeZAxisRay(hmd->GetHeadSqt()) gives the viewing direction.
Ray NegativeZAxisRay(const Sqt& sqt);

// Calculates the cosine of the angle from a point to a ray. The value will be
// +1 if the point is directly in front of the ray, and -1 if the point is
// directly behind it.
float CosAngleFromRay(const Ray& ray, const mathfu::vec3& point);

// Finds the distance from the ray origin to the point on the ray nearest to
// |point|.
float ProjectPointOntoRay(const Ray& ray, const mathfu::vec3 point);

float CheckRayTriangleCollision(const Ray& ray, const Triangle& triangle);

// Transforms a point in unwrapped 2.5d space to wrapped world space. This
// means that each constant-z plane in the original space is wrapped onto the
// vertical cylinder centered on the space's origin with radius z. |radius|
// indicates how "tightly" the wrapping should compact the planes onto the
// cylinders.
mathfu::vec3 DeformPoint(const mathfu::vec3& point, float radius);

// Inverts the DeformPoint function.
// Note that deform point is not completely invertible - this is accurate only
// if the original point before deformation didn't wrap around more than one
// cycle of the cylindrical deformation.
// It also assumes that the z coordinate before deformation was negative (in
// front of the user).
mathfu::vec3 UndeformPoint(const mathfu::vec3& point, float radius);

// Computes the local ray collision point.
// * The OBB world matrix is optional.  If unspecified, the ray intersects the
//   untransformed AABB.
bool ComputeLocalRayAABBCollision(const Ray& ray, const Aabb& aabb,
                                  bool collision_on_exit = false,
                                  mathfu::vec3* out = nullptr);
bool ComputeLocalRayOBBCollision(const Ray& ray, const mathfu::mat4& world_mat,
                                 const Aabb& aabb,
                                 bool collision_on_exit = false,
                                 mathfu::vec3* out = nullptr);

// Check if a ray intersects an OBB and return the distance along the ray to the
// point of intersection.  Returns kNoHitDistance if there is no intersection.
// * The OBB world matrix is optional.  If unspecified, the ray intersects the
//   untransformed AABB.
float CheckRayAABBCollision(const Ray& ray, const Aabb& aabb,
                            bool collision_on_exit = false);
float CheckRayOBBCollision(const Ray& ray, const mathfu::mat4& world_transform,
                           const Aabb& aabb, bool collision_on_exit = false);

// Returns true if a |point| lies within |aabb|. Transforms the point into local
// space prior to performing the check.
bool CheckPointOBBCollision(const mathfu::vec3& point,
                            const mathfu::mat4& world_from_object_matrix,
                            const Aabb& aabb);

// Returns true if a |point| lies within |aabb|.
bool CheckPointAABBCollision(const mathfu::vec3& point, const Aabb& aabb);

// Project |point| onto |plane|.
mathfu::vec3 ProjectPointOntoPlane(const Plane& plane,
                                   const mathfu::vec3& point);

// Compute the ray-plane collision in world space.
bool ComputeRayPlaneCollision(const Ray& ray, const Plane& plane,
                              mathfu::vec3* out_hit,
                              float* out_hit_distance = nullptr);

// Compute the first ray-sphere collision.
bool ComputeRaySphereCollision(const Ray& ray, const mathfu::vec3& center,
                               const float radius, mathfu::vec3* out);

// Project |point| onto |line| and return the position.
mathfu::vec3 ProjectPointOntoLine(const Line& line, const mathfu::vec3& point);

// Calculates the points along each line where the two lines are closest.
// Returns false if the lines are parallel; true otherwise.
bool ComputeClosestPointBetweenLines(const Line& line_a, const Line& line_b,
                                     mathfu::vec3* out_a, mathfu::vec3* out_b);

// Returns the 8 corners of the box. At least one client depends on the
// current order of the corners.
std::vector<mathfu::vec3> GetAabbCorners(const lull::Aabb& aabb);

// Scales the size of the AABB about its center.
lull::Aabb ScaledAabb(const lull::Aabb& aabb, const mathfu::vec3& scale);

inline lull::Aabb ScaledAabb(const lull::Aabb& aabb, float scale) {
  return ScaledAabb(aabb, mathfu::vec3(scale, scale, scale));
}

// Returns true if point is inside aabb.
bool PointInAabb(const mathfu::vec3& point, const lull::Aabb& aabb);

// Returns true if the two AABBs intersect.
bool AabbsIntersect(const lull::Aabb& aabb1, const lull::Aabb& aabb2);

enum BoxFace {
  kFaceXN,  // -X
  kFaceXP,  // +X
  kFaceYN,  // -Y
  kFaceYP,  // +Y
  kFaceZN,  // -Z
  kFaceZP,  // +Z
  kFaceCount
};

struct BoxPlanes {
  mathfu::vec4 v[kFaceCount];
};

enum IntersectBoxResult {
  kIntersectBoxHit,         // Definite intersection.
  kIntersectBoxMiss,        // Definite non-intersection.
  kIntersectBoxIndefinite,  // Hit is likely, but indefinite.
};

// Get the matrix transforming the unit-cube to OBB (i.e. combine AABB bounds
// with a transform matrix).
mathfu::mat4 GetBoxMatrix(const lull::Aabb& aabb, const mathfu::mat4& mat);

// Get the set of planes for each box face.
// * The box is described by the unit-cube transformed by box_to_world_mat. This
//   function takes the inverse of that matrix, world_to_box_mat (which also
//   happens to be the view-projection matrix in the case of a frustum box).
BoxPlanes GetBoxPlanes(const mathfu::mat4& world_to_box_mat);

// Check if an OBB overlaps a view frustum.
IntersectBoxResult IsObbInFrustum(const mathfu::mat4& obb_mat,
                                  const BoxPlanes& frustum_planes);

// Returns the signed angle from v1 to v2, about the given axis.
// Pass:
//   v1, v2 - non-zero vectors; these do not need to be unit length.
//   axis   - a unit length vector.
// Returns a result in [-pi, pi].
float GetSignedAngle(
    const mathfu::vec3& v1,
    const mathfu::vec3& v2,
    const mathfu::vec3& axis);

// Returns the yaw computed from the given quaternion. If the yaw is INF or NaN,
// then 0.0 is returned;
float GetYawFromQuat(const mathfu::quat& q);

// Enum for frustum clipping planes.
enum FrustumPlane {
  kRightFrustumPlane,
  kLeftFrustumPlane,
  kBottomFrustumPlane,
  kTopFrustumPlane,
  kFarFrustumPlane,
  kNearFrustumPlane,
  kNumFrustumPlanes
};

// Calculates frustum clipping planes from view projection matrix.
void CalculateViewFrustum(
    const mathfu::mat4& clip_from_world_matrix,
    mathfu::vec4 frustum_clipping_planes[kNumFrustumPlanes]);

// Returns true if a bounding sphere intersects the frustum clipping planes.
// The center of the sphere and frustum clipping planes are assumed to be in the
// same view space.
bool CheckSphereInFrustum(
    const mathfu::vec3& center, const float radius,
    const mathfu::vec4 frustum_clipping_planes[kNumFrustumPlanes]);

// Returns (x, y)'s uv coordinates in the XY plane of the given aabb.
// Clamped to [0, 1] if (x, y) is outside of the box.
mathfu::vec2 EvalPointUvFromAabb(const Aabb& aabb, float x, float y);

// Ease the value along a 1 - e^t curve.
//
//
// Nice properties of this ease function:
// 1. It feels physically plausible,
// 2. It is very smooth (in the class C^infinity).
// 3. The damping can be specified with the x's being position, velocity, or
//    acceleration, etc. while giving the same feel of motion.
//
// This implies a dampening force that is proportional to the velocity.  If
// x = position, v = x' = velocity , and a = v' = x''= acceleration, then
// F = ma = -kv
// mx'' = -kx'
// The solution to this differential equation is x = Ce^(-bt) + D, where t is
// time, b=k/m, and C and D are arbitrary constants.  Here D = x1 and
// C = (x0 - x1), so that we move from x0 to x1.
float DampedDriveEase(float t);

// Returns true if |a| is within |epsilon| of |b|.
bool AreNearlyEqual(float a, float b, float epsilon = kDefaultEpsilon);

// Returns true if |n| is within |epsilon| of 0.0.
bool IsNearlyZero(float n, float epsilon = kDefaultEpsilon);

// Returns true if |one| and |two| are nearly the same orientation. Note that
// this is different than two rotations being the same.
bool AreNearlyEqual(const mathfu::quat& one, const mathfu::quat& two,
                    float epsilon = kDefaultEpsilon);

// Returns true if every element of |one| is within the |epsilon| of the
// counterpart of |two|.
bool AreNearlyEqual(const mathfu::vec4& one, const mathfu::vec4& two,
                    float epsilon = kDefaultEpsilon);

// Returns true if every element of |one| is within the |epsilon| of the
// counterpart of |two|.
bool AreNearlyEqual(const mathfu::vec3& one, const mathfu::vec3& two,
                    float epsilon = kDefaultEpsilon);

// Returns true if every element of |one| is within the |epsilon| of the
// counterpart of |two|.
bool AreNearlyEqual(const mathfu::vec2& one, const mathfu::vec2& two,
                    float epsilon = kDefaultEpsilon);

// Returns true if every element of |one| is within the |epsilon| of the
// counterpart of |two|.
bool AreNearlyEqual(const mathfu::vec2_packed& one,
                    const mathfu::vec2_packed& two,
                    float epsilon = kDefaultEpsilon);

// Returns true if every element of |one| is within the |epsilon| of the
// counterpart of |two|.
bool AreNearlyEqual(const mathfu::mat4& one, const mathfu::mat4& two,
                    float epsilon = kDefaultEpsilon);

// Returns true if every element of |one| is within the |epsilon| of the
// counterpart of |two|.
bool AreNearlyEqual(const Aabb& one, const Aabb& two,
                    float epsilon = kDefaultEpsilon);

// Returns the |index|th 3D column vector of |mat|.
mathfu::vec3 GetMatrixColumn3D(const mathfu::mat4& mat, int index);

// Transforms the 8 corners of an axis-aligned bounding box.
void GetTransformedBoxCorners(const Aabb& box, const mathfu::mat4& transform,
                              mathfu::vec3 out_corners[8]);
inline void GetTransformedBoxCorners(const Aabb& box, const Sqt& sqt,
                                     mathfu::vec3 out_corners[8]) {
  GetTransformedBoxCorners(box, CalculateTransformMatrix(sqt), out_corners);
}

// Returns the 3D box that contains all the |points|.
Aabb GetBoundingBox(const mathfu::vec3* points, int num_points);

// Transforms an Aabb and recalculates a new Aabb around the transformed
// corners.
Aabb TransformAabb(const Sqt& transform, const Aabb& aabb);

// Transforms an Aabb and recalculates a new Aabb around the transformed
// corners.
Aabb TransformAabb(const mathfu::mat4& transform, const Aabb& aabb);

// Merge two Aabbs into a single one.
Aabb MergeAabbs(const Aabb& a, const Aabb& b);

// Returns the determinant of the upper left 3x3 of |m|.
// TODO move into mathfu.
float CalculateDeterminant3x3(const mathfu::mat4& m);

// Returns the 3-d projection of a homogeneous 4-vector.
// This is (x/w, y/w, z/w). Beware, if w == 0, the result will be Inf/NaN.
mathfu::vec3 ProjectHomogeneous(const mathfu::vec4& a);

// Finds where current_point falls along an ordered list of points. Sets
// min_index and max_index to the items in the array below and above
// current_point respectively, and percentage to the amount that current_point
// falls between the two items. Clamps current_point to the lower and upper
// bounds of the list.
void FindPositionBetweenPoints(const float current_point,
                               const std::vector<float>& points,
                               size_t* min_index, size_t* max_index,
                               float* match_percent);

// Given a line of startPosition to endPosition, return the % of the line
// segment closest to testPosition. Return result could be < 0 or > 1
float GetPercentageOfLineClosestToPoint(const mathfu::vec3& start_position,
                                        const mathfu::vec3& end_position,
                                        const mathfu::vec3& test_position);

// Tests whether |n| is a positive power of 2.
inline bool IsPowerOf2(uint32_t n) { return ((n & (n - 1)) == 0 && n != 0); }

// Aligns |n| to the next multiple of |align| (or |n| iff |n| == |align|).
inline uint32_t AlignToPowerOf2(uint32_t n, uint32_t align) {
  DCHECK(IsPowerOf2(align));
  return ((n + (align - 1)) & ~(align - 1));
}

// Converts the input degrees to radians.
inline float DegreesToRadians(float degree) {
  return degree * kDegreesToRadians;
}

// Converts the input radians to degrees.
inline float RadiansToDegrees(float radians) {
  return radians * kRadiansToDegrees;
}

// Returns the sum of the absolute differences between |a| and |b|.
float EulerDistance(const mathfu::vec3& a, const mathfu::vec3& b);

// Returns an equivalent angle to |value| normalized to be within +/- pi of
// |target|.
float EulerNormalize(float target, float value);

// Given two Euler angle representations of a rotation, returns a set of Euler
// angles equivalent to |value| such that the individual X, Y, and Z components
// are as close to |prev| as possible. This function should be called once per
// sample when creating rotation animation curves using the previous input
// as |prev|.
mathfu::vec3 EulerFilter(const mathfu::vec3& value, const mathfu::vec3& prev);

// Returns a TBN quaternion for |normal| and |tangent| packed into a vec4 in
// XYZW order (for convenient memcpy'ing). Both |normal| and |tangent| are
// assumed to be non-normalized.
mathfu::vec4 OrientationForTbn(const mathfu::vec3& normal,
                               const mathfu::vec3& tangent);

template <class T, int d>
inline mathfu::Vector<T, d> VectorFromSpan(const Span<uint8_t>& span) {
  CHECK(span.size() == sizeof(T) * d);
  return mathfu::Vector<T, d>(reinterpret_cast<const T*>(span.data()));
}

template <class T, int d>
inline Span<uint8_t> SpanFromVector(const mathfu::Vector<T, d>& vect) {
  return Span<uint8_t>(reinterpret_cast<const uint8_t*>(&vect[0]),
                       sizeof(T) * d);
}

// Type-cast that checks for lossy integer conversions.
// Can be used as a drop-in replacement for static_cast, like so:
//   SmallType to = IntCast<SmallType>(some_larger_type);
template <typename ToType, typename FromType>
inline ToType IntCast(FromType from) {
  constexpr FromType kMin = std::numeric_limits<ToType>::min();
  constexpr FromType kMax = std::numeric_limits<ToType>::max();
  static_assert(kMin >= std::numeric_limits<FromType>::min(), "");
  static_assert(kMax <= std::numeric_limits<FromType>::max(), "");
  DCHECK_GE(from, kMin);
  DCHECK_LE(from, kMax);
  return static_cast<ToType>(from);
}

// Like IntCast, except it stores to an output pointer so the output type is
// inferred from the destination.
template <typename ToType, typename FromType>
inline void IntStore(FromType from, ToType* to) {
  *to = IntCast<ToType>(from);
}

}  // namespace lull

#endif  // LULLABY_UTIL_MATH_H_
