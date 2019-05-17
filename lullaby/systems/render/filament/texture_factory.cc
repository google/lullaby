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

#include "lullaby/systems/render/filament/texture_factory.h"

#include "filament/Stream.h"
#include "filament/Texture.h"
#include "backend/DriverEnums.h"
#include "image/KtxBundle.h"
#include "image/KtxUtility.h"
#include "fplbase/glplatform.h"
#include "lullaby/generated/flatbuffers/texture_def_generated.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/modules/render/image_decode_ktx.h"
#include "lullaby/modules/render/image_util.h"
#include "lullaby/util/filename.h"

namespace lull {

static int CreateExternalGlTexture() {
#ifdef GL_TEXTURE_EXTERNAL_OES
  GLuint texture_id;
  GL_CALL(glGenTextures(1, &texture_id));
  GL_CALL(glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id));
  GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                          GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                          GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
                          GL_LINEAR));
  return texture_id;
#else
  LOG(DFATAL) << "External textures are not available.";
  return 0;
#endif  // GL_TEXTURE_EXTERNAL_OES
}


template <typename FilamentEnum>
static FilamentEnum GetFilamentAstcEnum(int x, int y) {
  if (x == 4 && y == 4) {
    return FilamentEnum::RGBA_ASTC_4x4;
  } else if (x == 5 && y == 4) {
    return FilamentEnum::RGBA_ASTC_5x4;
  } else if (x == 5 && y == 5) {
    return FilamentEnum::RGBA_ASTC_5x5;
  } else if (x == 6 && y == 5) {
    return FilamentEnum::RGBA_ASTC_6x5;
  } else if (x == 6 && y == 6) {
    return FilamentEnum::RGBA_ASTC_6x6;
  } else if (x == 8 && y == 5) {
    return FilamentEnum::RGBA_ASTC_8x5;
  } else if (x == 8 && y == 6) {
    return FilamentEnum::RGBA_ASTC_8x6;
  } else if (x == 8 && y == 8) {
    return FilamentEnum::RGBA_ASTC_8x8;
  } else if (x == 10 && y == 5) {
    return FilamentEnum::RGBA_ASTC_10x5;
  } else if (x == 10 && y == 6) {
    return FilamentEnum::RGBA_ASTC_10x6;
  } else if (x == 10 && y == 8) {
    return FilamentEnum::RGBA_ASTC_10x8;
  } else if (x == 10 && y == 10) {
    return FilamentEnum::RGBA_ASTC_10x10;
  } else if (x == 12 && y == 10) {
    return FilamentEnum::RGBA_ASTC_12x10;
  } else if (x == 12 && y == 12) {
    return FilamentEnum::RGBA_ASTC_12x12;
  } else {
    LOG(DFATAL) << "Unsupported ASTC block size";
    return FilamentEnum::EAC_R11;
  }
}

static filament::Texture::InternalFormat ToFilamentTextureInternalFormat(
    const ImageData& image) {
  const ImageData::Format format = image.GetFormat();
  switch (format) {
    case ImageData::Format::kRgba8888:
      return filament::Texture::InternalFormat::RGBA8;
    case ImageData::Format::kRgb888:
      return filament::Texture::InternalFormat::RGB8;
    case ImageData::Format::kRgba5551:
      return filament::Texture::InternalFormat::RGB5_A1;
    case ImageData::Format::kRgb565:
      return filament::Texture::InternalFormat::RGB565;
    case ImageData::Format::kLuminance:
      return filament::Texture::InternalFormat::RG8;
    case ImageData::Format::kLuminanceAlpha:
      return filament::Texture::InternalFormat::RG8;
    case ImageData::Format::kAstc: {
      const AstcHeader* header =
          GetAstcHeader(image.GetBytes(), image.GetDataSize());
      return GetFilamentAstcEnum<filament::Texture::InternalFormat>(
          header->blockdim_x, header->blockdim_y);
    }
    default:
      LOG(DFATAL) << "Unhandled format in ToFilamentTextureInternalFormat";
      return filament::Texture::InternalFormat::RGB8;
  }
}

static filament::Texture::Format ToFilamentTextureFormat(
    ImageData::Format format) {
  switch (format) {
    case ImageData::Format::kRgba8888:
      return filament::Texture::Format::RGBA;
    case ImageData::Format::kRgb888:
      return filament::Texture::Format::RGB;
    case ImageData::Format::kRgba5551:
      return filament::Texture::Format::RGBA;
    case ImageData::Format::kRgb565:
      return filament::Texture::Format::RGB;
    case ImageData::Format::kLuminance:
      return filament::Texture::Format::R;
    case ImageData::Format::kLuminanceAlpha:
      return filament::Texture::Format::RG;
    default:
      LOG(DFATAL) << "Unhandled format in ToFilamentTextureFormat: " << format;
      return filament::Texture::Format::RGBA;
  }
}

