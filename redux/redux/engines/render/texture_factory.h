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

#ifndef REDUX_ENGINES_RENDER_TEXTURE_FACTORY_H_
#define REDUX_ENGINES_RENDER_TEXTURE_FACTORY_H_

#include "redux/engines/render/texture.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/graphics/image_data.h"

namespace redux {

struct TextureParams {
  TextureParams() = default;

  TextureFilter min_filter = TextureFilter::Linear;
  TextureFilter mag_filter = TextureFilter::Linear;
  TextureWrap wrap_s = TextureWrap::Repeat;
  TextureWrap wrap_t = TextureWrap::Repeat;
  TextureWrap wrap_r = TextureWrap::Repeat;
  TextureTarget target = TextureTarget::Normal2D;
  bool premultiply_alpha = false;
  bool generate_mipmaps = false;
};

class TextureFactory {
 public:
  explicit TextureFactory(Registry* registry);

  TextureFactory(const TextureFactory&) = delete;
  TextureFactory& operator=(const TextureFactory&) = delete;

  // Returns the texture in the cache associated with |name|, else nullptr.
  TexturePtr GetTexture(HashValue name) const;

  // Attempts to add |texture| to the cache using |name|.
  void CacheTexture(HashValue name, const TexturePtr& texture);

  // Releases the cached texture associated with |name|.
  void ReleaseTexture(HashValue name);

  // Creates a texture using the specified data.
  TexturePtr CreateTexture(ImageData image, const TextureParams& params);

  // Creates a named texture using the specified data; automatically registered
  // with the factory.
  TexturePtr CreateTexture(HashValue name, ImageData image,
                           const TextureParams& params);

  // Creates an "empty" texture of the specified size. The |params| must
  // specify a format for the texture.
  TexturePtr CreateTexture(const vec2i& size, ImageFormat format,
                           const TextureParams& params);

  // Loads a texture off disk with the given |filename| and uses the creation
  // |params| to configure it for the GPU. The filename is also used as the
  // "name" of the texture. Subsequent calls to this function with the same
  // |filename| will return the original texture as long as any references to
  // that texture are still alive.
  TexturePtr LoadTexture(std::string_view uri, const TextureParams& params);

  // Some useful default/hard-coded textures.
  TexturePtr MissingBlackTexture();
  TexturePtr MissingWhiteTexture();
  TexturePtr MissingNormalTexture();
  TexturePtr DefaultEnvReflectionTexture();

 private:
  Registry* registry_ = nullptr;
  ResourceManager<Texture> textures_;
  TexturePtr missing_black_;
  TexturePtr missing_white_;
  TexturePtr missing_normal_;
  TexturePtr default_env_reflection_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::TextureFactory);

#endif  // REDUX_ENGINES_RENDER_TEXTURE_FACTORY_H_
