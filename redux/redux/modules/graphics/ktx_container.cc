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

#include "redux/modules/graphics/ktx_container.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/math/vector.h"

namespace redux {

static const char kKtxMagicId[] = "\xABKTX 11\xBB\r\n\x1A\n";

// OpenGL constants used by KTX images.
static constexpr uint32_t R8 = 0x8229;
static constexpr uint32_t RG8 = 0x822B;
static constexpr uint32_t RGB565 = 0x8D62;
static constexpr uint32_t RGBA4 = 0x8056;
static constexpr uint32_t RGB8 = 0x8051;
static constexpr uint32_t R11F_G11F_B10F = 0x8C3A;
static constexpr uint32_t RGBA8 = 0x8058;

static ImageFormat ToImageFormat(const KtxContainer::Header& header) {
  switch (header.internal_format) {
    case R8:
      return ImageFormat::Alpha8;
    case RG8:
      return ImageFormat::Rg88;
    case RGB8:
      return ImageFormat::Rgb888;
    case RGB565:
      return ImageFormat::Rgb565;
    case RGBA8:
      return ImageFormat::Rgba8888;
    case RGBA4:
      return ImageFormat::Rgba4444;
    case R11F_G11F_B10F:
      return ImageFormat::Rgb_11_11_10f;
  }
  LOG(FATAL) << "Unknown ktx format: " << header.internal_format;
  return ImageFormat::Invalid;
}

KtxContainer::KtxContainer(ImageData image_data) {
  ktx_data_ = std::make_shared<ImageData>(std::move(image_data));
  ValidateHeader();
}

KtxContainer::KtxContainer(DataContainer data) {
  ktx_data_ = std::make_shared<ImageData>(ImageFormat::Ktx, vec2i::Zero(),
                                          std::move(data));
  ValidateHeader();
}

KtxContainer::KtxContainer(std::shared_ptr<ImageData> image_data)
    : ktx_data_(std::move(image_data)) {
  ValidateHeader();
}

void KtxContainer::ValidateHeader() {
  CHECK_GE(ktx_data_->GetNumBytes(), sizeof(Header));
  header_ = reinterpret_cast<const Header*>(ktx_data_->GetData());
  CHECK(!memcmp(header_->magic, kKtxMagicId, sizeof(header_->magic)));
  CHECK(header_->array_elements == 0 || header_->array_elements == 1);
}

const KtxContainer::Header& KtxContainer::GetHeader() const {
  CHECK(header_);
  return *header_;
}

ImageFormat KtxContainer::GetImageFormat() const {
  return ToImageFormat(*header_);
}

absl::Span<const std::byte> KtxContainer::GetMetadata(
    std::string_view key) const {
  CacheMetadata();
  if (auto iter = metadata_.find(key); iter != metadata_.end()) {
    return iter->second;
  }
  return {};
}

ImageData KtxContainer::GetImage(int mip_level, int face) const {
  CHECK_LT(face, header_->faces);
  CHECK_LT(mip_level, header_->mip_levels);

  // Get the pointer to the data payload.
  const std::byte* data = reinterpret_cast<const std::byte*>(header_) +
                          sizeof(Header) + header_->keyvalue_data;

  int32_t mip_face_image_size = 0;
  vec2i dimensions(header_->width, header_->height);
  for (int i = 0; i < mip_level; ++i) {
    // For cube maps imageSize is the number of bytes in each face of the
    // texture for the current LOD level, not including bytes in cubePadding
    // or mipPadding.
    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.16
    mip_face_image_size = *(reinterpret_cast<const int32_t*>(data));
    data += sizeof(mip_face_image_size);

    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.17
    // Ignore cubePadding by assume aligned data.
    CHECK_EQ((mip_face_image_size % 4), 0);

    // https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.18
    // Ignore mipPadding by assume aligned data.
    uint32_t mip_size = mip_face_image_size * header_->faces;
    CHECK_EQ((mip_size % 4), 0);

    // Move to the next mip.
    data += mip_size;
    dimensions /= 2;
  }

  mip_face_image_size = *(reinterpret_cast<const int32_t*>(data));
  data += sizeof(mip_face_image_size);

  const uint32_t face_offset = mip_face_image_size * face;
  absl::Span<const std::byte> image{data + face_offset,
                                    static_cast<size_t>(mip_face_image_size)};
  return ImageData(GetImageFormat(), dimensions,
                   DataContainer::WrapDataInSharedPtr(image, ktx_data_));
}

void KtxContainer::CacheMetadata() const {
  if (!metadata_.empty()) {
    return;
  }
  if (header_->keyvalue_data == 0) {
    return;
  }

  const std::byte* ptr = ktx_data_->GetData() + sizeof(Header);
  const std::byte* end = ptr + header_->keyvalue_data;
  while (ptr < end) {
    const uint32_t entry_size = *reinterpret_cast<const uint32_t*>(ptr);
    const uint32_t padding = 3 - ((entry_size + 3) % 4);
    ptr += sizeof(uint32_t);

    const std::byte* key_ptr = ptr;
    std::string key(reinterpret_cast<const char*>(key_ptr));

    const std::byte* value_ptr = key_ptr + key.size() + 1;
    const std::byte* value_end = ptr + entry_size;
    absl::Span<const std::byte> value(value_ptr, value_end - value_ptr);

    ptr += entry_size + padding;

    metadata_.emplace(key, value);
  }
}

}  // namespace redux
