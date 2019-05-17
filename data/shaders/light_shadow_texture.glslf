#include "lullaby/data/shaders/fragment_common.glslh"
#include "lullaby/data/shaders/light.glslh"

STAGE_INPUT mediump vec3 vNormal;
STAGE_INPUT mediump vec3 vVertPos;
STAGE_INPUT mediump vec2 vTexCoord;

uniform sampler2D texture_unit_0;
uniform lowp vec4 color;

LIGHT_DECL_AMBIENT(1);
LIGHT_DECL_POINT(1);
LIGHT_DECL_DIRECTIONAL_SHADOW_5;

uniform vec3 camera_dir;

void main() {
  // Sample the texture.
  lowp vec4 texture_color = texture2D(texture_unit_0, vTexCoord);

  // Re-normalize the normal which been could have been scaled or interpolated
  // from the vertex stage.
  vec3 normal = normalize(vNormal);

  // Light calculations.
  vec3 light_color = vec3(0.0, 0.0, 0.0);
  LIGHT_CALCULATE_AMBIENTS(light_color)
  LIGHT_CALCULATE_DIRECTIONALS_SHADOW_5(light_color, vVertPos, normal, camera_dir)
  LIGHT_CALCULATE_POINTS(light_color, vVertPos, normal, camera_dir)

  gl_FragColor = vec4(texture_color.rgb * color.rgb * color.a * light_color,
                      texture_color.a * color.a);
}
