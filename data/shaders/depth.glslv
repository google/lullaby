#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;

uniform mat4 model;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
}
