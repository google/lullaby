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

#include "lullaby/systems/render/texture_factory.h"

#include "lullaby/modules/render/image_decode.h"

namespace lull {

TexturePtr TextureFactory::CreateTexture(const TextureDef* texture_def) {
  if (texture_def == nullptr) {
    return nullptr;
  }

  TextureParams params;
  if (texture_def) {
    params = TextureParams(*texture_def);
  }

  ImageData image;
  if (texture_def->data() && texture_def->data()->data()) {
    const uint8_t* bytes = texture_def->data()->data();
    const size_t num_bytes = texture_def->data()->size();
    image = DecodeImage(bytes, num_bytes, kDecodeImage_None);
  }

  if (!image.IsEmpty() && texture_def->name() && texture_def->name()->c_str()) {
    const HashValue name = Hash(texture_def->name()->c_str());
    return CreateTexture(name, std::move(image), params);
  } else if (!image.IsEmpty()) {
    return CreateTexture(std::move(image), params);
  } else if (texture_def->file() && texture_def->file()->c_str()) {
    return LoadTexture(texture_def->file()->c_str(), params);
  } else {
    LOG(DFATAL) << "TextureDef must contain either filename or image data!";
    return nullptr;
  }
}

TexturePtr TextureFactory::CreateTexture(const TextureDefT& texture_def) {
  const TextureParams params(texture_def);

  ImageData image;
  if (!texture_def.data.empty()) {
    const uint8_t* bytes = texture_def.data.data();
    const size_t num_bytes = texture_def.data.size();
    image = DecodeImage(bytes, num_bytes, kDecodeImage_None);
  }

  if (!image.IsEmpty() && !texture_def.name.empty()) {
    return CreateTexture(Hash(texture_def.name), std::move(image), params);
  } else if (!image.IsEmpty()) {
    return CreateTexture(std::move(image), params);
  } else if (!texture_def.file.empty()) {
    return LoadTexture(texture_def.file, params);
  } else {
    LOG(DFATAL) << "TextureDef must contain either filename or image data!";
    return nullptr;
  }
}

}  // namespace lull
