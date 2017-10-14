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

#include "lullaby/systems/render/next/texture_factory.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "fplbase/render_utils.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/file/file.h"
#include "lullaby/util/color.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace {

class TextureAsset : public Asset {
 public:
  TextureAsset(fplbase::TextureFlags flags,
               std::function<void(TextureAsset*)> finalizer)
      : flags_(flags), finalizer_(std::move(finalizer)) {}

  void OnLoad(const std::string& filename, std::string* data) override {
    uint8_t* bytes = nullptr;
    const std::string ext = GetExtensionFromFilename(filename);
    if (ext == ".astc") {
      bytes = fplbase::Texture::UnpackASTC(data->c_str(), data->size(), flags_,
                                           &size_, &format_);
    } else if (ext == ".pkm") {
      bytes = fplbase::Texture::UnpackPKM(data->c_str(), data->size(), flags_,
                                          &size_, &format_);
    } else if (ext == ".ktx") {
      bytes = fplbase::Texture::UnpackKTX(data->c_str(), data->size(), flags_,
                                          &size_, &format_);
    } else if (ext == ".png") {
      const mathfu::vec2 scale = mathfu::kOnes2f;
      bytes = fplbase::Texture::UnpackPng(data->c_str(), data->size(), scale,
                                          flags_, &size_, &format_);
    } else if (ext == ".jpg") {
      const mathfu::vec2 scale = mathfu::kOnes2f;
      bytes = fplbase::Texture::UnpackJpg(data->c_str(), data->size(), scale,
                                          flags_, &size_, &format_);
    } else if (ext == ".webp") {
      const mathfu::vec2 scale = mathfu::kOnes2f;
      bytes = fplbase::Texture::UnpackWebP(data->c_str(), data->size(), scale,
                                           flags_, &size_, &format_);
    } else {
      LOG(ERROR) << "Unsupported texture file type: " << ext;
    }
    if (bytes == nullptr) {
      LOG(ERROR) << "Unable to unpack texture data: " << ext;
      return;
    }
    image_data_ =
        std::unique_ptr<uint8_t[], std::function<void(const uint8_t*)>>(
            bytes, [](const uint8_t* ptr) {
              void* mem = const_cast<void*>(reinterpret_cast<const void*>(ptr));
              free(mem);
            });
  }

  void OnFinalize(const std::string& filename, std::string* data) override {
    finalizer_(this);
  }

  fplbase::TextureFlags flags_ = fplbase::TextureFlags(0);
  mathfu::vec2i size_ = mathfu::kZeros2i;
  fplbase::TextureFormat format_ = fplbase::kFormatAuto;
  std::unique_ptr<uint8_t[], std::function<void(const uint8_t*)>> image_data_;
  std::function<void(TextureAsset*)> finalizer_;
};

fplbase::TextureFlags GetTextureFlags(bool create_mips, bool async = false,
                                      bool is_cubemap = false,
                                      bool premultiply_alpha = true) {
  fplbase::TextureFlags none = fplbase::kTextureFlagsNone;
  return ((create_mips ? fplbase::kTextureFlagsUseMipMaps : none) |
          (is_cubemap ? (fplbase::kTextureFlagsIsCubeMap |
                         fplbase::kTextureFlagsClampToEdge)
                      : none) |
          (async ? fplbase::kTextureFlagsLoadAsync : none) |
          (premultiply_alpha ? fplbase::kTextureFlagsPremultiplyAlpha : none));
}

}  // namespace

TextureFactory::TextureFactory(Registry* registry, fplbase::Renderer* renderer)
    : registry_(registry), fpl_renderer_(renderer) {
  // Create placeholder white texture.
  {
    constexpr int kTextureSize = 2;
    const Color4ub data[kTextureSize * kTextureSize];
    white_texture_ = CreateTextureFromMemory(
        data, mathfu::vec2i(kTextureSize, kTextureSize), fplbase::kFormat8888);
  }
#ifdef DEBUG
  // Create placeholder "watermelon" texture.
  {
    constexpr int kTextureSize = 16;
    const Color4ub kUglyGreen(0, 255, 0, 255);
    const Color4ub kUglyPink(255, 0, 128, 255);
    Color4ub data[kTextureSize * kTextureSize];
    Color4ub* ptr = data;
    for (int y = 0; y < kTextureSize; ++y) {
      for (int x = 0; x < kTextureSize; ++x) {
        *ptr = ((x + y) % 2 == 0) ? kUglyGreen : kUglyPink;
        ++ptr;
      }
    }
    invalid_texture_ = CreateTextureFromMemory(
        data, mathfu::vec2i(kTextureSize, kTextureSize), fplbase::kFormat8888);
  }
#else
  invalid_texture_ = white_texture_;
#endif
}

