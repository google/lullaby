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

#include "lullaby/systems/render/filament/texture.h"

#include "lullaby/modules/render/image_decode.h"
#include "lullaby/util/logging.h"

namespace lull {

void Texture::Init(FTexturePtr texture, const TextureParams& params,
                   int external_texture_id) {
  external_texture_id_ = external_texture_id;
  filament_texture_ = std::move(texture);
  params_ = params;

  for (auto& cb : on_load_callbacks_) {
    cb();
  }
  on_load_callbacks_.clear();
}

filament::Texture* Texture::GetFilamentTexture() {
  return filament_texture_.get();
}

const TextureParams& Texture::GetTextureParams() const {
  return params_;
}

mathfu::vec2i Texture::GetDimensions() const {
  if (filament_texture_) {
    const int w = static_cast<int>(filament_texture_->getWidth());
    const int h = static_cast<int>(filament_texture_->getHeight());
    return {w, h};
  }
  return {0, 0};
}

bool Texture::IsLoaded() const {
  return filament_texture_ != nullptr;
}

int Texture::GetExternalTextureId() const {
  return external_texture_id_;
}

void Texture::AddOrInvokeOnLoadCallback(const std::function<void()>& callback) {
  if (IsLoaded()) {
    callback();
  } else {
    on_load_callbacks_.push_back(callback);
  }
}

bool IsTextureLoaded(const TexturePtr& texture) {
  return texture && texture->GetFilamentTexture() != nullptr;
}

mathfu::vec2i GetTextureDimensions(const TexturePtr& texture) {
  return texture ? texture->GetDimensions() : mathfu::kZeros2i;
}

bool IsTextureExternalOes(const TexturePtr& texture) {
  return false;
}

Optional<int> GetTextureGlHandle(const TexturePtr& texture) {
  if (texture) {
    const int id = texture->GetExternalTextureId();
    if (id != 0) {
      return id;
    }
  }
  return NullOpt;
}

}  // namespace lull
