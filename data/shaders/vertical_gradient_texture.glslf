#include "lullaby/data/shaders/uber_fragment_common.glslh"

uniform lowp vec4 color_top;
uniform lowp vec4 color_bottom;
uniform sampler2D texture_unit_0;

void main() {
  lowp vec4 mixed_color =
      GetColor() * mix(color_top, color_bottom, GetVTexCoord().y);
  lowp vec4 texture_color = SampleTexture(texture_unit_0, vTexCoord);
  SetFragColor(PremultiplyAlpha(mixed_color) * texture_color);
}
