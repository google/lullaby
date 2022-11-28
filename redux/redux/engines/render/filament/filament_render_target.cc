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

#include "redux/engines/render/filament/filament_render_target.h"

#include "redux/engines/platform/device_manager.h"
#include "redux/engines/render/filament/filament_render_engine.h"

namespace redux {

static filament::Texture::InternalFormat ToFilamentTextureInternalFormat(
    RenderTargetFormat format) {
  switch (format) {
    case RenderTargetFormat::Red8:
      return filament::Texture::InternalFormat::R8;
    case RenderTargetFormat::Rgb8:
      return filament::Texture::InternalFormat::RGB8;
    case RenderTargetFormat::Rgba8:
      return filament::Texture::InternalFormat::RGBA8;
    default:
      LOG(FATAL) << "Unsupported format: " << static_cast<int>(format);
  }
}

static filament::Texture::InternalFormat ToFilamentDepthStencilInternalFormat(
    RenderTargetDepthStencilFormat format) {
  switch (format) {
    case RenderTargetDepthStencilFormat::Depth16:
    case RenderTargetDepthStencilFormat::Depth24:
    case RenderTargetDepthStencilFormat::Depth24Stencil8:
      return filament::Texture::InternalFormat::DEPTH24_STENCIL8;
    case RenderTargetDepthStencilFormat::Depth32f:
      return filament::Texture::InternalFormat::DEPTH32F;
    case RenderTargetDepthStencilFormat::Depth32fStencil8:
      return filament::Texture::InternalFormat::DEPTH32F_STENCIL8;
    default:
      LOG(FATAL) << "Unsupported format: " << static_cast<int>(format);
  }
  return filament::Texture::InternalFormat::UNUSED;
}

FilamentRenderTarget::FilamentRenderTarget(Registry* registry)
    : registry_(registry) {
  auto display = registry_->Get<DeviceManager>()->Display();
  dimensions_ = display.GetProfile()->display_size;
}

FilamentRenderTarget::FilamentRenderTarget(Registry* registry,
                                           const RenderTargetParams& params)
    : registry_(registry) {
  fengine_ = redux::GetFilamentEngine(registry);
  color_format_ = params.texture_format;

  CreateColorAttachment(params);
  CreateDepthStencilAttachment(params);

  filament::RenderTarget::Builder builder;
  CHECK(fcolor_);
  builder.texture(filament::RenderTarget::AttachmentPoint::COLOR,
                  fcolor_.get());
  if (fdepth_stencil_) {
    builder.texture(filament::RenderTarget::AttachmentPoint::DEPTH,
                    fdepth_stencil_.get());
  }

  dimensions_ = params.dimensions;
  frender_target_ = MakeFilamentResource(builder.build(*fengine_), fengine_);
}

void FilamentRenderTarget::CreateColorAttachment(
    const RenderTargetParams& params) {
  filament::Texture::Builder builder;
  builder.width(params.dimensions.x);
  builder.height(params.dimensions.y);
  builder.format(ToFilamentTextureInternalFormat(params.texture_format));
  builder.sampler(filament::Texture::Sampler::SAMPLER_2D);
  builder.usage(filament::Texture::Usage::COLOR_ATTACHMENT);
  fcolor_ = MakeFilamentResource(builder.build(*fengine_), fengine_);
}

void FilamentRenderTarget::CreateDepthStencilAttachment(
    const RenderTargetParams& params) {
  const filament::Texture::InternalFormat format =
      ToFilamentDepthStencilInternalFormat(params.depth_stencil_format);

  if (format == filament::Texture::InternalFormat::UNUSED) {
    return;
  }

  static const filament::Texture::Usage kDepthStencilUsage =
      static_cast<filament::Texture::Usage>(
          static_cast<uint32_t>(
              filament::backend::TextureUsage::DEPTH_ATTACHMENT) |
          static_cast<uint32_t>(
              filament::backend::TextureUsage::STENCIL_ATTACHMENT));

  filament::Texture::Builder builder;
  builder.format(format);
  builder.width(params.dimensions.x);
  builder.height(params.dimensions.y);
  builder.levels(1);
  builder.usage(kDepthStencilUsage);
  fdepth_stencil_ = MakeFilamentResource(builder.build(*fengine_), fengine_);
}

vec2i FilamentRenderTarget::GetDimensions() const {
  return dimensions_;
}

RenderTargetFormat FilamentRenderTarget::GetRenderTargetFormat() const {
  return color_format_;
}

ImageData FilamentRenderTarget::GetFrameBufferData() {
  auto engine =
      static_cast<FilamentRenderEngine*>(registry_->Get<RenderEngine>());
  return engine->ReadPixels(this);
}

}  // namespace redux
