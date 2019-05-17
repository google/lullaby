#define MAX_COLORED_BUTTONS 20
#define MAX_BONES 20
#define NUM_VEC4S_IN_AFFINE_TRANSFORM 3

#include "lullaby/data/shaders/vertex_common.glslh"
#include "lullaby/data/shaders/skinning.glslh"

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec2 aTexCoord;

STAGE_OUTPUT mediump vec2 vTexCoord;
STAGE_OUTPUT vec4 vColor;

uniform vec4 button_uv_rects[MAX_COLORED_BUTTONS];
uniform vec4 button_colors[MAX_COLORED_BUTTONS];
uniform vec4 uv_bounds;

DECL_RIGID_SKINNING;

// Returns true if point is inside box.
// Expects rect as xmin, ymin, xmax, ymax.
bool inRect(vec2 point, vec4 rect) {
  // Use (vec2(1,1) - step(point, rect.zw)) to be inclusive at max bounds.
  vec2 result = step(rect.xy, point) - (vec2(1.0,1.0) - step(point, rect.zw));
  // Output of step should be 0 or 1 for each value.  Check against 0.5f to
  // avoid any possibility of floating point error.
  return result.x * result.y > 0.5;
}

void main() {
  vTexCoord = uv_bounds.xy + aTexCoord * uv_bounds.zw;
  vec4 skinned_position = ONE_BONE_SKINNED_VERTEX(aPosition);

  vColor = vec4(0.0);
  for (int i = 0; i < MAX_COLORED_BUTTONS; ++i) {
    if (inRect(vTexCoord, button_uv_rects[i])) {
      vColor = mix(vColor, button_colors[i], button_colors[i].a);
    }
  }

  gl_Position = GetClipFromModelMatrix() * skinned_position;
}
