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

#include "lullaby/modules/render/image_decode.h"
#include "lullaby/modules/render/image_decode_ktx.h"

#include <cstdlib>
#include <memory>

#if !LULLABY_DISABLE_STB_LOADERS
#include "stb/stb_image.h"
#endif

#if !LULLABY_DISABLE_WEBP_LOADER
#ifdef LULLABY_GYP
#include "webp/decode.h"
#include "webp/demux.h"
#include "webp/mux_types.h"
#else
#include "webp/decode.h"
#include "webp/demux.h"
#include "webp/mux_types.h"
#endif
#endif

#include "GL/gl.h"
#include "GL/glext.h"
#include "lullaby/modules/render/image_util.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/data_container.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"
#include "mathfu/glsl_mappings.h"
#if defined(LULLABY_ASTC_CPU_DECODE) && !defined(LULLABY_DISABLE_ASTC_LOADERS)
#include "lullaby/modules/render/image_decode_astc.h"
#endif  // defined(LULLABY_ASTC_CPU_DECODE) &&
        // !defined(LULLABY_DISABLE_ASTC_LOADERS)

namespace lull {
namespace {

constexpr char kRiffMagicId[] = "RIFF";
constexpr char kWebpMagicId[] = "WEBP";
constexpr char kPkmMagicId[] = "PKM ";
constexpr char kPkmVersion[] = "10";
constexpr uint8_t kAstcMagicId[] = {0x13, 0xab, 0xa1, 0x5c};

ImageData BuildImageData(
    const uint8_t* bytes, size_t num_bytes, ImageData::Format format,
    const mathfu::vec2i& size,
    std::function<void(const uint8_t*)> deleter = nullptr) {
  DataContainer data;
  if (deleter) {
    DataContainer::DataPtr ptr(const_cast<uint8_t*>(bytes), std::move(deleter));
    data = DataContainer(std::move(ptr), num_bytes, num_bytes,
                         DataContainer::kRead);
  } else {
    data = DataContainer::WrapDataAsReadOnly(bytes, num_bytes);
  }
  return ImageData(format, size, std::move(data));
}

int GetPkmSize(const uint8_t (&size)[2]) {
  // Pkm dimension is stored in big endian format.
  return (size[0] << 8) | (size[1]);
}

mathfu::vec2i GetPkmImageDimensions(const PkmHeader* header) {
  DCHECK(header != nullptr);
  const int xsize = GetPkmSize(header->width);
  const int ysize = GetPkmSize(header->height);
  return mathfu::vec2i(xsize, ysize);
}

mathfu::vec2i GetAstcImageDimensions(const AstcHeader* header) {
  DCHECK(header != nullptr);
  const int xsize = GetAstcSize(header->xsize);
  const int ysize = GetAstcSize(header->ysize);
  return mathfu::vec2i(xsize, ysize);
}

ImageData DecodeStbi(const uint8_t* src, size_t len, DecodeImageFlags flags) {
  // TODO: Include the source image name in log messages.
#if LULLABY_DISABLE_STB_LOADERS
  // libstb isn't known to be safe, so we've disabled it to eliminate an
  // attack vector.
  LOG(DFATAL) << "STB decoding disabled.";
  return ImageData();
#else
  int width = 0;
  int height = 0;
  int32_t channels = 0;
  uint8_t* bytes = stbi_load_from_memory(static_cast<const stbi_uc*>(src),
                                         static_cast<int>(len), &width, &height,
                                         &channels, 0);
  if (bytes == nullptr) {
    LOG(DFATAL) << "Unable to decode image.";
    return ImageData();
  }

  const mathfu::vec2i size(width, height);
  ImageData::Format format = ImageData::kInvalid;
  if (channels == 4) {
    if (flags & kDecodeImage_PremultiplyAlpha) {
      MultiplyRgbByAlpha(bytes, size);
    }
    format = ImageData::kRgba8888;
  } else if (channels == 3) {
    format = ImageData::kRgb888;
  } else if (channels == 2) {
    format = ImageData::kLuminanceAlpha;
  } else if (channels == 1) {
    format = ImageData::kLuminance;
  } else {
    LOG(DFATAL) << "Unsupported number of channels: " << channels;
  }

  const size_t num_bytes = width * height * channels;
  return BuildImageData(bytes, num_bytes, format, size, [](const uint8_t* ptr) {
    free(const_cast<uint8_t*>(ptr));
  });
#endif
}

ImageData DecodeWebp(const uint8_t* data, size_t len, DecodeImageFlags flags) {
  // TODO: Include the source image name in log messages.
#if LULLABY_DISABLE_WEBP_LOADER
  // libwebp isn't known to be safe, so we've disabled it to eliminate an
  // attack vector.
  LOG(DFATAL) << "WebP decoding disabled.";
  return ImageData();
#else
  WebPDecoderConfig config;
  memset(&config, 0, sizeof(WebPDecoderConfig));
  auto status = WebPGetFeatures(data, len, &config.input);
  if (status != VP8_STATUS_OK) {
    LOG(DFATAL) << "Source image data not an WebP file.";
    return ImageData();
  }
  if (config.input.has_alpha) {
    if (flags & kDecodeImage_PremultiplyAlpha) {
      config.output.colorspace = MODE_rgbA;
    } else {
      config.output.colorspace = MODE_RGBA;
    }
  }
  status = WebPDecode(data, len, &config);
  if (status != VP8_STATUS_OK) {
    LOG(DFATAL) << "Unable to decode WebP data.";
    return ImageData();
  }

  int bytes_per_pixel = 0;
  switch (config.output.colorspace) {
    case MODE_RGB:
    case MODE_BGR:
      bytes_per_pixel = 3;
      break;
    case MODE_RGBA:
    case MODE_BGRA:
    case MODE_ARGB:
    case MODE_rgbA:
    case MODE_bgrA:
    case MODE_Argb:
      bytes_per_pixel = 4;
      break;
    case MODE_RGBA_4444:
    case MODE_rgbA_4444:
    case MODE_RGB_565:
      bytes_per_pixel = 2;
      break;
    default:
      LOG(DFATAL) << "Unknown webp colorspace: " << config.output.colorspace;
      return ImageData();
  }

  const mathfu::vec2i size(config.output.width, config.output.height);
  const uint8_t* bytes = config.output.private_memory;
  const size_t num_bytes = size.x * size.y * bytes_per_pixel;
  const ImageData::Format format =
      config.input.has_alpha ? ImageData::kRgba8888 : ImageData::kRgb888;
  return BuildImageData(bytes, num_bytes, format, size, [](const uint8_t* ptr) {
    free(const_cast<uint8_t*>(ptr));
  });
#endif
}

mathfu::vec2i GetAstcBlockSizeFromGlInternalFormat(int gl_internal_format) {
  switch (gl_internal_format) {
    case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
      return mathfu::vec2i(4, 4);
    case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
      return mathfu::vec2i(5, 4);
    case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
      return mathfu::vec2i(5, 5);
    case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
      return mathfu::vec2i(6, 5);
    case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
      return mathfu::vec2i(6, 6);
    case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
      return mathfu::vec2i(8, 5);
    case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
      return mathfu::vec2i(8, 6);
    case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
      return mathfu::vec2i(8, 8);
    case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
      return mathfu::vec2i(10, 5);
    case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
      return mathfu::vec2i(10, 6);
    case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
      return mathfu::vec2i(10, 8);
    case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
      return mathfu::vec2i(10, 10);
    case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
      return mathfu::vec2i(12, 10);
    case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
    case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
      return mathfu::vec2i(12, 12);
    default:
      LOG(ERROR) << "Unsupported ASTC internal format " << gl_internal_format;
      break;
  }
  return mathfu::vec2i(0, 0);
}

bool g_gpu_astc_supported = false;

#if defined(LULLABY_ASTC_CPU_DECODE) && !defined(LULLABY_DISABLE_ASTC_LOADERS)
AstcDecoderFn g_astc_decoder = DecodeAstc;
#else
AstcDecoderFn g_astc_decoder = nullptr;
#endif  // defined(LULLABY_ASTC_CPU_DECODE) &&
        // !defined(LULLABY_DISABLE_ASTC_LOADERS)

#if !LULLABY_DISABLE_WEBP_LOADER
class WebPAnimatedImage : public lull::AnimatedImage {
 public:
  WebPAnimatedImage(std::string* data);
  ~WebPAnimatedImage() override;
  ImageData DecodeNextFrame() override;
  ImageData GetCurrentFrame() override;
  size_t GetFrameSize() override;
  Clock::duration GetCurrentFrameDuration() override;

