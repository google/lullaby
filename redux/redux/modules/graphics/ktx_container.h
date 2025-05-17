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

#ifndef REDUX_MODULES_GRAPHICS_KTX_CONTAINER_H_
#define REDUX_MODULES_GRAPHICS_KTX_CONTAINER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/image_data.h"

namespace redux {

// Wraps an ImageData that contains a KTX Image Container.
//
// Allows users to access the individual (sub)images within the KTX.
class KtxContainer {
 public:
  // Data stored in the KTX header.
  struct Header {
    uint8_t magic[12];
    uint32_t endian;
    uint32_t type;
    uint32_t type_size;
    uint32_t format;
    uint32_t internal_format;
    uint32_t base_internal_format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_elements;
    uint32_t faces;
    uint32_t mip_levels;
    uint32_t keyvalue_data;
  };

  explicit KtxContainer(DataContainer data);
  explicit KtxContainer(ImageData image_data);
  explicit KtxContainer(std::shared_ptr<ImageData> image_data);

  // Returns the KTX header which contains useful information about the KTX
  // (such as mip_levels and number of faces).
  const Header& GetHeader() const;

  // Returns the ImageFormat of each individual image stored in the KTX.
  ImageFormat GetImageFormat() const;

  // Returns the image in the KTX at the given mip level and face index.
  ImageData GetImage(int level, int face) const;

  // Returns any metadata that may be stored in the ktx.
  absl::Span<const std::byte> GetMetadata(std::string_view key) const;

 private:
  void ValidateHeader();
  void CacheMetadata() const;

  std::shared_ptr<ImageData> ktx_data_;
  const Header* header_ = nullptr;

  // We'll lazily cache the metadata once it is requested.
  mutable absl::flat_hash_map<std::string, absl::Span<const std::byte>>
      metadata_;
};
}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_KTX_CONTAINER_H_
