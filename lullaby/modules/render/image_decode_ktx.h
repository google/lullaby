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

#ifndef LULLABY_MODULES_RENDER_IMAGE_DECODE_KTX_H_
#define LULLABY_MODULES_RENDER_IMAGE_DECODE_KTX_H_

#include <functional>
#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/image_decode.h"

namespace lull {

/// Header structure for KTX files.
struct KtxHeader {
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

// Returns the KtxHeader if the buffer is a KTX image file, otherwise returns
// nullptr.
const KtxHeader* GetKtxHeader(const uint8_t* data, size_t len);

// For a given KTX file, will iterate over all the images contained internally
// and invoke the provided callback.
using KtxProcessor = std::function<void(
    const uint8_t* data, size_t num_bytes_per_face, size_t num_faces,
    mathfu::vec2i dimensions, int mip_level, mathfu::vec2i block_size)>;

int ProcessKtx(const KtxHeader* header, const KtxProcessor processor);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_IMAGE_DECODE_KTX_H_
