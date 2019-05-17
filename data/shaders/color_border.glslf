#include "lullaby/data/shaders/uber_fragment_common.glslh"

uniform vec4 border_color;
uniform float border_width;

void main() {
  float sdf_dist = distance(vTexCoord, vec2(0.5)) + border_width - 0.5;
  float border_amount = GET_AA_FROM_SDF(float, sdf_dist);
  // Interpolate between the normal color and the border color with that amount.
  vec4 color = mix(GetColor(), border_color, border_amount);
  SetFragColor(PremultiplyAlpha(color));
}
