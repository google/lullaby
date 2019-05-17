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

#include "lullaby/systems/render/next/texture_factory.h"
#include <cmath>

#include "fplbase/texture_atlas_generated.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/render/animated_texture_processor.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/modules/render/image_decode_ktx.h"
#include "lullaby/modules/render/image_util.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/util/filename.h"

namespace lull {
namespace {

// TODO: as KTX is a container class there is no way to determine
// if the extension is really supported without loading at least part of the
// file. These flags are used as a workaround but a better design would be less
// error prone.
bool g_app_set_ktx_supported = false;
bool g_is_ktx_supported = true;

bool ValidateNumFaces(int num_faces) {
  if (num_faces != 1 && num_faces != 6) {
    LOG(DFATAL) << "Number of faces should be 1 or 6.";
    return false;
  }
  return true;
}

bool IsCompressed(ImageData::Format format) {
  switch (format) {
    case ImageData::kRgba5551:
    case ImageData::kRgb565:
    case ImageData::kAstc:
    case ImageData::kPkm:
    case ImageData::kKtx:
      return true;
    default:
      return false;
  }
}

bool IsKtxSupported() {
  // KTX can contain any image format, so we allow the application to set
  // whether or not to allow .ktx files.  If the application doesn't set a
  // value, we assume the .ktx files contain ASTC data and return true if
  // ASTC is supported.
  if (g_app_set_ktx_supported) {
    return g_is_ktx_supported;
  } else {
    return CpuAstcDecodingAvailable() || GlSupportsAstc();
  }
}

bool IsExtensionSupported(const std::string& ext) {
  if (ext == ".astc") {
    return CpuAstcDecodingAvailable() || GlSupportsAstc();
  } else if (ext == ".ktx") {
    return IsKtxSupported();
  } else if (ext == ".pkm") {
    return GlSupportsEtc2();
  }
  return true;
}

GLenum GetTextureFormat(ImageData::Format format) {
  switch (format) {
    case ImageData::kAlpha:
#ifdef FPLBASE_GLES
      return GL_ALPHA;
#else
      return GL_RED;
#endif
    case ImageData::kLuminance:
#ifdef FPLBASE_GLES
      return GL_LUMINANCE;
#else
      return GL_RED;
#endif
    case ImageData::kLuminanceAlpha:
#ifdef FPLBASE_GLES
      return GL_LUMINANCE_ALPHA;
#else
      return GL_RG;
#endif
    case ImageData::kRg88:
      return GL_RG;
    case ImageData::kRgb888:
    case ImageData::kRgb565:
      return GL_RGB;
    case ImageData::kRgba8888:
    case ImageData::kRgba4444:
    case ImageData::kRgba5551:
      return GL_RGBA;
    default:
      LOG(ERROR) << "Unknown pixel format for " << format;
      return GL_RGBA;
  }
}

GLenum GetPixelFormat(ImageData::Format format) {
  switch (format) {
    case ImageData::kAlpha:
    case ImageData::kLuminance:
    case ImageData::kLuminanceAlpha:
    case ImageData::kRg88:
    case ImageData::kRgb888:
    case ImageData::kRgba8888:
      return GL_UNSIGNED_BYTE;
    case ImageData::kRgb565:
      return GL_UNSIGNED_SHORT_5_6_5;
    case ImageData::kRgba4444:
      return GL_UNSIGNED_SHORT_4_4_4_4;
    case ImageData::kRgba5551:
      return GL_UNSIGNED_SHORT_5_5_5_1;
    default:
      LOG(ERROR) << "Unknown pixel type for " << format;
      return GL_UNSIGNED_BYTE;
  }
}

GLenum GetAstcFormat(const AstcHeader* header) {
  const int x = header->blockdim_x;
  const int y = header->blockdim_y;
  if (x == 4 && y == 4) {
    return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
  } else if (x == 5 && y == 4) {
    return GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
  } else if (x == 5 && y == 5) {
    return GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
  } else if (x == 6 && y == 5) {
    return GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
  } else if (x == 6 && y == 6) {
    return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
  } else if (x == 8 && y == 5) {
    return GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
  } else if (x == 8 && y == 6) {
    return GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
  } else if (x == 8 && y == 8) {
    return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
  } else if (x == 10 && y == 5) {
    return GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
  } else if (x == 10 && y == 6) {
    return GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
  } else if (x == 10 && y == 8) {
    return GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
  } else if (x == 10 && y == 10) {
    return GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
  } else if (x == 12 && y == 10) {
    return GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
  } else if (x == 12 && y == 12) {
    return GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
  } else {
    LOG(DFATAL) << "Unsupported ASTC block size";
    return GL_NONE;
  }
}

void WriteTexImage(const uint8_t* data, int num_bytes_per_face, int num_faces,
                   int width, int height, int mip_level, GLenum internal_format,
                   GLenum format, GLenum type, bool compressed) {
  if (!ValidateNumFaces(num_faces)) {
    return;
  }

  // TODO: It's possible for this to fail due to unsupported image
  // format.  This is particularly likely for KTX since we can't validate the
  // format up-front, but it can also happen in general and checking the set of
  // extensions is not sufficient to eliminate this possibility (b/80187457).
  // This code should handle format errors gracefully by reporting them and
  // falling-back to the invalid texture.
  const int border = 0;
  const GLenum target =
      (num_faces == 6) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
  for (int i = 0; i < num_faces; ++i) {
    if (compressed) {
      GL_CALL(glCompressedTexImage2D(target + i, mip_level, internal_format,
                                     width, height, border, num_bytes_per_face,
                                     data));
    } else {
      GL_CALL(glTexImage2D(target + i, mip_level, internal_format, width,
                           height, border, format, type, data));
    }
    if (data) {
      data += num_bytes_per_face;
    }
  }
}

struct GlTextureData {
  const uint8_t* bytes = nullptr;
  GLint num_bytes_per_face = 0;
  GLint num_faces = 1;
  GLint width = 0;
  GLint height = 0;
  GLenum type = GL_NONE;
  GLenum format = GL_NONE;
  bool generate_mipmaps = false;
  bool compressed = false;
  bool luminance = false;
};

void BuildGlTexture(const GlTextureData& data) {
  const int main_mip_level = 0;
  WriteTexImage(data.bytes, data.num_bytes_per_face, data.num_faces, data.width,
                data.height, main_mip_level, data.format, data.format,
                data.type, data.compressed);

#if !defined(FPLBASE_GLES) && !defined(PLATFORM_OSX) && \
    defined(GL_TEXTURE_SWIZZLE_A)
  if (data.luminance) {
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN));
  }
#endif

