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

#include "lullaby/systems/render/next/texture.h"

#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/util/logging.h"
#include "mathfu/constants.h"

namespace lull {

Texture::~Texture() {
  if (hnd_ && !containing_texture_ && (flags_ & kIsExternal) == 0) {
    const auto handle = *hnd_;
    GL_CALL(glDeleteTextures(1, &handle));
  }
}

void Texture::Init(TextureHnd texture, Target texture_target,
                   const mathfu::vec2i& size, uint32_t flags) {
  hnd_ = texture;
  target_ = texture_target;
  size_ = size;
  flags_ = flags;
  for (auto& cb : on_load_callbacks_) {
    cb();
  }
  on_load_callbacks_.clear();
}

void Texture::Init(std::shared_ptr<Texture> containing_texture,
                   const mathfu::vec4& uv_bounds) {
  containing_texture_ = std::move(containing_texture);
  uv_bounds_ = uv_bounds;
  for (auto& cb : on_load_callbacks_) {
    containing_texture_->AddOnLoadCallback(cb);
  }
  on_load_callbacks_.clear();
}

bool Texture::IsLoaded() const {
  return containing_texture_ ? containing_texture_->IsLoaded() : hnd_.Valid();
}

void Texture::AddOnLoadCallback(const std::function<void()>& callback) {
  if (containing_texture_) {
    containing_texture_->AddOnLoadCallback(callback);
  } else if (IsLoaded()) {
    callback();
  } else {
    on_load_callbacks_.push_back(callback);
  }
}

mathfu::vec2i Texture::GetDimensions() const {
  return containing_texture_ ? containing_texture_->GetDimensions() : size_;
}

Texture::Target Texture::GetTarget() const {
  return containing_texture_ ? containing_texture_->GetTarget() : target_;
}

void Texture::SetName(const std::string& name) { name_ = name; }

std::string Texture::GetName() const {
  if (containing_texture_) {
    return containing_texture_->GetName();
  } else if (!name_.empty()) {
    return name_;
  } else {
    return "anonymous texture";
  }
}

bool Texture::IsSubtexture() const { return containing_texture_ != nullptr; }

const mathfu::vec4& Texture::UvBounds() const { return uv_bounds_; }

mathfu::vec4 Texture::CalculateClampBounds() const {
  const mathfu::vec2 size =
      mathfu::vec2::Max(mathfu::kOnes2f, mathfu::vec2(GetDimensions()));
  const mathfu::vec2 half_texel_size(0.5f / size.x, 0.5f / size.y);
  return mathfu::vec4(uv_bounds_.xy() + half_texel_size,
                      uv_bounds_.xy() + uv_bounds_.zw() - half_texel_size);
}

bool Texture::HasMips() const {
  if (containing_texture_) {
    return containing_texture_->HasMips();
  } else {
    return (flags_ & kHasMipMaps) != 0;
  }
}

TextureHnd Texture::GetResourceId() const {
  return containing_texture_ ? containing_texture_->GetResourceId() : hnd_;
}

bool IsTextureLoaded(const TexturePtr& texture) {
  return texture && texture->IsLoaded();
}

mathfu::vec2i GetTextureDimensions(const TexturePtr& texture) {
  return texture ? texture->GetDimensions() : mathfu::kZeros2i;
}

bool IsTextureExternalOes(const TexturePtr& texture) {
  DCHECK(texture);
#ifdef GL_TEXTURE_EXTERNAL_OES
  return texture->GetTarget() == GL_TEXTURE_EXTERNAL_OES;
#else  // GL_TEXTURE_EXTERNAL_OES
  return false;
#endif  // GL_TEXTURE_EXTERNAL_OES
}

Optional<int> GetTextureGlHandle(const TexturePtr& texture) {
  if (texture) {
    return static_cast<int>(*texture->GetResourceId());
  } else {
    return NullOpt;
  }
}

}  // namespace lull