static filament::Texture::Type ToFilamentTextureType(ImageData::Format format) {
  switch (format) {
    case ImageData::Format::kRgba8888:
      return filament::Texture::Type::UBYTE;
    case ImageData::Format::kRgb888:
      return filament::Texture::Type::UBYTE;
    case ImageData::Format::kRgba5551:
      return filament::Texture::Type::USHORT;
    case ImageData::Format::kRgb565:
      return filament::Texture::Type::USHORT;
    case ImageData::Format::kLuminance:
      return filament::Texture::Type::UBYTE;
    case ImageData::Format::kLuminanceAlpha:
      return filament::Texture::Type::UBYTE;
    default:
      LOG(DFATAL) << "Unhandled format in ToFilamentTextureType()";
      return filament::Texture::Type::UBYTE;
  }
}

static void ImageDataDeallocator(void*, size_t, void* user) {
  auto* image = reinterpret_cast<ImageData*>(user);
  delete image;
}

static filament::Texture::PixelBufferDescriptor CreatePixelBuffer(
    const void* bytes, size_t num_bytes,
    filament::backend::PixelDataFormat format,
    filament::backend::PixelDataType type, ImageData* src) {
  filament::backend::BufferDescriptor::Callback callback = nullptr;
  if (src) {
    callback = ImageDataDeallocator;
  }
  return filament::Texture::PixelBufferDescriptor(bytes, num_bytes, format,
                                                  type, callback, src);
}

static filament::Texture::PixelBufferDescriptor CreateCompressedPixelBuffer(
    const void* bytes, size_t num_bytes,
    filament::backend::CompressedPixelDataType type, ImageData* src) {
  filament::backend::BufferDescriptor::Callback callback = nullptr;
  if (src) {
    callback = ImageDataDeallocator;
  }
  const uint32_t image_size = static_cast<uint32_t>(num_bytes);
  return filament::Texture::PixelBufferDescriptor(bytes, num_bytes, type,
                                                  image_size, callback, src);
}

static filament::Texture* CreateTextureImpl(filament::Engine* engine,
                                            const ImageData& image_data,
                                            const TextureParams& params) {
  if (image_data.GetFormat() == ImageData::kKtx) {
    image::KtxBundle* ktx_bundle = new image::KtxBundle(
        image_data.GetBytes(), static_cast<uint32_t>(image_data.GetDataSize()));
    const bool is_srgb = false;
    return image::KtxUtility::createTexture(engine, ktx_bundle, is_srgb,
                                            params.is_rgbm);
  }

  filament::Texture::Builder builder;
  builder.width(image_data.GetSize().x);
  builder.height(image_data.GetSize().y);
  builder.format(ToFilamentTextureInternalFormat(image_data));
  if (params.is_cubemap) {
    builder.sampler(filament::Texture::Sampler::SAMPLER_CUBEMAP);
  }
  if (params.is_rgbm) {
    builder.rgbm(true);
  }
  filament::Texture* texture = builder.build(*engine);

  ImageData* copy = new ImageData(image_data.CreateHeapCopy());
  const uint8_t* bytes = copy->GetBytes();
  const size_t num_bytes = copy->GetDataSize();

  if (copy->GetFormat() == ImageData::Format::kAstc) {
    const AstcHeader* header =
        GetAstcHeader(copy->GetBytes(), copy->GetDataSize());
    const auto type =
        GetFilamentAstcEnum<filament::backend::CompressedPixelDataType>(
            header->blockdim_x, header->blockdim_y);
    const void* data = bytes + sizeof(AstcHeader);
    const size_t size = num_bytes - sizeof(AstcHeader);
    auto buffer = CreateCompressedPixelBuffer(data, size, type, copy);
    texture->setImage(*engine, 0, std::move(buffer));
  } else {
    const auto format = ToFilamentTextureFormat(copy->GetFormat());
    const auto type = ToFilamentTextureType(copy->GetFormat());
    auto buffer = CreatePixelBuffer(bytes, num_bytes, format, type, copy);
    texture->setImage(*engine, 0, std::move(buffer));
    if (params.generate_mipmaps) {
      texture->generateMipmaps(*engine);
    }
  }
  return texture;
}

