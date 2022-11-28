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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_TEXTURE_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_TEXTURE_H_

#include <memory>
#include <string>

#include "filament/Texture.h"
#include "filament/TextureSampler.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/texture.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/image_data.h"

namespace redux {

// Manages a filament Texture.
class FilamentTexture : public Texture, public Readiable {
 public:
  FilamentTexture(Registry* registry, std::string_view name);

  // Returns the name of the texture.
  const std::string& GetName() const;

  // Returns the target type of the texture.
  TextureTarget GetTarget() const;

  // Gets the dimensions of the underlying image.
  vec2i GetDimensions() const;

  // Returns the underlying filament texture.
  filament::Texture* GetFilamentTexture() { return ftexture_.get(); }

  // Returns the underlying filament sampler.
  filament::TextureSampler GetFilamentSampler() { return fsampler_; }

  // Creates the actual underlying filament texture and sampler using the
  // `image_data` and `params`.
  void Build(ImageData image_data, const TextureParams& params);

  // Updates the entire contents of the texture. Image data is sent as-is.
  // Returns false if the dimensions don't match or if the texture isn't
  // internal and 2D.
  bool Update(ImageData image);

  void Build(const std::shared_ptr<ImageData>& image_data,
             const TextureParams& params);
  void Update(const std::shared_ptr<ImageData>& image_data);

 private:
  std::string name_;
  vec2i dimensions_;
  TextureTarget target_ = TextureTarget::Normal2D;
  filament::Engine* fengine_ = nullptr;
  filament::TextureSampler fsampler_;
  FilamentResourcePtr<filament::Texture> ftexture_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_TEXTURE_H_
