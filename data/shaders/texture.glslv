// Scales and offsets UV0 by "uv_bounds" uniform. Supports multiview.

#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT vec2 vTexCoord;

uniform vec4 uv_bounds;

void main() {
  gl_Position = GetClipFromModelMatrix() * aPosition;
  vTexCoord = uv_bounds.xy + aTexCoord * uv_bounds.zw;
}
