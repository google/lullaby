#include "lullaby/data/shaders/fragment_common.glslh"

STAGE_INPUT mediump vec2 vTexCoord;
STAGE_INPUT lowp vec4 vColor;
uniform mediump float ring_diameter;
uniform mediump float inner_hole;
uniform mediump float inner_ring_end;
uniform mediump float inner_ring_thickness;
uniform mediump float mid_ring_end;
uniform mediump float mid_ring_opacity;

// TODO  Some of this math could be precomputed on the CPU, which is
// always preferable to doing it per-fragment: color_radius,
// color_feather_radius, hole_radius, black_radius, black_feather.

void main() {
  mediump float r = length(vTexCoord - vec2(0.5, 0.5));
  mediump float color_radius = inner_ring_end*ring_diameter;
  mediump float color_feather_radius = inner_ring_thickness*ring_diameter;
  mediump float hole_radius = inner_hole * ring_diameter - color_feather_radius;
  mediump float color1 =
    clamp(1.0-(r - color_radius)/color_feather_radius,0.0,1.0);
  mediump float hole_alpha =
    clamp((r - hole_radius)/color_feather_radius, 0.0, 1.0);

  mediump float black_radius = mid_ring_end*ring_diameter;
  mediump float black_feather = 1.0 / (ring_diameter*0.5 - black_radius);
  mediump float black_alpha_factor =
    mid_ring_opacity*(1.0-(r-black_radius)*black_feather);
  mediump float alpha =
    clamp(min(hole_alpha, max(color1,black_alpha_factor)),0.0,1.0);
  vec3 color_rgb = color1*vColor.xyz;
  float final_alpha = vColor.w * alpha;
  gl_FragColor = vec4(color_rgb * final_alpha, final_alpha);
}
