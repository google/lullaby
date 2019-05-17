#extension GL_OES_EGL_image_external : require
#include "lullaby/data/shaders/fragment_common.glslh"

STAGE_INPUT mediump vec2 vTexCoord;
uniform samplerExternalOES texture_unit_0;
uniform lowp vec4 color;

void main() {
  lowp vec4 texture_color = texture2D(texture_unit_0, vTexCoord);
  gl_FragColor = vec4(color.rgb * color.a, color.a) * texture_color;
}
