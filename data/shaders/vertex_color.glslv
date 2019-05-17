// Scales color uniform by vertex color and passes result to frag as "vColor".
// Supports multiview.

#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec4 aColor;
STAGE_OUTPUT vec4 vColor;
uniform lowp vec4 color;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vColor = aColor * color;
}
