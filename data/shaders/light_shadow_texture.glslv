#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec3 aNormal;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT vec3 vNormal;
STAGE_OUTPUT vec3 vVertPos;
STAGE_OUTPUT vec2 vTexCoord;
STAGE_OUTPUT vec4 vVertPosLightSpace;

uniform mat4 model;
uniform mat3 mat_normal;
uniform mat4 directional_light_shadow_matrix;

void main() {
  vec4 model_position = (model * aPosition);
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vNormal = mat_normal * aNormal;
  vVertPos = model_position.xyz;
  vTexCoord = aTexCoord;
  vVertPosLightSpace = directional_light_shadow_matrix * model_position;
}
