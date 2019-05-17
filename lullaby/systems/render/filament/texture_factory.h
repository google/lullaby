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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_TEXTURE_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_TEXTURE_FACTORY_H_

#include "filament/Engine.h"
#include "lullaby/modules/render/image_data.h"
#include "lullaby/systems/render/filament/texture.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Creates and manages Texture objects.
//
// Textures will be automatically released along with the last external
// reference unless explicitly added to the internal texture cache.
class TextureFactoryImpl : public TextureFactory {
 public:
  TextureFactoryImpl(Registry* registry, filament::Engine* engine);
  TextureFactoryImpl(const TextureFactoryImpl&) = delete;
  TextureFactoryImpl& operator=(const TextureFactoryImpl&) = delete;

  enum TextureFormat {
    kUnknown,
  };

  // Returns a resident white texture with an alpha channel: (1, 1, 1, 1).
  TexturePtr GetWhiteTexture() const override;

  // Returns a resident invalid texture to be used when a requested image fails
  // to load.  On debug builds it's a watermelon; on release builds it's just
  // the white texture.
  TexturePtr GetInvalidTexture() const override;

  // Caches a texture for later retrieval.
  void CacheTexture(HashValue name, const TexturePtr& texture) override;

  // Releases the cached texture associated with |texture_hash|.
  void ReleaseTexture(HashValue name) override;

  // Retrieves a cached texture by its name hash. If the texture isn't cached
  // this returns nullptr.
  TexturePtr GetTexture(HashValue name) const override;

  // Loads the texture with the given |filename| and |params|.
  TexturePtr LoadTexture(string_view filename,
                         const TextureParams& params) override;

  // Loads a texture atlas with the given |filename| and |params|.
  void LoadAtlas(const std::string& filename,
                 const TextureParams& params) override;

  // Creates a texture from the ImageData.
  TexturePtr CreateTexture(ImageData image,
                           const TextureParams& params) override;

  // Creates a texture from the ImageData.
  TexturePtr CreateTexture(HashValue name, ImageData image,
                           const TextureParams& params) override;

  // Creates an external texture.
  TexturePtr CreateExternalTexture();

  // Creates an external texture with |size|.
  TexturePtr CreateExternalTexture(const mathfu::vec2i& size) override;

  // Updates the entire image contents of |texture| using |image|.
  bool UpdateTexture(TexturePtr texture, ImageData image) override;

  // DEPRECATED. Old RenderSystem API passes ImageData by const reference which
  // can be redirected here.
  TexturePtr CreateTextureDeprecated(const ImageData* image,
                                     const TextureParams& params) override;

 private:
  void InitTextureImpl(TexturePtr texture, const ImageData& image,
                       const TextureParams& params);

  Registry* registry_;
  ResourceManager<Texture> textures_;
  TexturePtr white_texture_;
  TexturePtr invalid_texture_;
  filament::Engine* engine_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextureFactoryImpl);

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_TEXTURE_FACTORY_H_
