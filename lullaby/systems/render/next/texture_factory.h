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

#include "lullaby/modules/render/image_data.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

// Creates and manages Texture objects.
//
// See TextureFactory documentation for more information.
class TextureFactoryImpl : public TextureFactory {
 public:
  explicit TextureFactoryImpl(Registry* registry);
  TextureFactoryImpl(const TextureFactoryImpl&) = delete;
  TextureFactoryImpl& operator=(const TextureFactoryImpl&) = delete;

  /// Caches a texture for later retrieval. Effectively stores the shared_ptr
  /// to the texture in an internal cache, allowing all references to be
  /// destroyed without actually destroying the texture itself.
  void CacheTexture(HashValue name, const TexturePtr& texture) override;

  /// Retrieves a cached texture by its name hash, or returns nullptr if the
  /// texture is not cached.
  TexturePtr GetTexture(HashValue name) const override;

  /// Releases the cached texture associated with |name|. If no other references
  /// to the texture exist, then it will be destroyed.
  void ReleaseTexture(HashValue name) override;

  /// Creates a texture using the |image| data and configured on the GPU using
  /// the creation |params|.
  TexturePtr CreateTexture(ImageData image,
                           const CreateParams& params) override;

  /// Creates a "named" texture using the |image| data and configured on the GPU
  /// using the creation |params|. Subsequent calls to this function with the
  /// same texture |name| will return the original texture as long as any
  /// references to that texture are still alive.
  TexturePtr CreateTexture(HashValue name, ImageData image,
                           const CreateParams& params) override;

  /// Loads a texture off disk with the given |filename| and uses the creation
  /// |params| to configure it for the GPU. The filename is also used as the
  /// "name" of the texture. Subsequent calls to this function with the same
  /// |filename| will return the original texture as long as any references to
  /// that texture are still alive.
  TexturePtr LoadTexture(string_view filename,
                         const CreateParams& params) override;

  /// Updates the entire image contents of |texture| using |image|. Image data
  /// is sent as-is. Returns false if the dimensions don't match or if the
  /// texture isn't internal and 2D.
  bool UpdateTexture(TexturePtr texture, ImageData image) override;

  /// Creates a texture from specified GL |texture_target| and |texture_id|.
  /// This function is primarily used for "wrapping" a texture from an external
  /// source (eg. camera feed) within a lull::Texture.
  TexturePtr CreateTexture(uint32_t texture_target, uint32_t texture_id);

  /// Similar to above, but allows the caller to specify the texture size.
  TexturePtr CreateTexture(uint32_t texture_target, uint32_t texture_id,
                           const mathfu::vec2i& size);

  /// Creates an "empty" texture of the specified size. The |params| must
  /// specify a format for the texture.
  TexturePtr CreateTexture(const mathfu::vec2i& size,
                           const CreateParams& params);

  // Creates a texture as a subtexture of another texture.
  TexturePtr CreateSubtexture(const TexturePtr& texture,
                              const mathfu::vec4& uv_bounds);

  // Creates a named texture as a subtexture of another texture.
  TexturePtr CreateSubtexture(HashValue key, const TexturePtr& texture,
                              const mathfu::vec4& uv_bounds);

  // Returns a resident white texture with an alpha channel: (1, 1, 1, 1).
  const TexturePtr& GetWhiteTexture() const;

  // Returns a resident invalid texture to be used when a requested image fails
  // to load.  On debug builds it's a watermelon; on release builds it's just
  // the white texture.
  const TexturePtr& GetInvalidTexture() const;

  // DEPRECATED. Old RenderSystem API passes ImageData by const reference which
  // can be redirected here.
  TexturePtr CreateTexture(const ImageData* image, const CreateParams& params);
  TexturePtr CreateTexture(HashValue name, const ImageData* image,
                           const CreateParams& params);

 private:
  void InitTextureImpl(const TexturePtr& texture, const ImageData* image,
                       const TextureFactory::CreateParams& params);

  Registry* registry_;
  ResourceManager<Texture> textures_;
  TexturePtr white_texture_;
  TexturePtr invalid_texture_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextureFactoryImpl);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_FACTORY_H_
