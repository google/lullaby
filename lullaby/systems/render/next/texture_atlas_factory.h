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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_ATLAS_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_ATLAS_FACTORY_H_

#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "fplbase/texture_atlas.h"
#include "lullaby/systems/render/next/texture_atlas.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

// Creates and manages TextureAtlas objects.
//
// TextureAtlas objects will automatically be added to the internal cache and
// must be explicitly released.
class TextureAtlasFactory {
 public:
  TextureAtlasFactory(Registry* registry, fplbase::Renderer* renderer);
  TextureAtlasFactory(const TextureAtlasFactory&) = delete;
  TextureAtlasFactory& operator=(const TextureAtlasFactory&) = delete;

  // Loads the texture atlas with the given |filename| and optionally creates
  // mips.
  void LoadTextureAtlas(const std::string& filename, bool create_mips);

  // Releases the cached TextureAtlas associated with |key|.
  void ReleaseTextureAtlasFromCache(HashValue key);

 private:
  Registry* registry_;
  fplbase::Renderer* fpl_renderer_;
  ResourceManager<TextureAtlas> atlases_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextureAtlasFactory);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_ATLAS_FACTORY_H_
