#include "lullaby/data/shaders/vertex_common.glslh"

uniform float laser_width;
uniform vec3 camera_pos;
uniform mat4 entity_from_world;
uniform vec3 control_points[4];

STAGE_INPUT vec4 aPosition;
STAGE_OUTPUT mediump vec2 vTexCoord;

vec3 BezierPosition(float t, vec3 P[4]) {
  float t2 = t * t;
  float one_minus_t = 1.0 - t;
  float one_minus_t2 = one_minus_t * one_minus_t;
  return P[0] * one_minus_t2 * one_minus_t +
         P[1] * 3.0 * t * one_minus_t2 +
         P[2] * 3.0 * t2 * one_minus_t +
         P[3] * t2 * t;
}

vec3 BezierTangent(float t, vec3 P[4]) {
  float t2 = t * t;
  float one_minus_t = 1.0 - t;
  float one_minus_t2 = one_minus_t * one_minus_t;
  return -3.0 * (P[0] * one_minus_t2 +
                 P[1] * (-3.0 * t2 + 4.0 * t - 1.0) +
                 t * (3.0 * P[2] * t - 2.0 * P[2] - P[3] * t));
}

void main() {
  // local positions from our quad will be [-0.5..0.5] in x/y dimensions
  vec2 texcoord = aPosition.xy + vec2(0.5, 0.5);
  float extrude_dir = aPosition.x * 2.0; // -1 / 1
  float curve_t= texcoord.y;

  vec3 world_pos = BezierPosition(curve_t, control_points);
  vec3 world_tangent = normalize(BezierTangent(curve_t, control_points));
  vec3 view_to_pos = normalize(world_pos - camera_pos);
  vec3 extrude_axis = normalize(cross(view_to_pos, world_tangent));
  vec3 extruded_world_pos = world_pos +
                            extrude_axis * extrude_dir * laser_width;
  vec3 extruded_local_pos = (entity_from_world*vec4(extruded_world_pos, 1)).xyz;

  gl_Position = GetClipFromModelMatrix() * vec4(extruded_local_pos, 1);
  vTexCoord = texcoord;
}

