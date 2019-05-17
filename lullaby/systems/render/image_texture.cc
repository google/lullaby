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

#include "lullaby/systems/render/image_texture.h"

#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/texture_factory.h"

namespace lull {

void ImageTexture::SetData(const void* data, int x, int y, int width,
                           int height, size_t row_size_in_bytes) {
  CHECK_LE(x + width, image_.GetSize().x);
  CHECK_LE(y + height, image_.GetSize().y);

  const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
  uint8_t* dst = image_.GetMutableBytes() + y * image_.GetSize().x + x;

  for (int i = 0; i < height;
       ++i, dst += image_.GetSize().x, src += row_size_in_bytes) {
    memcpy(dst, src, width);
  }

  dirty_ = true;
}

void ImageTexture::UpdateTextureIfNecessary(Registry* registry) {
  if (!dirty_) {
    return;
  }

  if (texture_) {
    auto* texture_factory = registry->Get<TextureFactory>();
    if (texture_factory) {
      texture_factory->UpdateTexture(texture_, image_.CreateHeapCopy());
    } else {
      LOG(WARNING) << "Can't update texture; recreating";
      texture_.reset();
    }
  }

  if (!texture_) {
    auto* render_system = registry->Get<RenderSystem>();
    texture_ = render_system->CreateTexture(image_);
  }

  dirty_ = false;
}

}  // namespace lull
