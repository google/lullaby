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

#ifndef LULLABY_SYSTEMS_RENDER_TEXTURE_H_
#define LULLABY_SYSTEMS_RENDER_TEXTURE_H_

#include <memory>
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"

namespace lull {

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

// Returns true if the texture is loaded.
bool IsTextureLoaded(const TexturePtr& texture);

// Returns the dimensions of the texture.
mathfu::vec2i GetTextureDimensions(const TexturePtr& texture);

// Returns the underlying OpenGL texture handle for the texture for supported
// platforms/backends.
Optional<int> GetTextureGlHandle(const TexturePtr& texture);

// Returns true if the texture is GL_TEXTURE_EXTERNAL_OES for supported
// platforms/backends.
bool IsTextureExternalOes(const TexturePtr& texture);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_TEXTURE_H_
