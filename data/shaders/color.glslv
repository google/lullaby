#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
}
