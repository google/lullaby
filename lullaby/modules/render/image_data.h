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

#ifndef LULLABY_MODULES_RENDER_IMAGE_DATA_H_
#define LULLABY_MODULES_RENDER_IMAGE_DATA_H_

#include "lullaby/util/data_container.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Provides Image abstraction over arbitrary data containers.
//
// The data itself is stored in DataContainer objects moved into the ImageData
// in its constructor.
class ImageData {
 public:
  enum Format {
    kInvalid,
    kAlpha,           // Single-component alpha image, 8 bits per pixel.
    kLuminance,       // Single-component luminance image, 8 bits per pixel.
    kLuminanceAlpha,  // Two-component luminance+alpha image, 8 bits each.
    kRg88,            // RG color image, 8 bits each.
    kRgb888,          // RGB color image, 8 bits each.
    kRgba8888,        // RGBA color image, 8 bits each.
    kRgb565,          // RGB color image, 5 bits red and blue, 6 bits green.
    kRgba4444,        // RGBA color+alpha image, 4 bits each.
    kRgba5551,        // RGBA color+alpha image, 5 bits per color, 1 bit
    kAstc,            // ASTC compressed image data (container).
    kPkm,             // PKM compressed image data (container).
    kKtx,             // KTX compressed image data (container).
  };

  ImageData() {}

  // Constructs an image using the given data. If |stride| is 0, then it will
  // be set to the smallest possible value given |size| and |format|.
  ImageData(Format format, const mathfu::vec2i& size, DataContainer data,
            size_t stride = 0);

  // Returns true if no actual image data is available.
  bool IsEmpty() const { return size_.x == 0 || size_.y == 0; }

  // Returns the format of the image.
  Format GetFormat() const { return format_; }

  // Returns the dimensions of the image.
  mathfu::vec2i GetSize() const { return size_; }

  // Returns the number of bytes of data that make up the image.
  size_t GetDataSize() const { return data_.GetSize(); }

  // Returns the number of bytes between consecutive rows of pixels.
  size_t GetStride() const { return stride_; }

  // Returns the stride in pixels. That is the number of pixels per row,
  // including image padding unused as part of the image.
  int GetStrideInPixels() const;

  // Returns the alignment per row of pixel data.
  int GetRowAlignment() const;

  // Gets a const pointer to the image data as bytes. Returns nullptr if
  // the image DataContainer does not have read access.
  const uint8_t* GetBytes() const {
    return data_.IsReadable() ? data_.GetReadPtr() : nullptr;
  }

  // Gets a const pointer to the image data as bytes. Returns nullptr if
  // the image DataContainer does not have read access.
  uint8_t* GetMutableBytes() { return data_.GetData(); }

  // Creates and returns a copy with read+write access. If *this doesn't have
  // read access, logs a DFATAL and returns an empty mesh.
  ImageData CreateHeapCopy() const;

  // Returns |format|'s pixel size in bits, or 0 for container formats (eg pkm,
  // ktx).
  static size_t GetBitsPerPixel(Format format);

  // Returns |format|'s number of channels.
  static size_t GetChannelCount(Format format);

  // Calculates the data size given |size| and |format| assuming the smallest
  // possible stride. Returns 0 for container formats (eg pkm, ktx).
  static size_t CalculateDataSize(Format format, const mathfu::vec2i& size);

  // Calculates the smallest stride given |size| and |format|. Returns 0 for
  // container formats.
  static size_t CalculateMinStride(Format format, const mathfu::vec2i& size);

 private:
  Format format_ = kInvalid;
  mathfu::vec2i size_ = mathfu::kZeros2i;
  DataContainer data_;
  size_t stride_ = 0;
};

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_IMAGE_DATA_H_