TextureFactoryImpl::TextureFactoryImpl(Registry* registry,
                                       filament::Engine* engine)
    : registry_(registry),
      textures_(ResourceManager<Texture>::kWeakCachingOnly),
      engine_(engine) {
  white_texture_ = CreateTexture(CreateWhiteImage(), TextureParams());
#ifdef DEBUG
  invalid_texture_ = CreateTexture(CreateInvalidImage(), TextureParams());
#else
  invalid_texture_ = white_texture_;
#endif
}

TexturePtr TextureFactoryImpl::LoadTexture(string_view filename,
                                           const TextureParams& params) {
  const HashValue name = Hash(filename);
  return textures_.Create(name, [this, filename, &params]() {
    auto texture = std::make_shared<Texture>();

    auto* asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync<TextureAsset>(
        std::string(filename), params, [this, texture](TextureAsset* asset) {
          InitTextureImpl(texture, asset->image_data_, asset->params_);
        });
    return texture;
  });
}

void TextureFactoryImpl::LoadAtlas(const std::string& filename,
                                   const TextureParams& params) {
  LOG(DFATAL) << "Unimplemented.";
}

TexturePtr TextureFactoryImpl::CreateTexture(ImageData image,
                                             const TextureParams& params) {
  auto texture = std::make_shared<Texture>();
  InitTextureImpl(texture, image, params);
  return texture;
}

TexturePtr TextureFactoryImpl::CreateTexture(HashValue name, ImageData image,
                                             const TextureParams& params) {
  return textures_.Create(name, [&]() {
    return CreateTexture(std::move(image), params);
  });
}

TexturePtr TextureFactoryImpl::CreateExternalTexture() {
  return CreateExternalTexture(mathfu::vec2i(1280, 720));
}

TexturePtr TextureFactoryImpl::CreateExternalTexture(
    const mathfu::vec2i& size) {
  const int external_texture_id = CreateExternalGlTexture();
  filament::Stream::Builder stream_builder;
  stream_builder.width(size.x);
  stream_builder.height(size.y);
  stream_builder.stream(external_texture_id);
  filament::Stream* stream = stream_builder.build(*engine_);
  if (stream == nullptr) {
    return nullptr;
  }

  filament::Texture::Builder texture_builder;
  texture_builder.sampler(filament::Texture::Sampler::SAMPLER_EXTERNAL);
  texture_builder.format(filament::Texture::InternalFormat::RGB8);
  filament::Texture* filament_texture = texture_builder.build(*engine_);
  if (filament_texture == nullptr) {
    engine_->destroy(stream);
    return nullptr;
  }


  filament_texture->setExternalStream(*engine_, stream);

  filament::Engine* engine = engine_;
  Texture::FTexturePtr ptr(filament_texture,
                           [engine, stream](filament::Texture* obj) {
                             engine->destroy(stream);
                             engine->destroy(obj);
                           });

  TextureParams params;
  params.min_filter = TextureFiltering_Linear;
  params.mag_filter = TextureFiltering_Linear;
  params.wrap_s = TextureWrap_ClampToEdge;
  params.wrap_t = TextureWrap_ClampToEdge;

  auto texture = std::make_shared<Texture>();
  texture->Init(std::move(ptr), params, external_texture_id);
  return texture;
}

void TextureFactoryImpl::InitTextureImpl(TexturePtr texture,
                                         const ImageData& image,
                                         const TextureParams& params) {
  filament::Texture* filament_texture =
      CreateTextureImpl(engine_, image, params);
  if (filament_texture == nullptr) {
    return;
  }

  filament::Engine* engine = engine_;
  Texture::FTexturePtr ptr(filament_texture, [engine](filament::Texture* obj) {
    engine->destroy(obj);
  });
  texture->Init(std::move(ptr), params);
}

bool TextureFactoryImpl::UpdateTexture(TexturePtr texture, ImageData image) {
  LOG(FATAL) << "Unimplemented.";
  return false;
}

void TextureFactoryImpl::CacheTexture(HashValue name,
                                      const TexturePtr& texture) {
  textures_.Register(name, texture);
}

void TextureFactoryImpl::ReleaseTexture(HashValue name) {
  textures_.Release(name);
}

TexturePtr TextureFactoryImpl::GetTexture(HashValue name) const {
  return textures_.Find(name);
}

TexturePtr TextureFactoryImpl::GetWhiteTexture() const {
  return white_texture_;
}

TexturePtr TextureFactoryImpl::GetInvalidTexture() const {
  return invalid_texture_;
}

TexturePtr TextureFactoryImpl::CreateTextureDeprecated(
    const ImageData* image, const TextureParams& params) {
  auto texture = std::make_shared<Texture>();
  InitTextureImpl(texture, *image, params);
  return texture;
}

}  // namespace lull