  if (data.generate_mipmaps && data.bytes != nullptr) {
    // Work around for some Android devices to correctly generate miplevels.
    // NOTE:  If client creates a texture with buffer == nullptr (i.e. to
    // render into later), and wants mipmapping, and is on a phone requiring
    // this workaround, the client will need to do this preallocation
    // workaround themselves.
    const int min_dimension = std::min(data.width, data.height);
    const int num_levels = static_cast<int>(std::ceil(
        std::log(static_cast<float>(min_dimension)) / std::log(2.0f)));

    int width = data.width;
    int height = data.height;
    for (int i = 1; i < num_levels; ++i) {
      width /= 2;
      height /= 2;
      WriteTexImage(nullptr, data.num_bytes_per_face, data.num_faces, width,
                    height, i, data.format, data.format, data.type,
                    data.compressed);
    }
    const GLenum target =
        (data.num_faces == 6) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GL_CALL(glGenerateMipmap(target));
  }
}

void BuildKtxTexture(const uint8_t* buffer, size_t len, bool generate_mipmaps) {
  const KtxHeader* header = GetKtxHeader(buffer, len);
  auto processor = [&](const uint8_t* data, size_t num_bytes_per_face,
                       size_t num_faces, mathfu::vec2i dimensions,
                       int mip_level, mathfu::vec2i block_size) {
    // Don't process additional mips.
    if (generate_mipmaps == false && mip_level > 0) {
      return;
    }

    const bool compressed = block_size.x > 1 || block_size.y > 1;
    WriteTexImage(data, static_cast<int>(num_bytes_per_face),
                  static_cast<int>(num_faces), dimensions.x, dimensions.y,
                  mip_level, header->internal_format, header->format,
                  header->type, compressed);
  };

  const int num_images_processed = ProcessKtx(header, processor);

  // Some GL drivers need to be explicitly told that we don't have a full mip
  // chain (down to 1x1).
  if (num_images_processed > 0) {
    const GLenum target =
        (header->faces == 6) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL,
                            num_images_processed - 1));
  }
}

