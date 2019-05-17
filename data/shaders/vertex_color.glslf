// Outputs the vertex color.

#include "lullaby/data/shaders/fragment_common.glslh"

// It's assumed that vColor has already been multiplied by the "color" uniform
// in the vertex shader.
STAGE_INPUT lowp vec4 vColor;

void main() {
  gl_FragColor = vec4(vColor.rgb * vColor.a, vColor.a);
}