TexturePtr TextureFactory::LoadTexture(const std::string& filename,
                                       bool create_mips) {
  std::string resolved = filename;
  if (EndsWith(resolved, ".astc") &&
      !fpl_renderer_->SupportsTextureFormat(fplbase::kFormatASTC)) {
    resolved = RemoveExtensionFromFilename(resolved) + ".webp";
  } else if (EndsWith(resolved, ".ktx") &&
             !fpl_renderer_->SupportsTextureFormat(fplbase::kFormatKTX)) {
    resolved = RemoveExtensionFromFilename(resolved) + ".webp";
  } else if (EndsWith(resolved, ".pkm") &&
             !fpl_renderer_->SupportsTextureFormat(fplbase::kFormatPKM)) {
    resolved = RemoveExtensionFromFilename(resolved) + ".webp";
  }

  const HashValue key = Hash(resolved.c_str());
  TexturePtr texture = textures_.Create(key, [&]() {
    const bool async = true;
    const bool is_cubemap = (resolved.find("cubemap") != std::string::npos);
    const bool is_nopremult = (resolved.find("nopremult") != std::string::npos);
    const auto flags =
        GetTextureFlags(create_mips, async, is_cubemap, !is_nopremult);

    auto texture = std::make_shared<Texture>();
    auto finalizer = [texture, resolved](TextureAsset* asset) {
      if (asset->image_data_) {
        auto impl = MakeUnique<fplbase::Texture>(
            resolved.c_str(), fplbase::kFormatNative, asset->flags_);
        impl->LoadFromMemory(asset->image_data_.get(), asset->size_,
                             asset->format_);
        texture->Init(std::move(impl));
      }
    };

    auto* asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync<TextureAsset>(resolved, flags,
                                          std::move(finalizer));
    return texture;
  });
  textures_.Release(key);
  return texture;
}

TexturePtr TextureFactory::CreateTextureFromMemory(
    const void* data, const mathfu::vec2i size, fplbase::TextureFormat format,
    bool create_mips) {
  const bool async = false;
  const bool is_cubemap = false;
  const auto flags = GetTextureFlags(create_mips, async, is_cubemap);
  auto impl = MakeUnique<fplbase::Texture>(nullptr, format, flags);
  impl->LoadFromMemory(static_cast<const uint8_t*>(data), size, format);

  auto texture = std::make_shared<Texture>();
  texture->Init(std::move(impl));
  return texture;
}


TexturePtr TextureFactory::CreateProcessedTexture(
    const TexturePtr& texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor,
    const mathfu::vec2i& output_dimensions) {
  LULLABY_CPU_TRACE_CALL();

  if (!texture) {
    LOG(DFATAL) << "null texture passed to CreateProcessedTexture()";
    return TexturePtr();
  }
  // Make and bind a framebuffer for rendering to texture.
  GLuint framebuffer_id, current_framebuffer_id;
  GL_CALL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                        reinterpret_cast<GLint*>(&current_framebuffer_id)));
  GL_CALL(glGenFramebuffers(1, &framebuffer_id));
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id));

  // Make an empty FPL texture for the render target, sized to the specified
  // dimensions.
  mathfu::vec2i size = output_dimensions;
  bool target_is_subtexture = false;
  float texture_u_bound = 1.f;
  float texture_v_bound = 1.f;

  // If the input texture is a subtexture, scale the size appropriately.
  if (texture->IsSubtexture()) {
    auto scale = mathfu::vec2(texture->UvBounds().z, texture->UvBounds().w);
    auto size_f = scale * mathfu::vec2(static_cast<float>(size.x),
                                       static_cast<float>(size.y));
    size =
        mathfu::vec2i(static_cast<int>(size_f.x), static_cast<int>(size_f.y));
  }

  // If we don't support NPOT and the texture is NPOT, use UV bounds to work
  // around this.
  if (!fpl_renderer_->SupportsTextureNpot() &&
      (!IsPowerOf2(size.x) || !IsPowerOf2(size.y))) {
    target_is_subtexture = true;
    uint32_t next_power_of_two_x =
        mathfu::RoundUpToPowerOf2(static_cast<uint32_t>(size.x));
    uint32_t next_power_of_two_y =
        mathfu::RoundUpToPowerOf2(static_cast<uint32_t>(size.y));
    texture_u_bound =
        static_cast<float>(size.x) / static_cast<float>(next_power_of_two_x);
    texture_v_bound =
        static_cast<float>(size.y) / static_cast<float>(next_power_of_two_y);
    size = mathfu::vec2i(next_power_of_two_x, next_power_of_two_y);
  }

  const bool async = false;
  const bool is_cubemap = false;
  const auto format = fplbase::kFormat8888;
  const auto flags = GetTextureFlags(create_mips, async, is_cubemap);
  auto impl = MakeUnique<fplbase::Texture>(nullptr, format, flags);
  impl->LoadFromMemory(nullptr, size, format);

  TexturePtr out_ptr = std::make_shared<Texture>();
  if (target_is_subtexture) {
    TexturePtr tmp = std::make_shared<Texture>();
    tmp->Init(std::move(impl));
    const mathfu::vec4 bounds(0.f, 0.f, texture_u_bound, texture_v_bound);
    out_ptr->Init(tmp, bounds);
  } else {
    out_ptr->Init(std::move(impl));
  }

  // Bind the output texture to the framebuffer as the color attachment.
  GL_CALL(glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
      fplbase::GlTextureHandle(out_ptr->GetResourceId()), 0));

