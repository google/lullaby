/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lullaby/systems/render/testing/texture.h"

#if !defined(LULLABY_RENDER_BACKEND_MOCK)
#error This file should only be included with mock backend targets.
#endif  // !defined(LULLABY_RENDER_BACKEND_MOCK)

namespace lull {

bool IsTextureLoaded(const TexturePtr& texture) {
  return texture != nullptr;
}

mathfu::vec2i GetTextureDimensions(const TexturePtr& texture) {
  return mathfu::kZeros2i;
}

Optional<int> GetTextureGlHandle(const TexturePtr& texture) {
  return NullOpt;
}

bool IsTextureExternalOes(const TexturePtr& texture) {
  // GL_TEXTURE_EXTERNAL_OES not currently supported.
  return false;
}

}  // namespace lull