TextureHnd CreateTextureHnd(const uint8_t* buffer, const size_t len,
                            ImageData::Format texture_format,
                            const mathfu::vec2i& size,
                            const TextureParams& params, int unpack_alignment,
                            int pixels_per_row = 0) {
  GlTextureData data;
  data.num_faces = params.is_cubemap ? 6 : 1;
  data.width = size.x;
  data.height = (params.is_cubemap && (texture_format != ImageData::kKtx))
                    ? (size.y / data.num_faces)
                    : size.y;
  data.generate_mipmaps = params.generate_mipmaps;
  data.compressed = IsCompressed(texture_format);

  if (params.generate_mipmaps && data.compressed &&
      texture_format != ImageData::kKtx) {
    LOG(ERROR) << "Can't generate mipmaps for compressed textures";
    data.generate_mipmaps = false;
  }
  if (params.is_cubemap && data.width != data.height) {
    LOG(ERROR) << "Cubemap not in 1x6 format: " << size.x << "x" << size.y;
  }

  if (!GlSupportsTextureNpot()) {
    // Npot textures are supported in ES 2.0 if you use GL_CLAMP_TO_EDGE and no
    // mipmaps. See Section 3.8.2 of ES2.0 spec:
    // https://www.khronos.org/registry/gles/specs/2.0/es_full_spec_2.0.25.pdf
    if (params.generate_mipmaps || !params.is_cubemap) {
      const int area = data.width * data.height;
      if (area & (area - 1)) {
        LOG(ERROR) << "Texture not power of two in size: " << size.x << "x"
                   << size.y;
      }
    }
  }

  switch (texture_format) {
    case ImageData::kRgb888: {
      data.bytes = buffer;
      data.num_bytes_per_face = 3 * data.width * data.height;
      data.format = GetTextureFormat(texture_format);
      data.type = GetPixelFormat(texture_format);
      break;
    }
    case ImageData::kRgba8888: {
      data.bytes = buffer;
      data.num_bytes_per_face = 4 * data.width * data.height;
      data.format = GetTextureFormat(texture_format);
      data.type = GetPixelFormat(texture_format);
      break;
    }
    case ImageData::kLuminanceAlpha: {
      data.bytes = buffer;
      data.num_bytes_per_face = 2 * data.width * data.height;
      data.format = GetTextureFormat(texture_format);
      data.type = GetPixelFormat(texture_format);
      data.luminance = true;
      break;
    }
    case ImageData::kRgb565:
    case ImageData::kRgba5551:
    case ImageData::kRg88: {
      data.bytes = buffer;
      data.num_bytes_per_face = 2 * data.width * data.height;
      data.format = GetTextureFormat(texture_format);
      data.type = GetPixelFormat(texture_format);
      break;
    }
    case ImageData::kLuminance: {
      data.bytes = buffer;
      data.num_bytes_per_face = 1 * data.width * data.height;
      data.format = GetTextureFormat(texture_format);
      data.type = GetPixelFormat(texture_format);
      data.luminance = true;
      break;
    }
    case ImageData::kAstc: {
      const AstcHeader* header = GetAstcHeader(buffer, len);
      data.bytes = buffer + sizeof(AstcHeader);
      data.num_bytes_per_face =
          static_cast<int>(len - sizeof(AstcHeader)) / data.num_faces;
      data.format = GetAstcFormat(header);
      break;
    }
    case ImageData::kPkm: {
      data.bytes = buffer + sizeof(PkmHeader);
      data.num_bytes_per_face =
          static_cast<int>(len - sizeof(PkmHeader)) / data.num_faces;
      data.format = GL_COMPRESSED_RGB8_ETC2;
      break;
    }
    case ImageData::kKtx: {
      // We'll handle KTX images below.
#ifdef DEBUG
      // This check causes unused variable errors in opt builds, so only call
      // GetKtxHeader() in debug builds.
      const KtxHeader* header = GetKtxHeader(buffer, len);
      DCHECK(data.num_faces == static_cast<int>(header->faces));
#endif
      break;
    }
    default: {
      LOG(DFATAL) << "Unknown texture format.";
      return TextureHnd();
    }
  }

  GLuint texture_id;
  GL_CALL(glGenTextures(1, &texture_id));
  GL_CALL(glActiveTexture(GL_TEXTURE0));

  const GLenum target =
      (data.num_faces == 6) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
  GL_CALL(
      glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(unpack_alignment)));
  GL_CALL(
      glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(pixels_per_row)));
  GL_CALL(glBindTexture(target, texture_id));

  if (data.num_faces == 6) {
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
  } else {
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S,
                            GetGlTextureWrap(params.wrap_s)));
    GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T,
                            GetGlTextureWrap(params.wrap_t)));
  }
  GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER,
                          GetGlTextureFiltering(params.mag_filter)));
  // TODO: Use TextureParams for min filtering as well. It is not
  // yet used because the TextureParams default value (NearestMipmapLinear) is
  // not obviously reconcilable with either of the default values here (Linear
  // and LinearMipmapLinear.)
  if (data.generate_mipmaps) {
    GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR));
  } else {
    GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  }
  if (texture_format == ImageData::kKtx) {
    BuildKtxTexture(buffer, len, data.generate_mipmaps);
  } else {
    BuildGlTexture(data);
  }
  GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
  GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));

  return texture_id;
}

}  // namespace

