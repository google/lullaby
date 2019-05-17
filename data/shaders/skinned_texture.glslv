#define MAX_BONES 96
#define NUM_VEC4S_IN_AFFINE_TRANSFORM 3

#include "lullaby/data/shaders/vertex_common.glslh"
#include "lullaby/data/shaders/skinning.glslh"

// TODO Merge this into texture.glslv using

STAGE_INPUT vec4 aPosition;
STAGE_INPUT vec2 aTexCoord;
STAGE_OUTPUT mediump vec2 vTexCoord;

uniform mediump vec4 uv_bounds;

DECL_SOFT_SKINNING;

void main() {
  vec4 skinned_position = FOUR_BONE_SKINNED_VERTEX(aPosition);

  gl_Position = GetClipFromModelMatrix() * skinned_position;
  vTexCoord = uv_bounds.xy + aTexCoord * uv_bounds.zw;
}
