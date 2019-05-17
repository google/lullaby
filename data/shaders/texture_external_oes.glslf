// Samples external texture 0 and scales by a "color" uniform.
// TODO: It would be useful to merge this shader with texture.glslf
// and switch the input according to a define.  But this breaks compatibility
// with the Android emulator due to a bug in the emulator's shader compiler (it
// performs a dumb search for samplerExternalOES textures, locating them even in
// disabled code blocks).

#ifdef GL_ES
#extension GL_OES_EGL_image_external : enable
#endif  // GL_ES

#include "lullaby/data/shaders/uber_fragment_common.glslh"

#ifdef GL_ES
uniform samplerExternalOES texture_unit_0;
#else  // GL_ES
uniform sampler2D texture_unit_0;
#endif  // GL_ES

void main() {
  lowp vec4 texture_color = texture2D(texture_unit_0, GetVTexCoord());
  SetFragColor(PremultiplyAlpha(GetColor()) * texture_color);
}
