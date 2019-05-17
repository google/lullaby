// Samples texture 0 and scales by a "color" uniform.

#include "lullaby/data/shaders/uber_fragment_common.glslh"

uniform sampler2D texture_unit_0;

void main() {
  lowp vec4 texture_color = SampleTexture(texture_unit_0, GetVTexCoord());
  SetFragColor(PremultiplyAlpha(GetColor()) * texture_color);
}
