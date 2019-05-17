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

#include "lullaby/systems/render/texture_factory.h"

#include "lullaby/modules/render/image_decode.h"

namespace lull {

TextureAsset::TextureAsset(const TextureParams& params,
                           const Finalizer& finalizer, uint32_t decode_flags)
    : params_(params), finalizer_(finalizer), flags_(decode_flags) {}

TextureAsset::TextureAsset(const TextureParams& params,
                           const Finalizer& finalizer, const ErrorFn& error_fn,
                           uint32_t decode_flags)
    : params_(params),
      finalizer_(finalizer),
      error_fn_(error_fn),
      flags_(decode_flags) {}

ErrorCode TextureAsset::OnLoadWithError(const std::string& filename,
                                        std::string* data) {
  if (data->empty()) {
    return kErrorCode_InvalidArgument;
  }
  if (filename.find("cubemap") != std::string::npos) {
    params_.is_cubemap = true;
  }
  if (filename.find("nopremult") != std::string::npos) {
    params_.premultiply_alpha = false;
  }
  if (filename.find("rgbm") != std::string::npos) {
    params_.is_rgbm = true;
  }

  const auto* bytes = reinterpret_cast<const uint8_t*>(data->data());
  const size_t num_bytes = data->size();
  uint32_t flags = flags_;
  if (params_.premultiply_alpha) {
    flags |= kDecodeImage_PremultiplyAlpha;
  }

  // If this is an animated image, keep around the raw file bytes to pass off
  // to the decoder.
  if (IsAnimated(bytes, num_bytes)) {
    params_.generate_mipmaps = false;
    params_.premultiply_alpha = false;

    animated_image_ = LoadAnimatedImage(data);

    // Decode first frame while we're in the background thread.
    image_data_ = animated_image_->DecodeNextFrame();
    return kErrorCode_Ok;
  }

  image_data_ = DecodeImage(bytes, num_bytes, DecodeImageFlags(flags));
  if (image_data_.IsEmpty()) {
    LOG(ERROR) << "Unsupported texture file type.";
    return kErrorCode_InvalidArgument;
  }
  return kErrorCode_Ok;
}

void TextureAsset::OnFinalize(const std::string& filename, std::string* data) {
  if (!image_data_.IsEmpty() || animated_image_ != nullptr) {
    finalizer_(this);
  }
}

void TextureAsset::OnError(const std::string& filename, ErrorCode error) {
  if (error_fn_) {
    error_fn_(error);
  }
}

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

TexturePtr TextureFactory::LoadTexture(string_view filename) {
  return LoadTexture(filename, TextureParams());
}

}  // namespace lull
