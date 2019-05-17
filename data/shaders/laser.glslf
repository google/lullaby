#include "lullaby/data/shaders/fragment_common.glslh"

STAGE_INPUT mediump vec2 vTexCoord;
uniform sampler2D texture_unit_0;
uniform lowp vec4 color;
uniform mediump vec4 fade_points;

#define kNearFadeWidth 0.02
#define kNearFadeLength 0.0033

void main() {
  mediump float near_fade =
      clamp((vTexCoord.y - fade_points[0]) / (fade_points[1] - fade_points[0]),
            0.0, 1.0);
  mediump float far_fade = clamp(
      1.0 - (vTexCoord.y - fade_points[2]) / (fade_points[3] - fade_points[2]),
      0.0, 1.0);
  mediump float width_fade = clamp(
      (vTexCoord.y - abs(vTexCoord.x - 0.5) * kNearFadeWidth) / kNearFadeLength,
      0.0, 1.0);
  mediump float fade_factor = near_fade * far_fade * width_fade;

  lowp vec4 texture_color = texture2D(texture_unit_0, vTexCoord);
  lowp vec4 premult_uniform_color = vec4(color.rgb * color.a, color.a);
  gl_FragColor = premult_uniform_color * texture_color * fade_factor;
}
