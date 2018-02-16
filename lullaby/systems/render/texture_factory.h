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

#ifndef LULLABY_SYSTEMS_RENDER_TEXTURE_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_TEXTURE_FACTORY_H_

#include "lullaby/modules/render/image_data.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/typeid.h"
#include "lullaby/generated/texture_def_generated.h"

namespace lull {

/// Provides mechanisms for creating and managing Texture objects.
///
/// The TextureFactory can be used to create Texture objects from either CPU
/// memory (via an ImageData object) or from disk. It also provides a caching
/// mechanism whereby multiple requests to a texture identified by a unique
/// name will return the same Texture object.
class TextureFactory {
 public:
  /// Information on how to configure the texture on the GPU.
  struct CreateParams {
    Optional<ImageData::Format> format;
    TextureFiltering min_filter = TextureFiltering_NearestMipmapLinear;
    TextureFiltering mag_filter = TextureFiltering_Linear;
    TextureWrap wrap_s = TextureWrap_Repeat;
    TextureWrap wrap_t = TextureWrap_Repeat;
    bool premultiply_alpha = true;
    bool generate_mipmaps = false;
    bool is_cubemap = false;
  };

  virtual ~TextureFactory() {}

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
                                   const CreateParams& params) = 0;

  /// Creates a "named" texture using the |image| data and configured on the GPU
  /// using the creation |params|. Subsequent calls to this function with the
  /// same texture |name| will return the original texture as long as any
  /// references to that texture are still alive.
  virtual TexturePtr CreateTexture(HashValue name, ImageData image,
                                   const CreateParams& params) = 0;

  /// Loads a texture off disk with the given |filename| and uses the creation
  /// |params| to configure it for the GPU. The filename is also used as the
  /// "name" of the texture. Subsequent calls to this function with the same
  /// |filename| will return the original texture as long as any references to
  /// that texture are still valid.
  virtual TexturePtr LoadTexture(string_view filename,
                                 const CreateParams& params) = 0;

  /// Updates the entire image contents of |texture| using |image|. The image
  /// data is sent as-is (this does not perform alpha premultiplication).
  /// Returns false on error, which can depend on the backend but likely
  /// includes size or format mismatch.
  virtual bool UpdateTexture(TexturePtr texture, ImageData image) = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextureFactory);

#endif  // LULLABY_SYSTEMS_RENDER_TEXTURE_FACTORY_H_
