#version 300 es

// We need a separate vertex shader to match the version used in the fragment
// shader. The version should also go on the first line of the shader.

#include "lullaby/data/shaders/uber_vertex_common.glslh"

void main() {
  InitVertexShading();
}
