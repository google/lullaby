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

#include "redux/engines/render/filament/filament_texture.h"

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "filament/Texture.h"
#include "filament/TextureSampler.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/math/vector.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/graphics/image_utils.h"
#include "redux/modules/graphics/ktx_container.h"

namespace redux {

using ImageDataPtr = std::shared_ptr<ImageData>;

static filament::Texture::InternalFormat ToFilamentTextureInternalFormat(
    ImageFormat format, const TextureParams& params) {
  switch (format) {
    case ImageFormat::Rgba8888:
      return params.color_space == ColorSpace::sRGB
                 ? filament::Texture::InternalFormat::SRGB8_A8
                 : filament::Texture::InternalFormat::RGBA8;
    case ImageFormat::Rgb888:
      return params.color_space == ColorSpace::sRGB
                 ? filament::Texture::InternalFormat::SRGB8
                 : filament::Texture::InternalFormat::RGB8;
    case ImageFormat::Rgba5551:
      return filament::Texture::InternalFormat::RGB5_A1;
    case ImageFormat::Rgb565:
      return filament::Texture::InternalFormat::RGB565;
    case ImageFormat::Alpha8:
      return filament::Texture::InternalFormat::R8;
    case ImageFormat::Luminance8:
      return filament::Texture::InternalFormat::RG8;
    case ImageFormat::LuminanceAlpha88:
      return filament::Texture::InternalFormat::RG8;
    case ImageFormat::Rgb_11_11_10f:
      return filament::Texture::InternalFormat::R11F_G11F_B10F;
    default:
      LOG(FATAL) << "Unhandled format: " << ToString(format);
      return filament::Texture::InternalFormat::RGB8;
  }
}

static filament::Texture::Format ToFilamentTextureFormat(ImageFormat format) {
  switch (format) {
    case ImageFormat::Rgba8888:
      return filament::Texture::Format::RGBA;
    case ImageFormat::Rgb888:
      return filament::Texture::Format::RGB;
    case ImageFormat::Rgba5551:
      return filament::Texture::Format::RGBA;
    case ImageFormat::Rgb565:
      return filament::Texture::Format::RGB;
    case ImageFormat::Alpha8:
      return filament::Texture::Format::R;
    case ImageFormat::Luminance8:
      return filament::Texture::Format::R;
    case ImageFormat::LuminanceAlpha88:
      return filament::Texture::Format::RG;
    case ImageFormat::Rgb_11_11_10f:
      return filament::Texture::Format::RGB;
    default:
      LOG(FATAL) << "Unhandled format: " << ToString(format);
      return filament::Texture::Format::RGBA;
  }
}

static filament::Texture::Type ToFilamentTextureType(ImageFormat format) {
  switch (format) {
    case ImageFormat::Rgba8888:
      return filament::Texture::Type::UBYTE;
    case ImageFormat::Rgb888:
      return filament::Texture::Type::UBYTE;
    case ImageFormat::Rgba5551:
      return filament::Texture::Type::USHORT;
    case ImageFormat::Rgb565:
      return filament::Texture::Type::USHORT;
    case ImageFormat::Alpha8:
      return filament::Texture::Type::UBYTE;
    case ImageFormat::Luminance8:
      return filament::Texture::Type::UBYTE;
    case ImageFormat::LuminanceAlpha88:
      return filament::Texture::Type::UBYTE;
    case ImageFormat::Rgb_11_11_10f:
      return filament::Texture::Type::UINT_10F_11F_11F_REV;
    default:
      LOG(FATAL) << "Unhandled format: " << ToString(format);
      return filament::Texture::Type::UBYTE;
  }
}

static filament::TextureSampler::MinFilter ToFilamentMinFilter(
    TextureFilter value) {
  switch (value) {
    case TextureFilter::Nearest:
      return filament::TextureSampler::MinFilter::NEAREST;
    case TextureFilter::Linear:
      return filament::TextureSampler::MinFilter::LINEAR;
    case TextureFilter::LinearMipmapLinear:
      return filament::TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR;
    case TextureFilter::LinearMipmapNearest:
      return filament::TextureSampler::MinFilter::LINEAR_MIPMAP_NEAREST;
    case TextureFilter::NearestMipmapNearest:
      return filament::TextureSampler::MinFilter::NEAREST_MIPMAP_NEAREST;
    case TextureFilter::NearestMipmapLinear:
      return filament::TextureSampler::MinFilter::NEAREST_MIPMAP_LINEAR;
    default:
      LOG(FATAL) << "Unsupported filter: " << ToString(value);
  }
}

static filament::TextureSampler::MagFilter ToFilamentMagFilter(
    TextureFilter value) {
  switch (value) {
    case TextureFilter::Nearest:
      return filament::TextureSampler::MagFilter::NEAREST;
    case TextureFilter::Linear:
      return filament::TextureSampler::MagFilter::LINEAR;
    default:
      LOG(FATAL) << "Unsupported filter: " << ToString(value);
  }
}

static filament::TextureSampler::WrapMode ToFilamentWrapMode(
    TextureWrap value) {
  switch (value) {
    case TextureWrap::Repeat:
      return filament::TextureSampler::WrapMode::REPEAT;
    case TextureWrap::ClampToEdge:
      return filament::TextureSampler::WrapMode::CLAMP_TO_EDGE;
    case TextureWrap::MirroredRepeat:
      return filament::TextureSampler::WrapMode::MIRRORED_REPEAT;
    default:
      LOG(FATAL) << "Unsupported wrap mode: " << ToString(value);
  }
}

static filament::Texture::PixelBufferDescriptor CreatePixelBuffer(
    const ImageDataPtr& image_data) {
  // We allocate a ImageDataPtr to extend the lifetime of the image data until
  // filament is done with it.
  void* user_data = new ImageDataPtr(image_data);
  auto callback = [](void*, size_t, void* user) {
    ImageDataPtr* image = reinterpret_cast<ImageDataPtr*>(user);
    delete image;
  };

  const std::byte* bytes = image_data->GetData();
  const size_t num_bytes = image_data->GetNumBytes();
  const auto format = ToFilamentTextureFormat(image_data->GetFormat());
  const auto type = ToFilamentTextureType(image_data->GetFormat());
  return filament::Texture::PixelBufferDescriptor(bytes, num_bytes, format,
                                                  type, callback, user_data);
}

FilamentTexture::FilamentTexture(Registry* registry, std::string_view name)
    : name_(name) {
  fengine_ = GetFilamentEngine(registry);
}

const std::string& FilamentTexture::GetName() const { return name_; }

TextureTarget FilamentTexture::GetTarget() const { return target_; }

vec2i FilamentTexture::GetDimensions() const { return dimensions_; }

void FilamentTexture::Update(ImageData image) {
  Update(std::make_shared<ImageData>(std::move(image)));
}

void FilamentTexture::Build(ImageData image_data, const TextureParams& params) {
  Build(std::make_shared<ImageData>(std::move(image_data)), params);
}

void FilamentTexture::Build(const std::shared_ptr<ImageData>& image_data,
                            const TextureParams& params) {
  CHECK(image_data);
  CHECK(ftexture_ == nullptr);
  target_ = params.target;

  fsampler_.setMinFilter(ToFilamentMinFilter(params.min_filter));
  fsampler_.setMagFilter(ToFilamentMagFilter(params.mag_filter));
  fsampler_.setWrapModeR(ToFilamentWrapMode(params.wrap_r));
  fsampler_.setWrapModeS(ToFilamentWrapMode(params.wrap_s));
  fsampler_.setWrapModeT(ToFilamentWrapMode(params.wrap_t));

  filament::Texture::Builder builder;
  if (target_ == TextureTarget::CubeMap) {
    builder.sampler(filament::Texture::Sampler::SAMPLER_CUBEMAP);
  } else {
    builder.sampler(filament::Texture::Sampler::SAMPLER_2D);
  }

  if (IsContainerFormat(image_data->GetFormat())) {
    CHECK(image_data->GetFormat() == ImageFormat::Ktx);
    KtxContainer ktx(image_data);
    const KtxContainer::Header& header = ktx.GetHeader();
    if (target_ == TextureTarget::CubeMap) {
      CHECK_EQ(header.faces, 6);
    } else {
      CHECK_EQ(header.faces, 1);
    }

    const ImageFormat format = ktx.GetImageFormat();
    builder.width(header.width);
    builder.height(header.height);
    builder.levels(header.mip_levels);
    builder.format(ToFilamentTextureInternalFormat(format, params));

    auto sh_data = ktx.GetMetadata("sh");
    if (!sh_data.empty()) {
      const char* src = reinterpret_cast<const char*>(sh_data.data());

      // 3 bands, 9 RGB coefficients for a total of 27 floats.
      spherical_harmonics_.resize(9);
      for (auto& coeff : spherical_harmonics_) {
        for (int i = 0; i < 3; ++i) {
          char* next = nullptr;
          coeff[i] = std::strtof(src, &next);
          CHECK_NE(next, src) << "Invalid spherical harmonics data.";
          src = next;
        }
      }
    }
  } else {
    const ImageFormat format = image_data->GetFormat();
    builder.width(image_data->GetSize().x);
    builder.height(image_data->GetSize().y);
    builder.format(ToFilamentTextureInternalFormat(format, params));
  }

  ftexture_ = MakeFilamentResource(builder.build(*fengine_), fengine_);
  if (image_data->GetNumBytes() > 0) {
    Update(image_data);
  }
  if (params.generate_mipmaps) {
    ftexture_->generateMipmaps(*fengine_);
  }
  dimensions_ = vec2i(ftexture_->getWidth(), ftexture_->getHeight());
  NotifyReady();
}

void FilamentTexture::Update(const std::shared_ptr<ImageData>& image_data) {
  CHECK(image_data);
  CHECK_GT(image_data->GetNumBytes(), 0);
  CHECK(ftexture_ != nullptr);

  if (IsContainerFormat(image_data->GetFormat())) {
    CHECK(image_data->GetFormat() == ImageFormat::Ktx);
    KtxContainer ktx(image_data);
    const KtxContainer::Header& header = ktx.GetHeader();

    for (int level = 0; level < header.mip_levels; ++level) {
      if (target_ == TextureTarget::CubeMap) {
        for (int face = 0; face < 6; ++face) {
          auto image_data =
              std::make_shared<ImageData>(ktx.GetImage(level, face));
          const vec2i size = image_data->GetSize();
          ftexture_->setImage(*fengine_, level, 0, 0, face, size.x, size.y, 1,
                              CreatePixelBuffer(image_data));
        }
      } else {
        auto image_data = std::make_shared<ImageData>(ktx.GetImage(level, 0));
        ftexture_->setImage(*fengine_, level, CreatePixelBuffer(image_data));
      }
    }
  } else {
    if (target_ == TextureTarget::CubeMap) {
      const size_t face_size = image_data->GetNumBytes() / 6;
      ftexture_->setImage(*fengine_, 0, CreatePixelBuffer(image_data),
                          filament::Texture::FaceOffsets(face_size));
    } else {
      ftexture_->setImage(*fengine_, 0, CreatePixelBuffer(image_data));
    }
  }
}

}  // namespace redux
