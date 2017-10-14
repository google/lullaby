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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_FACTORY_H_

#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

// Creates and manages Texture objects.
//
// Textures will be automatically released along with the last external
// reference unless explicitly added to the internal texture cache.
class TextureFactory {
 public:
  TextureFactory(Registry* registry, fplbase::Renderer* renderer);
  TextureFactory(const TextureFactory&) = delete;
  TextureFactory& operator=(const TextureFactory&) = delete;

  // Returns a resident white texture with an alpha channel: (1, 1, 1, 1).
  const TexturePtr& GetWhiteTexture() const;

  // Returns a resident invalid texture to be used when a requested image fails
  // to load.  On debug builds it's a watermelon; on release builds it's just
  // the white texture.
  const TexturePtr& GetInvalidTexture() const;

  // Caches a texture for later retrieval.
  void CacheTexture(HashValue name, const TexturePtr& texture);

  // Releases the cached texture associated with |texture_hash|.
  void ReleaseTextureFromCache(HashValue texture_hash);

  // Retrieves a cached texture by its name hash. If the texture isn't cached
  // this returns nullptr.
  TexturePtr GetCachedTexture(HashValue texture_hash) const;

  // Loads the texture with the given |filename| and optionally creates mips.
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);

  // Creates a texture from specified GL |texture_target| and |texture_id|.
  TexturePtr CreateTexture(uint32_t texture_target, uint32_t texture_id);

  // Creates a texture from memory.  |data| is copied into GL memory, so it's no
  // longer needed after calling this function.
  TexturePtr CreateTextureFromMemory(const void* data, const mathfu::vec2i size,
                                     fplbase::TextureFormat format,
                                     bool create_mips = false);

  void CreateSubtexture(HashValue key, const TexturePtr& texture,
                        const mathfu::vec4& uv_bounds);

  // Create and return a pre-processed texture.  This will set up a rendering
  // environment suitable to render |sourcE_texture| with a pre-process shader.
  // texture and shader binding / setup should be performed in |processor|.
  TexturePtr CreateProcessedTexture(
      const TexturePtr& texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor,
      const mathfu::vec2i& output_dimensions);

  // Create and return a pre-processed texture as above, but size the output
  // according to |output_dimensions|.
  TexturePtr CreateProcessedTexture(
      const TexturePtr& source_texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor);


 private:
  Registry* registry_;
  fplbase::Renderer* fpl_renderer_;
  ResourceManager<Texture> textures_;
  TexturePtr white_texture_;
  TexturePtr invalid_texture_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextureFactory);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_FACTORY_H_
