/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_ENGINES_RENDER_THUNKS_TEXTURE_H_
#define REDUX_ENGINES_RENDER_THUNKS_TEXTURE_H_

#include "redux/engines/render/texture.h"

namespace redux {

// Thunk functions to call the actual implementation.
const std::string& Texture::GetName() const { return Upcast(this)->GetName(); }
TextureTarget Texture::GetTarget() const { return Upcast(this)->GetTarget(); }
vec2i Texture::GetDimensions() const { return Upcast(this)->GetDimensions(); }
bool Texture::Update(ImageData image) {
  return Upcast(this)->Update(std::move(image));
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_TEXTURE_H_
