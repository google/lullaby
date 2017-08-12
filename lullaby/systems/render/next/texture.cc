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

#include "lullaby/systems/render/next/texture.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"

namespace lull {

Texture::Texture(TextureImplPtr texture)
    : texture_impl_(std::move(texture)),
      uv_bounds_(0.f, 0.f, 1.f, 1.f),
      is_subtexture_(false) {}

Texture::Texture(AtlasImplPtr atlas)
    : atlas_impl_(std::move(atlas)),
      uv_bounds_(0.f, 0.f, 1.f, 1.f),
      is_subtexture_(false) {}

Texture::Texture(TextureImplPtr texture, const mathfu::vec4& uv_bounds)
    : texture_impl_(std::move(texture)),
      uv_bounds_(uv_bounds),
      is_subtexture_(true) {}

Texture::Texture(uint32_t texture_target, uint32_t texture_id)
    : texture_impl_(new fplbase::Texture(),
                    [](const fplbase::Texture* texture) { delete texture; }),
      uv_bounds_(0.f, 0.f, 1.f, 1.f),
      is_subtexture_(false) {
  // TODO(jsanmiya): change this constructor (and all calls above it) to
  // use rendering-system agnostic types fplbase::TextureTarget and
  // fplbase::TextureHandle.
  texture_impl_->SetTextureId(fplbase::TextureTargetFromGl(texture_target),
                              fplbase::TextureHandleFromGl(texture_id));
}

void Texture::Bind(int unit) { texture_impl_->Set(unit); }

bool Texture::IsLoaded() const {
  // The OnLoad callback is only called for textures that are loaded from files.
  // To avoid waiting forever for it to be called, we consider textures not
  // loaded from files to always be loaded.
  return fplbase::ValidTextureHandle(texture_impl_->id()) ||
         texture_impl_->filename().empty();
}

void Texture::AddOnLoadCallback(
    const fplbase::AsyncAsset::AssetFinalizedCallback& callback) {
  if (!IsLoaded()) {
    texture_impl_->AddFinalizeCallback(callback);
  }
}

mathfu::vec2i Texture::GetDimensions() const { return texture_impl_->size(); }

std::string Texture::GetName() const {
  if (texture_impl_) {
    std::string name = texture_impl_->filename();
    if (!name.empty()) {
      return name;
    }
  }
  return "anonymous texture";
}

const bool Texture::IsSubtexture() const { return is_subtexture_; }

const mathfu::vec4& Texture::UvBounds() const { return uv_bounds_; }

mathfu::vec4 Texture::CalculateClampBounds() const {
  const mathfu::vec2 size =
      mathfu::vec2::Max(mathfu::kOnes2f, mathfu::vec2(GetDimensions()));
  const mathfu::vec2 half_texel_size(0.5f / size.x, 0.5f / size.y);
  return mathfu::vec4(uv_bounds_.xy() + half_texel_size,
                      uv_bounds_.xy() + uv_bounds_.zw() - half_texel_size);
}

bool Texture::HasMips() const {
  return (texture_impl_->flags() & fplbase::kTextureFlagsUseMipMaps) != 0;
}

fplbase::TextureHandle Texture::GetResourceId() const {
  const fplbase::Texture* resource =
      atlas_impl_ ? atlas_impl_->atlas_texture() : texture_impl_.get();
  return resource ? resource->id() : fplbase::InvalidTextureHandle();
}

}  // namespace lull
