// Scales color uniform by vertex color and passes result to frag as "vColor".
// Passes through UV0. Supports multiview.

#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec4 aColor;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT vec4 vColor;
STAGE_OUTPUT vec2 vTexCoord;
uniform lowp vec4 color;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vColor = aColor * color;
  vTexCoord = aTexCoord;
}
