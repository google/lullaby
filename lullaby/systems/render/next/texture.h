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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/systems/render/texture.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Represents a texture object used for rendering.
class Texture {
 public:
  // Wraps the GLenum for texture targets, eg. GL_TEXTURE_2D.  We use uint32_t
  // so that we do not have to depend on a GL header file.
  using Target = uint32_t;

  Texture() {}
  ~Texture();

  Texture(const Texture& rhs) = delete;
  Texture& operator=(const Texture& rhs) = delete;

  // Returns True if this texture has been loaded into OpenGL.
  bool IsLoaded() const;

  // Add a function that will be called textures loaded from file are done
  // loading.
  void AddOnLoadCallback(const std::function<void()>& callback);

  // Like the above, but invokes the callback immediately if the texture is
  // loaded.
  void AddOrInvokeOnLoadCallback(const std::function<void()>& callback) {
    if (IsLoaded()) {
      callback();
    } else {
      AddOnLoadCallback(callback);
    }
  }

  // Gets the dimensions of the underlying texture.
  mathfu::vec2i GetDimensions() const;

  // Sets the name for the texture.
  void SetName(const std::string& name);

  // Returns the name of the texture.
  std::string GetName() const;

  // Returns the target for this texture.
  Target GetTarget() const;

  // Returns true if the Texture is referencing a subtexture in a texture atlas.
  bool IsSubtexture() const;

  // Gets the UV bounds of a subtexture.
  const mathfu::vec4& UvBounds() const;

  // Returns the clamp bounds of a subtexture.
  mathfu::vec4 CalculateClampBounds() const;

  // Returns whether the texture has mips.
  bool HasMips() const;

  // Returns the GL resource id.
  TextureHnd GetResourceId() const;

 private:
  enum Flags {
    kIsExternal = 0x01 << 0,
    kHasMipMaps = 0x01 << 1,
  };

  friend class TextureFactoryImpl;
  void Init(TextureHnd texture, Target texture_target,
            const mathfu::vec2i& size, uint32_t flags);
  void Init(std::shared_ptr<Texture> containing_texture,
            const mathfu::vec4& uv_bounds);

  TextureHnd hnd_;
  Target target_ = 0;
  mathfu::vec2i size_ = {0, 0};
  uint32_t flags_ = 0;

  std::shared_ptr<Texture> containing_texture_;
  mathfu::vec4 uv_bounds_ = mathfu::vec4(0, 0, 1, 1);

  std::string name_;
  std::vector<std::function<void()>> on_load_callbacks_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_TEXTURE_H_
