#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT mediump vec2 vTexCoord;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vTexCoord = aTexCoord;
}
