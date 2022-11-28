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

#ifndef REDUX_ENGINES_RENDER_TEXTURE_H_
#define REDUX_ENGINES_RENDER_TEXTURE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Textures contain pixel data that can be used for a variety of purposes by a
// Shader when drawing a Renderable.
//
// The simplest example is where the entire contents of the texture are
// rendered "as-is" by a shader onto the screen. More complex situations are
// used for things like bump mapping, environmental lighting, etc.
class Texture {
 public:
  virtual ~Texture() = default;

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  // Returns the name of the texture.
  const std::string& GetName() const;

  // Returns the target type of the texture.
  TextureTarget GetTarget() const;

  // Gets the dimensions of the underlying image.
  vec2i GetDimensions() const;

  // Updates the entire contents of the texture. Image data is sent as-is.
  // Returns false if the dimensions don't match or if the texture isn't
  // internal and 2D.
  bool Update(ImageData image);

 protected:
  Texture() = default;
};

using TexturePtr = std::shared_ptr<Texture>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_TEXTURE_H_
