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

#ifndef LULLABY_SYSTEMS_RENDER_FPL_TEXTURE_H_
#define LULLABY_SYSTEMS_RENDER_FPL_TEXTURE_H_

#include <memory>
#include "fplbase/asset_manager.h"
#include "fplbase/texture.h"
#include "lullaby/systems/render/texture.h"

namespace lull {

// Wraps an fplbase::Texture but also allows us to use subtextures inside of a
// texture atlas with no differences to client code.
class Texture {
 public:
  // A unique_ptr to the underlying fplbase::Texture.
  typedef std::unique_ptr<fplbase::Texture,
                          std::function<void(const fplbase::Texture*)>>
      TextureImplPtr;

  // A unique_ptr to the underlying fplbase::TextureAtlas.
  typedef std::unique_ptr<fplbase::TextureAtlas,
                          std::function<void(const fplbase::TextureAtlas*)>>
      AtlasImplPtr;

  // Creates a Texture from its GL |texture_target| and |texture_id|.
  Texture(uint32_t texture_target, uint32_t texture_id);

  // Takes ownership of the specified FPL texture.
  explicit Texture(TextureImplPtr texture);

  // Takes ownership of the specified FPL texture.
  explicit Texture(AtlasImplPtr atlas);

  // Takes ownership of the specified FPL subtexture (which is part of a FPL
  // texture atlas).
  // Note: Because the actual texture is owned by the atlas, "ownership" in this
  // case is normally a unique_ptr with a no-op deleter.
  Texture(TextureImplPtr texture, const mathfu::vec4& uv_bounds);

  // Binds the texture to the specified texture unit for rendering.
  void Bind(int unit);

  // Returns True if this texture has been loaded into OpenGL.
  bool IsLoaded() const;

  // Add a function that will be called textures loaded from file are done
  // loading.
  void AddOnLoadCallback(
      const fplbase::AsyncAsset::AssetFinalizedCallback& callback);

  // Gets the dimensions of the underlying texture.
  mathfu::vec2i GetDimensions() const;

  // Returns the name of the texture.
  std::string GetName() const;

  // Returns true if the Texture is referencing a subtexture in a texture atlas.
  const bool IsSubtexture() const;

  // Gets the UV bounds of a subtexture.
  const mathfu::vec4& UvBounds() const;

  // Returns the clamp bounds of a subtexture.
  mathfu::vec4 CalculateClampBounds() const;

  // Returns whether the texture has mips.
  bool HasMips() const;

  // Returns the GL resource id.
  fplbase::TextureHandle GetResourceId() const;

 private:
  TextureImplPtr texture_impl_;
  AtlasImplPtr atlas_impl_;
  mathfu::vec4 uv_bounds_;
  bool is_subtexture_;

  Texture(const Texture& rhs) = delete;
  Texture& operator=(const Texture& rhs) = delete;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FPL_TEXTURE_H_
