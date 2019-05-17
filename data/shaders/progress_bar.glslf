#extension GL_OES_standard_derivatives : enable

#include "lullaby/data/shaders/fragment_common.glslh"

STAGE_INPUT mediump vec2 vTexCoord;
uniform lowp vec4 color;

uniform lowp vec4 progress_base_color;
uniform lowp vec4 progress_color;

uniform lowp float progress_start;
uniform lowp float progress_stop;

void main() {
  // Compute 1-D distance inside in-progress region
  float start_distance = vTexCoord.x - progress_start;
  float stop_distance = progress_stop - vTexCoord.x;
  float progress_distance = min(start_distance, stop_distance);

  // Scale distance by rate-of-change of distance and clamp, and
  // this is our blend ratio
  float blend = progress_distance / fwidth(progress_distance);
  blend = clamp(blend, 0., 1.);

  // Blend and set frag color
  vec4 out_color = color * mix(progress_base_color, progress_color, blend);
  gl_FragColor = vec4(out_color.rgb * out_color.a, out_color.a);
}
