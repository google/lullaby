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
#include "lullaby/util/logging.h"

namespace lull {

void Texture::Init(std::unique_ptr<fplbase::Texture> texture_impl) {
  DCHECK(fplbase::ValidTextureHandle(texture_impl->id()));
  impl_ = std::move(texture_impl);
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

void Texture::Bind(int unit) {
  if (impl_) {
    impl_->Set(unit);
  } else if (containing_texture_) {
    containing_texture_->Bind(unit);
  }
}

bool Texture::IsLoaded() const {
  if (impl_) {
    return true;
  } else if (containing_texture_) {
    return containing_texture_->IsLoaded();
  } else {
    return false;
  }
}

void Texture::AddOnLoadCallback(const std::function<void()>& callback) {
  if (containing_texture_) {
    containing_texture_->AddOnLoadCallback(callback);
  } else if (impl_) {
    callback();
  } else {
    on_load_callbacks_.push_back(callback);
  }
}

mathfu::vec2i Texture::GetDimensions() const {
  if (impl_) {
    return impl_->size();
  } else if (containing_texture_) {
    return containing_texture_->GetDimensions();
  } else {
    return mathfu::kZeros2i;
  }
}

std::string Texture::GetName() const {
  if (impl_) {
    std::string name = impl_->filename();
    if (!name.empty()) {
      return name;
    }
  } else if (containing_texture_) {
    return containing_texture_->GetName();
  }
  return "anonymous texture";
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
  if (impl_) {
    return (impl_->flags() & fplbase::kTextureFlagsUseMipMaps) != 0;
  } else if (containing_texture_) {
    return containing_texture_->HasMips();
  } else {
    return false;
  }
}

fplbase::TextureHandle Texture::GetResourceId() const {
  if (impl_) {
    return impl_->id();
  } else if (containing_texture_) {
    return containing_texture_->GetResourceId();
  } else {
    return fplbase::InvalidTextureHandle();
  }
}

}  // namespace lull
