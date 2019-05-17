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

#ifndef LULLABY_SYSTEMS_RENDER_TEXTURE_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_TEXTURE_FACTORY_H_

#include <cstdint>
#include <functional>
#include <string>

#include "lullaby/modules/file/asset.h"
#include "lullaby/systems/render/animated_texture_processor.h"
#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/texture_params.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/typeid.h"
#include "lullaby/generated/texture_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// Asset class that can be used for loading textures.
class TextureAsset : public Asset {
 public:
  using Finalizer = std::function<void(TextureAsset*)>;
  using ErrorFn = std::function<void(ErrorCode)>;

  TextureAsset(const TextureParams& params, const Finalizer& finalizer,
               uint32_t decode_flags = 0);
  TextureAsset(const TextureParams& params, const Finalizer& finalizer,
               const ErrorFn& error_fn, uint32_t decode_flags = 0);

  ErrorCode OnLoadWithError(const std::string& filename,
                            std::string* data) override;

  void OnFinalize(const std::string& filename, std::string* data) override;

  void OnError(const std::string&, ErrorCode error) override;

  TextureParams params_;
  ImageData image_data_;
  Finalizer finalizer_;
  ErrorFn error_fn_;
  uint32_t flags_ = 0;

  // Null if image is not animated.
  AnimatedImagePtr animated_image_;
};

/// Provides mechanisms for creating and managing Texture objects.
///
/// The TextureFactory can be used to create Texture objects from either CPU
/// memory (via an ImageData object) or from disk. It also provides a caching
/// mechanism whereby multiple requests to a texture identified by a unique
/// name will return the same Texture object.
class TextureFactory {
 public:
  virtual ~TextureFactory() {}

  // Returns a simple texture that can be used as a placeholder.
  virtual TexturePtr GetWhiteTexture() const = 0;

  // Returns a simple invalid texture that can be used as a placeholder.
  // In debug builds, this will be an "ugly green and pink" pattern, but in
  // release builds this will be white.
  virtual TexturePtr GetInvalidTexture() const = 0;

  /// Caches a texture for later retrieval. Effectively stores the shared_ptr
  /// to the texture in an internal cache, allowing all references to be
  /// destroyed without actually destroying the texture itself.
  virtual void CacheTexture(HashValue name, const TexturePtr& texture) = 0;

  /// Retrieves a cached texture by its name hash, or returns nullptr if the
  /// texture is not cached.
  virtual TexturePtr GetTexture(HashValue name) const = 0;

  /// Releases the cached texture associated with |name|. If no other references
  /// to the texture exist, then it will be destroyed.
  virtual void ReleaseTexture(HashValue name) = 0;

  /// Creates a texture using the |image| data and configured on the GPU using
  /// the creation |params|.
  virtual TexturePtr CreateTexture(ImageData image,
                                   const TextureParams& params) = 0;

  /// Creates a "named" texture using the |image| data and configured on the GPU
  /// using the creation |params|. Subsequent calls to this function with the
  /// same texture |name| will return the original texture as long as any
  /// references to that texture are still alive.
  virtual TexturePtr CreateTexture(HashValue name, ImageData image,
                                   const TextureParams& params) = 0;

  /// Creates a texture using the specified TextureDef. If the |texture_def| has
  /// a name, it will cache the texture internally.
  TexturePtr CreateTexture(const TextureDef* texture_def);

  /// Creates a texture using the specified TextureDef. If the |texture_def| has
  /// a name, it will cache the texture internally.
  TexturePtr CreateTexture(const TextureDefT& texture_def);

  /// Loads a texture off disk with the given |filename| and uses the creation
  /// |params| to configure it for the GPU. The filename is also used as the
  /// "name" of the texture. Subsequent calls to this function with the same
  /// |filename| will return the original texture as long as any references to
  /// that texture are still valid.
  virtual TexturePtr LoadTexture(string_view filename,
                                 const TextureParams& params) = 0;
  /// Same as above, but with default params.
  TexturePtr LoadTexture(string_view filename);

  // Loads a texture atlas off disk with the given |filename| and uses the
  // creation |params| to configure it for the GPU.  Individual subtextures
  // will be extracted from the atlas and made available as their own Texture
  // objects.
  virtual void LoadAtlas(const std::string& filename,
                         const TextureParams& params) = 0;

  // Creates an external texture. |size| is ignored by some renderers.
  virtual TexturePtr CreateExternalTexture(const mathfu::vec2i& size) = 0;

  /// Updates the entire image contents of |texture| using |image|. The image
  /// data is sent as-is (this does not perform alpha premultiplication).
  /// Returns false on error, which can depend on the backend but likely
  /// includes size or format mismatch.
  virtual bool UpdateTexture(TexturePtr texture, ImageData image) = 0;

  // Same as CreateTexture(ImageData, const TextureParams&), but accepts a
  // pointer to the ImageData instead of by-value.
  virtual TexturePtr CreateTextureDeprecated(const ImageData* image,
                                             const TextureParams& params) = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextureFactory);

#endif  // LULLABY_SYSTEMS_RENDER_TEXTURE_FACTORY_H_
