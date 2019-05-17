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

#ifndef LULLABY_UTIL_IMAGE_DECODE_H_
#define LULLABY_UTIL_IMAGE_DECODE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "lullaby/modules/render/image_data.h"
#include "lullaby/util/clock.h"

namespace lull {

/// Header structure for Webp files.
struct WebpHeader {
  uint8_t magic[4];
  uint32_t size;
  uint8_t webp[4];
};

/// Header structure for PKM files.
struct PkmHeader {
  char magic[4];          // "PKM "
  char version[2];        // "10"
  uint8_t data_type[2];   // 0 (ETC1_RGB_NO_MIPMAPS)
  uint8_t ext_width[2];   // rounded up to 4 size, big endian.
  uint8_t ext_height[2];  // rounded up to 4 size, big endian.
  uint8_t width[2];       // original, big endian.
  uint8_t height[2];      // original, big endian.
  // Data follows header, size = (ext_width / 4) * (ext_height / 4) * 8
};

/// Header structure for ASTC files.
struct AstcHeader {
  uint8_t magic[4];
  uint8_t blockdim_x;
  uint8_t blockdim_y;
  uint8_t blockdim_z;
  uint8_t xsize[3];
  uint8_t ysize[3];
  uint8_t zsize[3];
};

// Returns the PkmHeader if the buffer is a PKM image file, otherwise returns
// nullptr.
const PkmHeader* GetPkmHeader(const uint8_t* data, size_t len);

// Returns an int from a 3 byte array
int GetAstcSize(const uint8_t (&size)[3]);

// Returns the AstcHeader if the buffer is a ASTC image file, otherwise returns
// nullptr.
const AstcHeader* GetAstcHeader(const uint8_t* data, size_t len);

using AstcDecoderFn = ImageData(*)(const mathfu::vec2i& size,
                                   const mathfu::vec2i& block, int faces,
                                   const uint8_t* data, size_t len);

// Sets an ASTC decoder to use if ASTC isn't supported by GL.
void SetAstcDecoder(AstcDecoderFn decoder);
void SetGpuDecodingEnabled(bool enabled);

// Returns true if ASTC images can be decoded on the CPU.
bool CpuAstcDecodingAvailable();
bool GpuAstcDecodingAvailable();

// Returns the WebpHeader if the buffer is a WEBP image file, otherwise returns
// nullptr.
const WebpHeader* GetWebpHeader(const uint8_t* data, size_t len);

// Flags used for decoding.
enum DecodeImageFlags {
  kDecodeImage_None = 0,
  kDecodeImage_PremultiplyAlpha = 0x01 << 1,
  kDecodeImage_DecodeAstc = 0x01 << 2,
};

// Decodes image data that is stored in either jpg, png, webp, tga, ktx, pkm,
// or astc format.
ImageData DecodeImage(const uint8_t* data, size_t len, DecodeImageFlags flags);

// Returns whether the image contains animation. Only webp is supported
// currently.
bool IsAnimated(const std::uint8_t* data, std::size_t len);

// Interface for working with animated images.
class AnimatedImage {
 public:
  virtual ~AnimatedImage() = default;
  virtual ImageData DecodeNextFrame() = 0;
  virtual ImageData GetCurrentFrame() = 0;

  // Size in bytes of the decoded image frame.
  virtual std::size_t GetFrameSize() = 0;

  // Length of time to display the current frame.
  virtual Clock::duration GetCurrentFrameDuration() = 0;
};

using AnimatedImagePtr = std::unique_ptr<AnimatedImage>;

// Returned AnimatedImagePtr takes ownership of the underlying image |data|.
AnimatedImagePtr LoadAnimatedImage(std::string* data);

}  // namespace lull

#endif  // LULLABY_UTIL_IMAGE_DECODE_H_
