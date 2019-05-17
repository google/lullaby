#include "lullaby/data/shaders/uber_fragment_common.glslh"

void main() {
  SetFragColor(PremultiplyAlpha(GetColor()));
}
