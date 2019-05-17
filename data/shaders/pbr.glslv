// Scales and offsets UV0 by "uv_bounds" uniform. Supports multiview.

#include "lullaby/data/shaders/vertex_common.glslh"
#include "lullaby/data/shaders/math.glslh"

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



void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vec4 position_homogeneous = model * aPosition;
  vec3 position = position_homogeneous.xyz / position_homogeneous.w;
  vViewDirection = camera_pos - position;
  // TODO Get UVs working correctly we so don't have to invert y.
  // The proper fix is to get texture_pipeline to flip the texture before it is
  // compressed.
  vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);

  vTangentBitangentNormal = mat_normal * Mat3FromQuaternion(aOrientation);
}
