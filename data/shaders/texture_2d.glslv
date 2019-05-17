// Simple 2D textured shader. Position is scaled and offset into normalized
// device coordinates, bypassing the view transform.

#include "lullaby/data/shaders/vertex_common.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT vec2 vTexCoord;

uniform vec4 position_offset;
uniform vec4 position_scale;
uniform vec4 uv_bounds;

void main() {
  gl_Position = position_offset + aPosition * position_scale;
  vTexCoord = uv_bounds.xy + aTexCoord * uv_bounds.zw;
}