#if defined(DEBUG)
  // Check for completeness of the framebuffer.
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG(DFATAL) << "Failed to create offscreen framebuffer: " << std::hex
                << glCheckFramebufferStatus(GL_FRAMEBUFFER);
  }
#endif

  // Subtexturing on output texture can pick up sample noise around the edges
  // of the rendered area. Clear to transparent black.
  if (target_is_subtexture) {
    GL_CALL(glClearColor(0.f, 0.f, 0.f, 0.f));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
  }

  processor(out_ptr);

  // Setup viewport, input texture, shader, and draw quad.
  fpl_renderer_->SetViewport(fplbase::Viewport(mathfu::kZeros2i, size));

  // We render a quad starting in the lower left corner and extending up and
  // right for as long as the output subtexture is needed.
  fplbase::RenderAAQuadAlongX(
      mathfu::vec3(-1.f, -1.f, 0.f),
      mathfu::vec3((texture_u_bound * 2.f) - 1.f, (texture_v_bound * 2.f) - 1.f,
                   0),
      mathfu::vec2(texture->UvBounds().x, texture->UvBounds().y),
      mathfu::vec2(texture->UvBounds().x + texture->UvBounds().z,
                   texture->UvBounds().y + texture->UvBounds().w));

  // Delete framebuffer, we retain the texture.
  GL_CALL(glDeleteFramebuffers(1, &framebuffer_id));

  // Regenerate Mipmaps on the processed texture.
  if (create_mips) {
    out_ptr->Bind(0);
    GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  }

  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, current_framebuffer_id));

  return out_ptr;
}

TexturePtr TextureFactory::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor) {
  auto size = source_texture->GetDimensions();
  return CreateProcessedTexture(source_texture, create_mips, processor, size);
}

TexturePtr TextureFactory::CreateTexture(uint32_t texture_target,
                                         uint32_t texture_id) {
  auto impl = MakeUnique<fplbase::Texture>();
  impl->SetTextureId(fplbase::TextureTargetFromGl(texture_target),
                     fplbase::TextureHandleFromGl(texture_id));

  auto texture = std::make_shared<Texture>();
  texture->Init(std::move(impl));
  return texture;
}

void TextureFactory::CreateSubtexture(HashValue key, const TexturePtr& texture,
                                      const mathfu::vec4& uv_bounds) {
  textures_.Create(key, [&]() {
    auto subtexture = std::make_shared<Texture>();
    subtexture->Init(texture, uv_bounds);
    return subtexture;
  });
}

void TextureFactory::CacheTexture(HashValue name, const TexturePtr& texture) {
  textures_.Create(name, [&]() { return texture; });
}

void TextureFactory::ReleaseTextureFromCache(HashValue texture_hash) {
  textures_.Release(texture_hash);
}

TexturePtr TextureFactory::GetCachedTexture(HashValue texture_hash) const {
  return textures_.Find(texture_hash);
}

const TexturePtr& TextureFactory::GetWhiteTexture() const {
  return white_texture_;
}

const TexturePtr& TextureFactory::GetInvalidTexture() const {
  return invalid_texture_;
}

}  // namespace lull
