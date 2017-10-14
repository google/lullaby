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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_H_

#include <memory>
#include "fplbase/asset_manager.h"
#include "fplbase/texture.h"
#include "lullaby/systems/render/texture.h"

namespace lull {

// Wraps an fplbase::Texture.
class Texture {
 public:
  Texture() {}
  Texture(const Texture& rhs) = delete;
  Texture& operator=(const Texture& rhs) = delete;

  // Binds the texture to the specified texture unit for rendering.
  void Bind(int unit);

  // Returns True if this texture has been loaded into OpenGL.
  bool IsLoaded() const;

  // Add a function that will be called textures loaded from file are done
  // loading.
  void AddOnLoadCallback(const std::function<void()>& callback);

  // Gets the dimensions of the underlying texture.
  mathfu::vec2i GetDimensions() const;

  // Returns the name of the texture.
  std::string GetName() const;

  // Returns true if the Texture is referencing a subtexture in a texture atlas.
  bool IsSubtexture() const;

  // Gets the UV bounds of a subtexture.
  const mathfu::vec4& UvBounds() const;

  // Returns the clamp bounds of a subtexture.
  mathfu::vec4 CalculateClampBounds() const;

  // Returns whether the texture has mips.
  bool HasMips() const;

  // Returns the GL resource id.
  fplbase::TextureHandle GetResourceId() const;

 private:
  friend class TextureFactory;
  void Init(std::unique_ptr<fplbase::Texture> texture_impl);
  void Init(std::shared_ptr<Texture> containing_texture,
            const mathfu::vec4& uv_bounds);

  std::unique_ptr<fplbase::Texture> impl_;
  std::shared_ptr<Texture> containing_texture_;
  mathfu::vec4 uv_bounds_ = mathfu::vec4(0, 0, 1, 1);
  std::vector<std::function<void()>> on_load_callbacks_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_H_
