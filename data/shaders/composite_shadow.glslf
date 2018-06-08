varying mediump vec2 vTexCoord;
uniform sampler2D texture_unit_0;
uniform lowp vec4 color;

uniform mediump vec4 clamp_bounds;
uniform mediump float blur_level;

void main() {
  mediump vec2 uv = clamp(vTexCoord, clamp_bounds.xy, clamp_bounds.zw);

  vec3 shadow_opacities = texture2D(texture_unit_0, uv).rgb;

  float shadow_opacity = blur_level <= 0.5 ?
    mix(shadow_opacities[0], shadow_opacities[1], blur_level * 2.0) :
    mix(shadow_opacities[1], shadow_opacities[2], blur_level * 2.0 - 1.0);
  float final_alpha = color.a * shadow_opacity;
  gl_FragColor = vec4(color.rgb * final_alpha, final_alpha);
}
