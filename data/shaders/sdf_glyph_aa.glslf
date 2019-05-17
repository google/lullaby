#version 300 es

#include "lullaby/data/shaders/uber_fragment_common.glslh"

uniform sampler2D texture_unit_0;

// This 16 here is what is used in our SDF texture generation tools as the
// number of texels that represent a distance of 1.  Ideally this would be
// plumbed in to the shader as a compile-time constant rather than redefined
// here.
#define kSDFTextureUnitDistancePerTexel (1. / 16.)
#define kSDFTransitionValue .5

#ifdef OUTLINE
uniform lowp float outline_width;
uniform lowp vec4 outline_color;
#endif  // OUTLINE

void main() {
  float sdf_dist = texture(texture_unit_0, GetVTexCoord()).r -
                   kSDFTransitionValue;

  vec2 uv_texel = GetVTexCoord() * vec2(textureSize(texture_unit_0, 0));
  vec2 width = fwidth(uv_texel);
  float uv_rate = max(width.x, width.y);
  float sdf_dist_rate = uv_rate * kSDFTextureUnitDistancePerTexel;

  // Here we divide the distance by the rate of change of distance with respect
  // to screen pixels, which then allows us to precisely control the width of
  // the antialiasing band.
  // We add .5 to make sure the antialiasing pixels occur just along the
  // outside of the SDF edge which preserves the solid interior of the glyph
  // better during minimization.  This may seem inconsequential or hacky but it
  // was the final piece needed to make the text readable at distance.
  float alpha = sdf_dist / sdf_dist_rate + .5;

#ifdef OUTLINE
  float outline_alpha = (sdf_dist + outline_width) / sdf_dist_rate + 0.5;
  if (outline_alpha <= 0.0) {
    discard;
  }

  alpha = clamp(alpha, 0.0, 1.0);
  vec4 frag_color = outline_color;
  outline_alpha = min(outline_alpha, 1.0);
  frag_color.a *= outline_alpha;
  vec4 uniform_color = GetColor();
  uniform_color.a = 1.0;
  // Note: technically this should use gamma correction for the blending, but
  // it isn't visible due to how short the transition is.
  frag_color = mix(frag_color, uniform_color, alpha);
  frag_color.a *= GetColor().a;
#else  // OUTLINE
  if (alpha <= 0.)
    discard;

  alpha = min(alpha, 1.);

  vec4 frag_color = GetColor();
  frag_color.a *= alpha;

#endif  // OUTLINE


  SetFragColor(PremultiplyAlpha(frag_color));
}
