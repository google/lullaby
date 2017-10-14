/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/render/next/texture_atlas.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"

namespace lull {

void TextureAtlas::Init(std::shared_ptr<Texture> texture,
                        std::vector<std::string> subtextures) {
  texture_ = std::move(texture);
  subtextures_ = std::move(subtextures);
}

const std::vector<std::string>& TextureAtlas::Subtextures() const {
  return subtextures_;
}

std::shared_ptr<Texture> TextureAtlas::GetTexture() const { return texture_; }

}  // namespace lull
