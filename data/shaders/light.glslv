#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec3 aNormal;
STAGE_OUTPUT vec3 vNormal;
STAGE_OUTPUT vec3 vVertPos;

uniform mat4 model;
uniform mat3 mat_normal;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vNormal = mat_normal * aNormal;
  vVertPos = (model * aPosition).xyz;
}
