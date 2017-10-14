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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_ATLAS_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_ATLAS_H_

#include <memory>
#include <unordered_map>
#include "fplbase/asset_manager.h"
#include "fplbase/texture.h"
#include "lullaby/systems/render/texture.h"

namespace lull {

// A single texture that contains named "subtextures" which can be individually
// referenced as a normal texture.
class TextureAtlas {
 public:
  TextureAtlas() {}
  TextureAtlas(const TextureAtlas& rhs) = delete;
  TextureAtlas& operator=(const TextureAtlas& rhs) = delete;

  const std::vector<std::string>& Subtextures() const;
  std::shared_ptr<Texture> GetTexture() const;

 private:
  friend class TextureAtlasFactory;
  void Init(std::shared_ptr<Texture> texture,
            std::vector<std::string> subtextures);

  std::shared_ptr<Texture> texture_;
  std::vector<std::string> subtextures_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_ATLAS_H_
