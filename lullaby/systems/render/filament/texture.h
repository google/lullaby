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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_TEXTURE_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_TEXTURE_H_

#include "filament/Texture.h"
#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/texture_params.h"
#include "lullaby/systems/render/filament/filament_utils.h"
#include "lullaby/systems/render/texture.h"

namespace lull {

// Image data used for rendering.
//
// Effectively a wrapper around Filament's Texture object with some additional
// functionality.
class Texture {
 public:
  Texture() {}

  // Returns the dimensions of the stored texture.
  mathfu::vec2i GetDimensions() const;

  // Returns the texture parameters.
  const TextureParams& GetTextureParams() const;

  // Returns true if the Texture is actually loaded into filament, false
  // otherwise (eg. texture is still decoding asynchronously).
  bool IsLoaded() const;

  // Registers a callback that will be invoked when the texture is fully loaded.
  // If the texture is already loaded, the callback is invoked immediately.
  void AddOrInvokeOnLoadCallback(const std::function<void()>& callback);

  // Returns the GL texture ID associated with this texture if it is an
  // external texture, 0 otherwise.
  int GetExternalTextureId() const;

  // Returns the underlying filament::Texture object.
  filament::Texture* GetFilamentTexture();

 private:
  friend class TextureFactoryImpl;
  using FTexturePtr = FilamentResourcePtr<filament::Texture>;

  void Init(FTexturePtr texture, const TextureParams& params,
            int external_texture_id = 0);

  FTexturePtr filament_texture_;
  TextureParams params_;
  int external_texture_id_ = 0;
  std::vector<std::function<void()>> on_load_callbacks_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_TEXTURE_H_