void TextureFactoryImpl::SetKtxFormatSupported(bool enable) {
  g_app_set_ktx_supported = true;
  g_is_ktx_supported = enable;
}

TextureFactoryImpl::TextureFactoryImpl(Registry* registry)
    : registry_(registry),
      textures_(ResourceManager<Texture>::kWeakCachingOnly) {
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
    std::string resolved(filename);
    if (!IsExtensionSupported(GetExtensionFromFilename(resolved))) {
      resolved = RemoveExtensionFromFilename(resolved) + ".webp";
    }

    auto texture = std::make_shared<Texture>();
    texture->SetName(resolved);

    // Try software decoding of the astc if needed.
    uint32_t flags = 0;
    if (!GlSupportsAstc()) {
      flags |= kDecodeImage_DecodeAstc;
    }

    auto* asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync<TextureAsset>(
        resolved, params,
        [this, texture](TextureAsset* asset) {
          InitTextureImpl(texture, &asset->image_data_, asset->params_);
          if (asset->animated_image_ != nullptr) {
            auto* animated_texture_processor =
                registry_->Get<AnimatedTextureProcessor>();
            if (animated_texture_processor != nullptr) {
              animated_texture_processor->Animate(
                  texture, std::move(asset->animated_image_));
            } else {
              LOG(DFATAL) << "AnimatedTextureProcessor not found.";
            }
          }
        },
        [texture, params](ErrorCode error) {
          const ImageData image = CreateInvalidImage();
          const TextureHnd hnd = CreateTextureHnd(
              image.GetBytes(), image.GetDataSize(), image.GetFormat(),
              image.GetSize(), params, image.GetRowAlignment(),
              image.GetStrideInPixels());
          const GLenum target =
              params.is_cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
          texture->Init(hnd, target, image.GetSize(), 0);
        },
        flags);
    return texture;
  });
}

void TextureFactoryImpl::LoadAtlas(const std::string& filename,
                                   const TextureParams& params) {
  const HashValue key = Hash(filename.c_str());
  atlases_.Create(key, [&]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    auto asset = asset_loader->LoadNow<SimpleAsset>(filename);
    if (asset->GetSize() == 0) {
      return std::shared_ptr<TextureAtlas>();
    }

    auto atlasdef = atlasdef::GetTextureAtlas(asset->GetData());
    auto* texture_factory =
        static_cast<TextureFactoryImpl*>(registry_->Get<TextureFactory>());

    TexturePtr texture = texture_factory->LoadTexture(
        atlasdef->texture_filename()->c_str(), params);

    std::vector<std::string> subtextures;
    subtextures.reserve(atlasdef->entries()->Length());
    for (const auto* entry : *atlasdef->entries()) {
      const char* name = entry->name()->c_str();
      subtextures.emplace_back(name);

      const mathfu::vec2 size(entry->size()->x(), entry->size()->y());
      const mathfu::vec2 location(entry->location()->x(),
                                  entry->location()->y());
      const mathfu::vec4 uv_bounds(location.x, location.y, size.x, size.y);
      TexturePtr sub_texture =
          texture_factory->CreateSubtexture(Hash(name), texture, uv_bounds);
      texture_factory->CacheTexture(Hash(name), sub_texture);
    }

    auto atlas = std::make_shared<TextureAtlas>();
    atlas->Init(texture, std::move(subtextures));
    return atlas;
  });
}

