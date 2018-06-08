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

#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/util/filename.h"

namespace lull {
namespace {

// TODO(gavindodd): as KTX is a container class there is no way to determine
// if the extension is really supported
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
  // KTX can contain any image format.
  return g_is_ktx_supported;
}

bool IsExtensionSupported(const std::string& ext) {
  if (ext == ".astc") {
    return GlSupportsAstc();
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

static bool IsETC(GLenum format) {
  switch (format) {
#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_4_3)
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      return true;
#endif  // GL_ES_VERSION_3_0 || GL_VERSION_4_3
    default:
      return false;
  }
}

static mathfu::vec2i GetKtxBlockSize(GLenum format) {
  switch (format) {
#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_4_3)
    // ETC1 and ETC2 use 4x4 blocks.
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      return mathfu::vec2i(4, 4);
#endif  // GL_ES_VERSION_3_0 || GL_VERSION_4_3
#if defined(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR)
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
      return mathfu::vec2i(4, 4);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
      return mathfu::vec2i(5, 4);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
      return mathfu::vec2i(5, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
      return mathfu::vec2i(6, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
      return mathfu::vec2i(6, 6);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
      return mathfu::vec2i(8, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
      return mathfu::vec2i(8, 6);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
      return mathfu::vec2i(8, 8);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
      return mathfu::vec2i(10, 5);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
      return mathfu::vec2i(10, 6);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
      return mathfu::vec2i(10, 8);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
      return mathfu::vec2i(10, 10);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
      return mathfu::vec2i(12, 10);
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
      return mathfu::vec2i(12, 12);
#endif  // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
    case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
      return mathfu::vec2i(4, 4);
    case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
      return mathfu::vec2i(5, 4);
    case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
      return mathfu::vec2i(5, 5);
    case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
      return mathfu::vec2i(6, 5);
    case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
      return mathfu::vec2i(6, 6);
    case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
      return mathfu::vec2i(8, 5);
    case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
      return mathfu::vec2i(8, 6);
    case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
      return mathfu::vec2i(8, 8);
    case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
      return mathfu::vec2i(10, 5);
    case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
      return mathfu::vec2i(10, 6);
    case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
      return mathfu::vec2i(10, 8);
    case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
      return mathfu::vec2i(10, 10);
    case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
      return mathfu::vec2i(12, 10);
    case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
      return mathfu::vec2i(12, 12);
    default:
      // Uncompressed textures effectively have 1x1 blocks.
      return mathfu::vec2i(1, 1);
  }
}

void WriteTexImage(const uint8_t* data, int num_bytes_per_face, int num_faces,
                   int width, int height, int mip_level, GLenum format,
                   GLenum type, bool compressed) {
  if (!ValidateNumFaces(num_faces)) {
    return;
  }

  // TODO(b/80479928): It's possible for this to fail due to unsupported image
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
      GL_CALL(glCompressedTexImage2D(target + i, mip_level, format, width,
                                     height, border, num_bytes_per_face, data));
    } else {
      GL_CALL(glTexImage2D(target + i, mip_level, format, width, height, border,
                           format, type, data));
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
                data.height, main_mip_level, data.format, data.type,
                data.compressed);

#if !defined(FPLBASE_GLES) && defined(GL_TEXTURE_SWIZZLE_A)
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
    const int num_levels = static_cast<int>(
        ceil(log(static_cast<float>(min_dimension)) / log(2.0f)));

    int width = data.width;
    int height = data.height;
    for (int i = 1; i < num_levels; ++i) {
      width /= 2;
      height /= 2;
      WriteTexImage(nullptr, data.num_bytes_per_face, data.num_faces, width,
                    height, i, data.format, data.type, data.compressed);
    }
    const GLenum target =
        (data.num_faces == 6) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GL_CALL(glGenerateMipmap(target));
  }
}

void BuildKtxTexture(const uint8_t* buffer, size_t len, bool generate_mipmaps) {
  const KtxHeader* header = GetKtxHeader(buffer, len);

  const bool isETC = IsETC(header->internal_format);
  const mathfu::vec2i block_size = GetKtxBlockSize(header->internal_format);
  const bool compressed = block_size.x > 1 || block_size.y > 1;

  int mip_width = header->width;
  int mip_height = header->height;
  const uint8_t* data = buffer + sizeof(KtxHeader) + header->keyvalue_data;
  for (uint32_t i = 0; i < header->mip_levels; ++i) {
    // Guard against extra mip levels in the ktx when using ETC compression.
    if (isETC && (mip_width < block_size.x || mip_height < block_size.y)) {
      LOG(ERROR) << "KTX file has too many mips.";
      // Some GL drivers need to be explicitly told that we don't have a
      // full mip chain (down to 1x1).
      DCHECK(i > 0);
      const GLenum target =
          (header->faces == 6) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
      GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, i - 1));
      break;
    }

    // For cube maps imageSize is the number of bytes in each face of the
    // texture for the current LOD level, not including bytes in cubePadding
    // or mipPadding.
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.16
    const int32_t data_size = *(reinterpret_cast<const int32_t*>(data));
    data += sizeof(data_size);
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.17
    // We do not need to worry about cubePadding as we only support etc and astc
    // which are both aligned to 8 or 16 bytes (block size)
    DCHECK_EQ((data_size % 4), 0);

    // Keep loading mip data even if one of our calculated dimensions goes
    // to 0, but maintain a min size of 1.  This is needed to get non-square
    // mip chains to work using ETC2 (eg a 256x512 needs 10 mips defined).
    const int width = std::max(mip_width, 1);
    const int height = std::max(mip_height, 1);

    WriteTexImage(data, data_size, header->faces, width, height, i,
                  header->internal_format, header->type, compressed);

    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.18
    // We do not need to worry about mipPadding as we only support etc and astc
    // which are both aligned to 8 or 16 bytes (block size)
    uint32_t mip_size = data_size * header->faces;
    DCHECK_EQ((mip_size % 4), 0);
    data += mip_size;
    mip_width /= 2;
    mip_height /= 2;
    if (generate_mipmaps == false) {
      break;
    }
  }
}

TextureHnd CreateTextureHnd(const uint8_t* buffer, const size_t len,
                            ImageData::Format texture_format,
                            const mathfu::vec2i& size,
                            const TextureParams& params) {
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
    case ImageData::kRgb565: {
      data.bytes = buffer;
      data.num_bytes_per_face = 2 * data.width * data.height;
      data.format = GetTextureFormat(texture_format);
      data.type = GetPixelFormat(texture_format);
      break;
    }
    case ImageData::kRgba5551: {
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
    case ImageData::kLuminanceAlpha: {
      data.bytes = buffer;
      data.num_bytes_per_face = 2 * data.width * data.height;
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
      const KtxHeader* header = GetKtxHeader(buffer, len);
      DCHECK(data.num_faces == static_cast<int>(header->faces));
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
  return texture_id;
}

class TextureAsset : public Asset {
 public:
  TextureAsset(const TextureParams& params,
               const std::function<void(TextureAsset*)>& finalizer)
      : params_(params), finalizer_(finalizer) {}

  void OnLoad(const std::string& filename, std::string* data) override {
    if (data->empty()) {
      return;
    }
    if (filename.find("cubemap") != std::string::npos) {
      params_.is_cubemap = true;
    }
    if (filename.find("nopremult") != std::string::npos) {
      params_.premultiply_alpha = false;
    }

    const auto* bytes = reinterpret_cast<const uint8_t*>(data->data());
    const size_t num_bytes = data->size();
    const auto flags = params_.premultiply_alpha ? kDecodeImage_PremultiplyAlpha
                                                 : kDecodeImage_None;
    image_data_ = DecodeImage(bytes, num_bytes, flags);
    if (image_data_.IsEmpty()) {
      LOG(ERROR) << "Unsupported texture file type.";
    }
  }

  void OnFinalize(const std::string&, std::string*) override {
    if (!image_data_.IsEmpty()) {
      finalizer_(this);
    }
  }

  ImageData image_data_;
  TextureParams params_;
  std::function<void(TextureAsset*)> finalizer_;
};

}  // namespace

void TextureFactoryImpl::SetKtxFormatSupported(bool enable) {
  g_is_ktx_supported = enable;
}

TextureFactoryImpl::TextureFactoryImpl(Registry* registry)
    : registry_(registry),
      textures_(ResourceManager<Texture>::kWeakCachingOnly) {
  // Create placeholder white texture.
  {
    constexpr int kTextureSize = 2;
    const Color4ub data[kTextureSize * kTextureSize];
    const mathfu::vec2i size(kTextureSize, kTextureSize);
    ImageData image(ImageData::kRgba8888, size,
                    DataContainer::WrapDataAsReadOnly(data, sizeof(data)));
    white_texture_ = CreateTexture(std::move(image), TextureParams());
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
    const mathfu::vec2i size(kTextureSize, kTextureSize);
    ImageData image(ImageData::kRgba8888, size,
                    DataContainer::WrapDataAsReadOnly(data, sizeof(data)));
    invalid_texture_ = CreateTexture(std::move(image), TextureParams());
  }
#else
  invalid_texture_ = white_texture_;
#endif
}

TexturePtr TextureFactoryImpl::LoadTexture(string_view filename,
                                           const TextureParams& params) {
  const HashValue name = Hash(filename);
  return textures_.Create(name, [this, filename, &params]() {
    std::string resolved = filename.to_string();
    if (!IsExtensionSupported(GetExtensionFromFilename(resolved))) {
      resolved = RemoveExtensionFromFilename(resolved) + ".webp";
    }

    auto texture = std::make_shared<Texture>();
    texture->SetName(resolved);

    auto* asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync<TextureAsset>(
        resolved, params, [this, texture](TextureAsset* asset) {
          InitTextureImpl(texture, &asset->image_data_, asset->params_);
        });
    return texture;
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
  return CreateTexture(&image, params);
}

TexturePtr TextureFactoryImpl::CreateTexture(HashValue name, ImageData image,
                                             const TextureParams& params) {
  return textures_.Create(name,
                          [&]() { return CreateTexture(&image, params); });
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
}

TexturePtr TextureFactoryImpl::GetTexture(HashValue name) const {
  return textures_.Find(name);
}

const TexturePtr& TextureFactoryImpl::GetWhiteTexture() const {
  return white_texture_;
}

const TexturePtr& TextureFactoryImpl::GetInvalidTexture() const {
  return invalid_texture_;
}

TexturePtr TextureFactoryImpl::CreateTexture(const ImageData* image,
                                             const TextureParams& params) {
  auto texture = std::make_shared<Texture>();
  InitTextureImpl(texture, image, params);
  return texture;
}

TexturePtr TextureFactoryImpl::CreateTexture(HashValue name,
                                             const ImageData* image,
                                             const TextureParams& params) {
  return textures_.Create(name, [&]() { return CreateTexture(image, params); });
}

void TextureFactoryImpl::InitTextureImpl(
    const TexturePtr& texture, const ImageData* image,
    const TextureParams& params) {
  const TextureHnd hnd =
      CreateTextureHnd(image->GetBytes(), image->GetDataSize(),
                       image->GetFormat(), image->GetSize(), params);
  const GLenum target = params.is_cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
  const auto texture_flags = params.generate_mipmaps ? Texture::kHasMipMaps : 0;
  texture->Init(hnd, target, image->GetSize(), texture_flags);
}

}  // namespace lull
