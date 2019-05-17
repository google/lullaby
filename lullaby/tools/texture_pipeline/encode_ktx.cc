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

#include "lullaby/tools/texture_pipeline/encode_ktx.h"

#include "lullaby/tools/pack_ktx/ktx_astc_image.h"
#include "lullaby/tools/pack_ktx/ktx_direct_image.h"
#include "lullaby/tools/pack_ktx/ktx_image.h"

namespace lull {
namespace tool {

namespace {

uint32_t MipLevels(uint32_t w, uint32_t h) {
  uint32_t dim = std::max(w, h);
  uint32_t mips =
      static_cast<uint32_t>(std::ceil(std::log2(static_cast<double>(dim)))) + 1;
  DCHECK_EQ(dim >> (mips - 1), 1);
  return mips;
}

bool IsUncompressed(ImageData::Format format) {
  switch (format) {
    case ImageData::kAlpha:
    case ImageData::kLuminance:
    case ImageData::kLuminanceAlpha:
    case ImageData::kRgb888:
    case ImageData::kRgba8888:
    case ImageData::kRgb565:
    case ImageData::kRgba4444:
    case ImageData::kRgba5551:
      return true;
    default:
      return false;
  }
}

class KtxUncompressedImage : public KtxImage {
 public:
  KtxUncompressedImage(const ImageData& src);
  ~KtxUncompressedImage() override;
  bool Valid() const override {
    return IsUncompressed(image_data_.GetFormat());
  }

  uint32_t GlType() const override;
  uint32_t GlTypeSize() const override;
  uint32_t GlFormat() const override;
  uint32_t GlInternalFormat() const override;
  uint32_t GlBaseInternalFormat() const override;
  uint32_t PixelWidth() const override;
  uint32_t PixelHeight() const override;
  uint32_t PixelDepth() const override;
  uint32_t NumberOfArrayElements() const override;
  uint32_t NumberOfFaces() const override;
  uint32_t NumberOfMipmapLevels() const override;

