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

#ifndef REDUX_MODULES_GRAPHICS_IMAGE_UTILS_H_
#define REDUX_MODULES_GRAPHICS_IMAGE_UTILS_H_

#include <cstddef>
#include "absl/types/span.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Attempts to identify the image format by inspecting the "magic" number that
// would be encoded in the header of the image data. Returns
// ImageFormat::Invalid if unable to determine the image type.
ImageFormat IdentifyImageTypeFromHeader(absl::Span<const std::byte> header);

// Returns the |format|'s pixel size in bits. Returns 0 for compressed or
// container formats (e.g. pkm, ktx, etc.) where pixels can have arbitrary
// sizes.
std::size_t GetBitsPerPixel(ImageFormat format);

// Same as above, but returns the number of bytes, not bits.
std::size_t GetBytesPerPixel(ImageFormat format);

// Calculates the data size given |size| and |format| assuming the smallest
// possible stride. Returns 0 for container formats (e.g. pkm, ktx).
size_t CalculateDataSize(ImageFormat format, const vec2i& size);

// Calculates the smallest stride given |size| and |format|. Returns 0 for
// container formats.
std::size_t CalculateMinStride(ImageFormat format, const vec2i& size);

// Returns |format|'s number of channels.
std::size_t GetChannelCountForFormat(ImageFormat format);

// Returns true if the format is for a compressed (eg. jpg) image type.
bool IsCompressedFormat(ImageFormat format);

// Returns true if the format is for a container (eg. ktx) image type.
bool IsContainerFormat(ImageFormat format);

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_IMAGE_UTILS_H_
