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

#include "lullaby/tools/texture_pipeline/encode_png.h"
#include "png.h"

namespace lull {
namespace tool {

static int ImageDataFormatToComponentCount(ImageData::Format format) {
  switch (format) {
    case ImageData::kLuminance:
      return 1;
    case ImageData::kLuminanceAlpha:
      return 2;
    case ImageData::kRgb888:
      return 3;
    case ImageData::kRgba8888:
      return 4;
    default:
      LOG(DFATAL) << "Unsupported format";
      return 0;
  }
}

static int ImageDataFormatToPngColorType(ImageData::Format format) {
  switch (format) {
    case ImageData::kLuminance:
      return PNG_COLOR_TYPE_GRAY;
    case ImageData::kLuminanceAlpha:
      return PNG_COLOR_TYPE_GRAY_ALPHA;
    case ImageData::kRgb888:
      return PNG_COLOR_TYPE_RGB;
    case ImageData::kRgba8888:
      return PNG_COLOR_TYPE_RGB_ALPHA;
    default:
      LOG(DFATAL) << "Unsupported format";
      return 0;
  }
}

static void PngWriteFn(png_structp png_ptr, png_bytep data, png_size_t length) {
  auto byte_array = static_cast<ByteArray*>(png_get_io_ptr(png_ptr));
  const size_t offset = byte_array->size();
  byte_array->resize(offset + length);
  memcpy(byte_array->data() + offset, data, length);
}

static void PngFlushFn(png_structp png_ptr) {}

ByteArray EncodePng(const ImageData& src) {
  ByteArray encoded_png;

  // Create basic png struct.
  png_structp png =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop png_info = nullptr;
  png_set_write_fn(png, &encoded_png, &PngWriteFn, &PngFlushFn);

  // Set and write png header.
  if (png != nullptr) {
    png_info = png_create_info_struct(png);
  }
  const int color_type = ImageDataFormatToPngColorType(src.GetFormat());
  const int component_count = ImageDataFormatToComponentCount(src.GetFormat());
  const int bits_per_pixel = ImageData::GetBitsPerPixel(src.GetFormat());
  const int bit_depth = bits_per_pixel / component_count;
  png_set_IHDR(png, png_info, src.GetSize().x, src.GetSize().y, bit_depth,
               color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, png_info);

  // Encode pixel data.
  const int pitch = bits_per_pixel * src.GetSize().x / 8;
  for (int row = 0; row < src.GetSize().y; ++row) {
    png_write_row(png, src.GetBytes() + row * pitch);
  }

  // Write the png trailer and destroy allocated libpng objects.
  png_write_end(png, nullptr);
  png_destroy_write_struct(&png, &png_info);

  return encoded_png;
}

}  // namespace tool
}  // namespace lull
