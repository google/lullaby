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

#ifndef LULLABY_SYSTEMS_RENDER_IMAGE_TEXTURE_H_
#define LULLABY_SYSTEMS_RENDER_IMAGE_TEXTURE_H_

#include "lullaby/modules/render/image_data.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/registry.h"

namespace lull {

// An image-texture pair, where the complete image data is persistently stored
// in main memory, and the GPU resource is updated as necessary.
class ImageTexture {
 public:
  // Constructs from image data.
  ImageTexture(ImageData image) : image_(std::move(image)) {}

  // Returns the dimensions of the atlas.
  mathfu::vec2i GetSize() const { return image_.GetSize(); }

  // Sets a subrect of the atlas's data.
  void SetData(const void* data, int x, int y, int width, int height,
               size_t row_size_in_bytes);

  // Returns the texture created from this atlas.
  TexturePtr GetTexture() const { return texture_; }

  // Updates the texture using the latest data.
  void UpdateTextureIfNecessary(Registry* registry);

 private:
  ImageData image_;
  TexturePtr texture_;
  bool dirty_ = true;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_IMAGE_TEXTURE_H_
