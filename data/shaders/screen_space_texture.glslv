#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT vec2 vTexCoord;

uniform vec3 uv_matrix_rows[2];

void main() {
  gl_Position = aPosition;

  // Apply a 2x3 transform matrix to our UVs to allow for rotations & offsets.
  vec3 uv_vec3 = vec3(aTexCoord, 1.0);
  vTexCoord.x = dot(uv_matrix_rows[0], uv_vec3);
  vTexCoord.y = dot(uv_matrix_rows[1], uv_vec3);
}
