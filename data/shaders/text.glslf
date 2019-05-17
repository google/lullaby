#include "lullaby/data/shaders/uber_fragment_common.glslh"

uniform sampler2D texture_unit_0;

// x: dist offset, y: dist scale, z: dist @ a = 0, w: dist @ a = 1 (NB: w > z!)
// For white glyphs in black space (flatui): x = 0, y = 1.
// For black glyphs in white space (Ion): x = 1, y = -1.
uniform lowp vec4 sdf_params;

#ifdef OUTLINE
// x: dist @ outline = 1, y: dist @ outline = 0 (NB: y > x!)
// To turn off outline, make x < y < sdf_params.z.
uniform lowp vec2 outline_params;
uniform lowp vec4 outline_color;
#endif

void main() {
  float dist = sdf_params.x +
               sdf_params.y * texture2D(texture_unit_0, GetVTexCoord()).r;

  if (dist < sdf_params.z)
    discard;

  float alpha = smoothstep(sdf_params.z, sdf_params.w, dist);

#ifdef OUTLINE
  float inv_outline = smoothstep(outline_params.x, outline_params.y, dist);
  vec4 frag_color = mix(outline_color, GetColor(), inv_outline);
#else
  vec4 frag_color = GetColor();
#endif

  SetFragColor(PremultiplyAlpha(frag_color) * alpha);
}
