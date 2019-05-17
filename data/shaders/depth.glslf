#include "lullaby/data/shaders/fragment_common.glslh"

void main() {
  gl_FragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1);
}