 private:
  std::string raw_webp_bytes_;
  WebPData webp_data_;
  WebPAnimDecoderOptions decoder_options_;
  WebPAnimDecoder* decoder_ = nullptr;
  mathfu::vec2i canvas_size_ = {0, 0};
  uint8_t* decoded_frame_ = nullptr;
  int timestamp_ms_ = 0;  // time point in webp's animation timeline
  int duration_ms_ = 0;   // time to hold this frame.
};

WebPAnimatedImage::WebPAnimatedImage(std::string* data) {
  raw_webp_bytes_ = std::move(*data);

  WebPDataInit(&webp_data_);
  webp_data_.bytes = reinterpret_cast<const uint8_t*>(raw_webp_bytes_.data());
  webp_data_.size = raw_webp_bytes_.size();

  WebPAnimDecoderOptionsInit(&decoder_options_);
  decoder_options_.color_mode = MODE_rgbA;
  decoder_options_.use_threads = 0;

  decoder_ = WebPAnimDecoderNew(&webp_data_, &decoder_options_);

  WebPAnimInfo anim_info;
  WebPAnimDecoderGetInfo(decoder_, &anim_info);
  canvas_size_ =
      mathfu::vec2i(anim_info.canvas_width, anim_info.canvas_height);
}

WebPAnimatedImage::~WebPAnimatedImage() {
  // Delete the decoder.
  WebPAnimDecoderDelete(decoder_);
}

ImageData WebPAnimatedImage::DecodeNextFrame() {
  // Decode & return
  if (!WebPAnimDecoderHasMoreFrames(decoder_)) {
    WebPAnimDecoderReset(decoder_);
  }
  int prev_timestamp_ms_ = timestamp_ms_;
  WebPAnimDecoderGetNext(decoder_, &decoded_frame_, &timestamp_ms_);
  duration_ms_ = timestamp_ms_ - prev_timestamp_ms_;

  // Account for looping animation back to beginning.
  if (duration_ms_ < 0) {
    duration_ms_ = timestamp_ms_;
  }

  // Note: intentionally not providing a deleter since the decoded frame is
  // owned by the decoder.
  return BuildImageData(decoded_frame_, GetFrameSize(),
                        ImageData::kRgba8888, canvas_size_);
}

ImageData WebPAnimatedImage::GetCurrentFrame() {
  // Note: intentionally not providing a deleter since the decoded frame is
  // owned by the decoder.
  return BuildImageData(decoded_frame_, GetFrameSize(),
                        ImageData::kRgba8888, canvas_size_);
}

size_t WebPAnimatedImage::GetFrameSize() {
  size_t bytes_per_pixel = 4;  // RGBA
  return canvas_size_.x * canvas_size_.y * bytes_per_pixel;
}

Clock::duration WebPAnimatedImage::GetCurrentFrameDuration() {
  return DurationFromMilliseconds(static_cast<float>(duration_ms_));
}

#endif  // !LULLABY_DISABLE_WEBP_LOADER

}  // namespace

const WebpHeader* GetWebpHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const WebpHeader*>(data);
  if (len < sizeof(WebpHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kRiffMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  if (memcmp(header->webp, kWebpMagicId, sizeof(header->webp))) {
    return nullptr;
  }
  return header;
}

const PkmHeader* GetPkmHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const PkmHeader*>(data);
  if (len < sizeof(PkmHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kPkmMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  if (memcmp(header->version, kPkmVersion, sizeof(header->version))) {
    return nullptr;
  }
  return header;
}

int GetAstcSize(const uint8_t (&size)[3]) {
  return (size[0]) | (size[1] << 8) | (size[2] << 16);
}

const AstcHeader* GetAstcHeader(const uint8_t* data, size_t len) {
  const auto* header = reinterpret_cast<const AstcHeader*>(data);
  if (len < sizeof(AstcHeader)) {
    return nullptr;
  }
  if (memcmp(header->magic, kAstcMagicId, sizeof(header->magic))) {
    return nullptr;
  }
  return header;
}

void SetAstcDecoder(AstcDecoderFn decoder) { g_astc_decoder = decoder; }

void SetGpuDecodingEnabled(bool enabled) { g_gpu_astc_supported = enabled; }

bool CpuAstcDecodingAvailable() { return g_astc_decoder != nullptr; }

bool GpuAstcDecodingAvailable() { return g_gpu_astc_supported; }

ImageData DecodeImage(const uint8_t* data, size_t len, DecodeImageFlags flags) {
  if (auto header = GetAstcHeader(data, len)) {
    const mathfu::vec2i size = GetAstcImageDimensions(header);
    const int zsize = GetAstcSize(header->zsize);
    DCHECK(zsize == 1 || !(flags & kDecodeImage_DecodeAstc));
    if (g_astc_decoder != nullptr && zsize == 1 &&
        (flags & kDecodeImage_DecodeAstc)) {
      const mathfu::vec2i block(header->blockdim_x, header->blockdim_y);
      DCHECK(header->blockdim_z == 1);
      // Skip the ASTC header.
      len -= sizeof(*header);
      data += sizeof(*header);
      return g_astc_decoder(size, block, 1, data, len);
    } else {
      return BuildImageData(data, len, ImageData::kAstc, size);
    }
  } else if (auto header = GetPkmHeader(data, len)) {
    const mathfu::vec2i size = GetPkmImageDimensions(header);
    return BuildImageData(data, len, ImageData::kPkm, size);
  } else if (auto header = GetKtxHeader(data, len)) {
    const mathfu::vec2i size(header->width, header->height);
    if (g_astc_decoder != nullptr && (flags & kDecodeImage_DecodeAstc)) {
      const bool is_simple = header->depth == 0 && header->array_elements == 0;
      DCHECK(is_simple) << "3D or array textures not yet supported.";
      const mathfu::vec2i block =
          GetAstcBlockSizeFromGlInternalFormat(header->internal_format);
      if (is_simple && block.x != 0 && block.y != 0) {
        // Skip the KTX header.
        len -= sizeof(*header);
        data += sizeof(*header);
        // Skip any key/value data.
        len -= header->keyvalue_data;
        data += header->keyvalue_data;
        // Skip the 32 bits of size data.
        len -= sizeof(uint32_t);
        data += sizeof(uint32_t);
        return g_astc_decoder(size, block, header->faces, data, len);
      }
    }
    return BuildImageData(data, len, ImageData::kKtx, size);
  } else if (GetWebpHeader(data, len)) {
    return DecodeWebp(data, len, flags);
  } else {
    return DecodeStbi(data, len, flags);
  }
}

bool IsAnimated(const uint8_t* data, size_t len) {
  if (GetWebpHeader(data, len)) {
#if LULLABY_DISABLE_WEBP_LOADER
    return false;
#else
    WebPBitstreamFeatures features;
    std::memset(&features, 0, sizeof(WebPBitstreamFeatures));
    auto status = WebPGetFeatures(data, len, &features);
    if (status != VP8_STATUS_OK) {
      LOG(DFATAL) << "Source image data not an WebP file.";
      return false;
    }
    return features.has_animation;
#endif
  }
  return false;
}

AnimatedImagePtr LoadAnimatedImage(std::string* data) {
  const auto* bytes = reinterpret_cast<const uint8_t*>(data->data());
  const size_t num_bytes = data->size();
  if (GetWebpHeader(bytes, num_bytes)) {
#if LULLABY_DISABLE_WEBP_LOADER
    LOG(DFATAL) << "WebP decoding disabled.";
    return nullptr;
#else
    return std::unique_ptr<AnimatedImage>(new WebPAnimatedImage(data));
#endif  // LULLABY_DISABLE_WEBP_LOADER
  }
  return nullptr;
}

}  // namespace lull