bool TextureFactoryImpl::UpdateTexture(TexturePtr texture, ImageData image) {
  if (image.GetSize() != texture->GetDimensions()) {
    return false;
  }
  if (!texture->GetResourceId().Valid()) {
    LOG(ERROR) << "Can't update invalid textures.";
    return false;
  }
  if (texture->target_ != GL_TEXTURE_2D) {
    LOG(ERROR) << "Only internal 2D textures can be updated";
    return false;
  }
  if (texture->IsSubtexture()) {
    LOG(ERROR) << "Updating subtextures is not supported.";
    return false;
  }
  const int level = 0;
  const int xoffset = 0;
  const int yoffset = 0;
  const GLenum format = GetTextureFormat(image.GetFormat());
  const GLenum type = GetPixelFormat(image.GetFormat());

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(texture->target_, *texture->GetResourceId()));
  GL_CALL(glTexSubImage2D(texture->target_, level, xoffset, yoffset,
                          image.GetSize().x, image.GetSize().y, format, type,
                          image.GetBytes()));
  if (texture->flags_ & Texture::kHasMipMaps) {
    GL_CALL(glGenerateMipmap(texture->target_));
  }
  return true;
}

TexturePtr TextureFactoryImpl::CreateTexture(ImageData image,
                                             const TextureParams& params) {
  return CreateTextureDeprecated(&image, params);
}

TexturePtr TextureFactoryImpl::CreateTexture(HashValue name, ImageData image,
                                             const TextureParams& params) {
  return textures_.Create(
      name, [&]() { return CreateTextureDeprecated(&image, params); });
}

TexturePtr TextureFactoryImpl::CreateTexture(const mathfu::vec2i& size,
                                             const TextureParams& params) {
  if (params.format == ImageData::kInvalid) {
    LOG(DFATAL) << "Must specify format for empty texture.";
    return nullptr;
  }

  DataContainer data(nullptr, 0, DataContainer::kRead);
  ImageData empty(params.format, size, std::move(data));
  return CreateTexture(std::move(empty), params);
}

TexturePtr TextureFactoryImpl::CreateTexture(uint32_t texture_target,
                                             uint32_t texture_id) {
  return CreateTexture(texture_target, texture_id, mathfu::kZeros2i);
}

TexturePtr TextureFactoryImpl::CreateTexture(uint32_t texture_target,
                                             uint32_t texture_id,
                                             const mathfu::vec2i& size) {
  auto texture = std::make_shared<Texture>();
  texture->Init(TextureHnd(texture_id), texture_target, size,
                Texture::kIsExternal);
  return texture;
}

TexturePtr TextureFactoryImpl::CreateExternalTexture() {
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
  return CreateTexture(GL_TEXTURE_EXTERNAL_OES, texture_id, mathfu::kZeros2i);
#else
  LOG(DFATAL) << "External textures are not available.";
  return nullptr;
#endif  // GL_TEXTURE_EXTERNAL_OES
}

TexturePtr TextureFactoryImpl::CreateExternalTexture(
    const mathfu::vec2i& size) {
  return CreateExternalTexture();
}

TexturePtr TextureFactoryImpl::CreateSubtexture(const TexturePtr& texture,
                                                const mathfu::vec4& uv_bounds) {
  auto subtexture = std::make_shared<Texture>();
  subtexture->Init(texture, uv_bounds);
  return subtexture;
}

TexturePtr TextureFactoryImpl::CreateSubtexture(HashValue name,
                                                const TexturePtr& texture,
                                                const mathfu::vec4& uv_bounds) {
  return textures_.Create(
      name, [&]() { return CreateSubtexture(texture, uv_bounds); });
}

void TextureFactoryImpl::CacheTexture(HashValue name,
                                      const TexturePtr& texture) {
  textures_.Register(name, texture);
}

void TextureFactoryImpl::ReleaseTexture(HashValue name) {
  textures_.Erase(name);

  auto atlas = atlases_.Find(name);
  if (atlas) {
    for (const auto& subname : atlas->Subtextures()) {
      const HashValue key = Hash(subname.c_str());
      textures_.Erase(key);
    }
    atlases_.Release(name);
  }
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
  InitTextureImpl(texture, image, params);
  return texture;
}

void TextureFactoryImpl::InitTextureImpl(const TexturePtr& texture,
                                         const ImageData* image,
                                         const TextureParams& params) {
  const TextureHnd hnd =
      CreateTextureHnd(image->GetBytes(), image->GetDataSize(),
                       image->GetFormat(), image->GetSize(), params,
                       image->GetRowAlignment(), image->GetStrideInPixels());
  const GLenum target = params.is_cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
  const auto texture_flags = params.generate_mipmaps ? Texture::kHasMipMaps : 0;
  texture->Init(hnd, target, image->GetSize(), texture_flags);
}

}  // namespace lull
