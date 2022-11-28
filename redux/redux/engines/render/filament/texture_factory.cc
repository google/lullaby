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

#include "redux/engines/render/texture_factory.h"

#include <utility>

#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/codecs/decode_image.h"
#include "redux/modules/graphics/enums.h"

namespace redux {

TextureFactory::TextureFactory(Registry* registry) : registry_(registry) {}

TexturePtr TextureFactory::GetTexture(HashValue name) const {
  return textures_.Find(name);
}

void TextureFactory::CacheTexture(HashValue name, const TexturePtr& texture) {
  textures_.Register(name, texture);
}

void TextureFactory::ReleaseTexture(HashValue name) { textures_.Release(name); }

TexturePtr TextureFactory::CreateTexture(ImageData image,
                                         const TextureParams& params) {
  auto texture = std::make_shared<FilamentTexture>(registry_, "");
  texture->Build(std::move(image), params);
  return std::static_pointer_cast<Texture>(texture);
}

TexturePtr TextureFactory::CreateTexture(HashValue name, ImageData image,
                                         const TextureParams& params) {
  TexturePtr texture = CreateTexture(std::move(image), params);
  CacheTexture(name, texture);
  return texture;
}

TexturePtr TextureFactory::CreateTexture(const vec2i& size, ImageFormat format,
                                         const TextureParams& params) {
  DataContainer data(nullptr, 0);
  ImageData empty(format, size, std::move(data));
  return CreateTexture(std::move(empty), params);
}

TexturePtr TextureFactory::LoadTexture(std::string_view uri,
                                       const TextureParams& params) {
  const HashValue key = Hash(uri);
  return textures_.Create(key, [=]() {
    auto texture = std::make_shared<FilamentTexture>(registry_, uri);

    std::shared_ptr<ImageData> image = std::make_shared<ImageData>();
    auto on_load = [=](AssetLoader::StatusOrData& asset) mutable {
      if (asset.ok()) {
        DecodeImageOptions opts;
        opts.premultiply_alpha = params.premultiply_alpha;
        *image = DecodeImage(*asset, opts);
      }
    };

    auto on_finalize = [=](AssetLoader::StatusOrData& asset) mutable {
      texture->Build(image, params);
    };

    auto asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync(uri, on_load, on_finalize);

    return std::static_pointer_cast<Texture>(texture);
  });
}

TexturePtr TextureFactory::MissingBlackTexture() {
  if (missing_black_ == nullptr) {
    // clang-format off
    uint8_t data[] = {
      0, 0, 0, 255,   0, 0, 0, 255,
      0, 0, 0, 255,   0, 0, 0, 255,
    };
    // clang-format on
    ImageData image(ImageFormat::Rgba8888, vec2i(2, 2),
                    DataContainer::WrapData(data, sizeof(data)));
    TextureParams params;
    params.target = TextureTarget::Normal2D;
    missing_black_ = CreateTexture(std::move(image), params);
  }
  return missing_black_;
}

TexturePtr TextureFactory::MissingWhiteTexture() {
  if (missing_white_ == nullptr) {
    // clang-format off
    uint8_t data[] = {
      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,
    };
    // clang-format on
    ImageData image(ImageFormat::Rgba8888, vec2i(2, 2),
                    DataContainer::WrapData(data, sizeof(data)));
    TextureParams params;
    params.target = TextureTarget::Normal2D;
    missing_white_ = CreateTexture(std::move(image), params);
  }
  return missing_white_;
}

TexturePtr TextureFactory::MissingNormalTexture() {
  if (missing_normal_ == nullptr) {
    // clang-format off
    uint8_t data[] = {
      127, 127, 127,   127, 127, 127,
      127, 127, 127,   127, 127, 127,
    };
    // clang-format on
    ImageData image(ImageFormat::Rgb888, vec2i(2, 2),
                    DataContainer::WrapData(data, sizeof(data)));
    TextureParams params;
    params.target = TextureTarget::Normal2D;
    missing_normal_ = CreateTexture(std::move(image), params);
  }
  return missing_normal_;
}

TexturePtr TextureFactory::DefaultEnvReflectionTexture() {
  if (default_env_reflection_ == nullptr) {
    // clang-format off
    uint8_t data[] = {
      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,

      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,

      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,

      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,

      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,

      255, 255, 255, 255,   255, 255, 255, 255,
      255, 255, 255, 255,   255, 255, 255, 255,
    };
    // clang-format on
    ImageData image(ImageFormat::Rgba8888, vec2i(2, 2),
                    DataContainer::WrapData(data, sizeof(data)));
    TextureParams params;
    params.target = TextureTarget::CubeMap;
    default_env_reflection_ = CreateTexture(std::move(image), params);
  }
  return default_env_reflection_;
}

}  // namespace redux
