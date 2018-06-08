// Scales and offsets UV0 by "uv_bounds" uniform. Supports multiview.

#include "third_party/lullaby/data/shaders/vertex_common.glslh"

uniform vec3 camera_pos;
uniform mat4 model;
uniform mat3 mat_normal;

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec3 aNormal;
STAGE_INPUT vec2 aTexCoord;
STAGE_INPUT vec4 aOrientation;
STAGE_OUTPUT vec3 vViewDirection;
STAGE_OUTPUT vec2 vTexCoord;
STAGE_OUTPUT mat3 vTangentBitangentNormal;

mat3 Mat3FromQuaterion(vec4 q) {
  mat3 dest;
  vec3 two_v = 2. * q.xyz;
  vec3 two_v_sq = two_v * q.xyz;
  float two_x_y = two_v.x * q.y;
  float two_x_z = two_v.x * q.z;
  float two_y_z = two_v.y * q.z;
  vec3 two_v_w = two_v * q.w;

  dest[0][0] = 1. - two_v_sq.y - two_v_sq.z;
  dest[1][0] = two_x_y - two_v_w.z;
  dest[2][0] = two_x_z + two_v_w.y;

  dest[0][1] = two_x_y + two_v_w.z;
  dest[1][1] = 1. - two_v_sq.x - two_v_sq.z;
  dest[2][1] = two_y_z - two_v_w.x;

  dest[0][2] = two_x_z - two_v_w.y;
  dest[1][2] = two_y_z + two_v_w.x;
  dest[2][2] = 1. - two_v_sq.x - two_v_sq.y;

  return dest;
}

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vec4 position_homogeneous = model * aPosition;
  vec3 position = position_homogeneous.xyz / position_homogeneous.w;
  vViewDirection = camera_pos - position;
  // TODO(b/80094251) Get UVs working correctly we so don't have to invert y.
  // The proper fix is to get texture_pipeline to flip the texture before it is
  // compressed.
  vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);

  vTangentBitangentNormal = mat_normal * Mat3FromQuaterion(aOrientation);
}