 private:
  const ImageData& image_data_;
};

KtxUncompressedImage::KtxUncompressedImage(const ImageData& src)
    : image_data_(src) {
  // TODO: handle mips and cube map
  KTX_image_info image_info;
  image_info.data = const_cast<GLubyte*>(src.GetBytes());
  image_info.size = src.GetDataSize();
  AddImageInfo(image_info);
}

KtxUncompressedImage::~KtxUncompressedImage() {}

// The data type for pixel data
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.3
// Open GL 4.4 Specification Table 8.2
// https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.2
uint32_t KtxUncompressedImage::GlType() const {
  switch (image_data_.GetFormat()) {
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
      LOG(ERROR) << "Unknown GL type for " << image_data_.GetFormat();
      return GL_UNSIGNED_BYTE;
  }
}

// Pixel data type size for endian swapping
// Can be inferred from GlType
// e.g. GL_RGBA8 uses bytes so type size is 1
// e.g. GLRGBA32F uses 32 bit floats so type size is 4
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.4
uint32_t KtxUncompressedImage::GlTypeSize() const {
  // TODO to make more robust?
  switch (image_data_.GetFormat()) {
    case ImageData::kAlpha:
    case ImageData::kLuminance:
      return 1;
    case ImageData::kLuminanceAlpha:
    case ImageData::kRgba4444:
    case ImageData::kRgb565:
    case ImageData::kRgba5551:
      return 2;
    case ImageData::kRgb888:
    case ImageData::kRgba8888:
      return 1;
    default:
      LOG(ERROR) << "Unknown Gl type size for " << image_data_.GetFormat();
      return 1;
  }
}

// The format parameter passed to glTexImage2D or similar
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.5
// Open GL 4.4 Specification Table 8.3
// https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.3
uint32_t KtxUncompressedImage::GlFormat() const {
  switch (image_data_.GetFormat()) {
    case ImageData::kAlpha:
      return GL_RED;  // TODO: Handle GLES
    case ImageData::kLuminance:
      return GL_RED;  // TODO: Handle GLES
    case ImageData::kLuminanceAlpha:
      return GL_RG;  // TODO: Handle GLES
    case ImageData::kRgb888:
    case ImageData::kRgb565:
      return GL_RGB;
    case ImageData::kRgba8888:
    case ImageData::kRgba4444:
    case ImageData::kRgba5551:
      return GL_RGBA;
    default:
      LOG(ERROR) << "Unknown GL format for " << image_data_.GetFormat();
      return 0;
  }
}

// The internalformat parameter passed to glTexImage2D or similar
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.6
// Open GL 4.4 Specification Table 8.12 and 8.13
// https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.12
// https://www.khronos.org/registry/OpenGL/specs/gl/glspec44.core.pdf#nameddest=table-8.13
uint32_t KtxUncompressedImage::GlInternalFormat() const {
  switch (image_data_.GetFormat()) {
    case ImageData::kAlpha:
      return GL_R8;  // TODO: Handle GLES
    case ImageData::kLuminance:
      return GL_R8;  // TODO: Handle GLES
    case ImageData::kLuminanceAlpha:
      return GL_RG8;  // TODO: Handle GLES
    case ImageData::kRgb888:
      return GL_RGB8;
    case ImageData::kRgba8888:
      return GL_RGBA8;
    case ImageData::kRgb565:
      return GL_RGB565;
    case ImageData::kRgba4444:
      return GL_RGBA4;
    case ImageData::kRgba5551:
      return GL_RGB5_A1;
    default:
      LOG(ERROR) << "Unknown internal format for " << image_data_.GetFormat();
      return 0;
  }
}

// For uncompressed textures GlBaseInternalFormat is the same as GlFormat
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.7
uint32_t KtxUncompressedImage::GlBaseInternalFormat() const {
  return GlFormat();
}

uint32_t KtxUncompressedImage::PixelWidth() const {
  return image_data_.GetSize().x;
}

uint32_t KtxUncompressedImage::PixelHeight() const {
  return image_data_.GetSize().y;
}

uint32_t KtxUncompressedImage::PixelDepth() const { return 0; }

uint32_t KtxUncompressedImage::NumberOfArrayElements() const {
  // TODO: support array images
  return 1;
}
uint32_t KtxUncompressedImage::NumberOfFaces() const {
  // TODO: support cube map images
  return 1;
}
uint32_t KtxUncompressedImage::NumberOfMipmapLevels() const {
  // TODO: support mip map images
  return 1;
}

}  // namespace

ByteArray EncodeKtx(const ImageData& src) {
  KtxImage::ImagePtr ktx_image;
  switch (src.GetFormat()) {
    case ImageData::kAstc: {
      KtxImage::ErrorCode result =
          KtxAstcImage::Create(src.GetBytes(), src.GetDataSize(), &ktx_image);
      if (result != KtxImage::kOK) {
        return ByteArray();
      }
    } break;
    case ImageData::kPkm:
      return ByteArray();
    case ImageData::kKtx: {
      KtxImage::ErrorCode result =
          KtxDirectImage::Create(src.GetBytes(), src.GetDataSize(), &ktx_image);
      if (result != KtxImage::kOK) {
        return ByteArray();
      }
    } break;
    default:
      ktx_image.reset(new KtxUncompressedImage(src));
      break;
  }
  return ktx_image->ToByteArray();
}

ByteArray EncodeKtx(const std::vector<ImageData>& srcs,
                    const EncodeInfo& encode_info) {
  KtxImage::ImagePtr ktx_image;
  std::vector<KtxImage::ImagePtr> image_parts;
  for (const auto& src : srcs) {
    switch (src.GetFormat()) {
      case ImageData::kAstc: {
        KtxImage::ErrorCode result =
            KtxAstcImage::Create(src.GetBytes(), src.GetDataSize(), &ktx_image);
        if (result != KtxImage::kOK) {
          LOG(ERROR) << "KTX failed: " << result;
          return ByteArray();
        }
      } break;
      case ImageData::kPkm:
        return ByteArray();
      case ImageData::kKtx: {
        KtxImage::ErrorCode result = KtxDirectImage::Create(
            src.GetBytes(), src.GetDataSize(), &ktx_image);
        if (result != KtxImage::kOK) {
          LOG(ERROR) << "KTX failed: " << result;
          return ByteArray();
        }
      } break;
      default:
        ktx_image.reset(new KtxUncompressedImage(src));
        break;
    }
    image_parts.push_back(std::move(ktx_image));
  }
  if (image_parts.empty()) {
    LOG(ERROR) << "No parts for KTX";
    return ByteArray();
  } else if (encode_info.cube_map) {
    if (encode_info.mip_map) {
      LOG(INFO) << "Encoding mipped cube map KTX";
      std::vector<KtxImage::ImagePtr> image_mips;
      bool generate_mips = image_parts[0]->NumberOfMipmapLevels() == 0;
      uint32_t mips = MipLevels(image_parts[0]->PixelWidth(),
                                image_parts[0]->PixelHeight());
      for (size_t i = 0; i < image_parts.size(); ++i) {
        auto& part = image_parts[i];
        if (part->NumberOfMipmapLevels() == 0) {
          if (!generate_mips) {
            LOG(ERROR)
                << "Mismatched parts for KTX, some generate mips and some not";
            return ByteArray();
          }
          image_mips.push_back(std::move(part));
        } else if (part->NumberOfMipmapLevels() != 1) {
          if (generate_mips) {
            LOG(ERROR)
                << "Mismatched parts for KTX, some generate mips and some not";
            return ByteArray();
          }
          image_mips.push_back(std::move(part));
        } else {
          size_t mip_end = i + mips;
          if (mip_end > image_parts.size()) {
            LOG(ERROR)
                << "Mismatched parts for KTX, ran out of parts for face mip";
            return ByteArray();
          }
          std::vector<KtxImage::ImagePtr> image_mip;
          for (size_t mip_i = i;
               (mip_i < image_parts.size()) && (mip_i < mip_end); ++mip_i) {
            if (image_parts[mip_i]->NumberOfMipmapLevels() > 1) {
              LOG(ERROR) << "Mismatched parts for KTX, mips in mip";
              return ByteArray();
            }
            image_mip.push_back(std::move(image_parts[mip_i]));
          }
          KtxImage::ErrorCode result =
              KtxMipmapImage::Create(&image_mip, &ktx_image);
          if (result != KtxImage::kOK) {
            LOG(ERROR) << "Mipped KTX failed: " << result;
            return ByteArray();
          }
          i = mip_end - 1;
          image_mips.push_back(std::move(ktx_image));
        }
      }
      image_parts = std::move(image_mips);
    } else {
      LOG(INFO) << "Encoding cube map KTX";
    }
    KtxImage::ErrorCode result =
        KtxCubemapImage::Create(&image_parts, &ktx_image);
    if (result != KtxImage::kOK) {
      LOG(ERROR) << "Cube mapped KTX failed: " << result;
      return ByteArray();
    }
  } else if (encode_info.mip_map) {
    LOG(INFO) << "Encoding mipped KTX";
    KtxImage::ErrorCode result =
        KtxMipmapImage::Create(&image_parts, &ktx_image);
    if (result != KtxImage::kOK) {
      LOG(ERROR) << "Mipped KTX failed: " << result;
      return ByteArray();
    }
  } else {
    LOG(INFO) << "Encoding single KTX";
    ktx_image = std::move(image_parts[0]);
  }

  return ktx_image->ToByteArray();
}

}  // namespace tool
}  // namespace lull
