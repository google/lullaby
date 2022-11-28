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

#ifndef REDUX_MODULES_GRAPHICS_IMAGE_DATA_H_
#define REDUX_MODULES_GRAPHICS_IMAGE_DATA_H_

#include <cstddef>
#include <memory>

#include "redux/modules/base/data_container.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Provides an Image abstraction over binary data.
class ImageData {
 public:
  ImageData() = default;

  // Constructs an image using the given data. If |stride| is 0, then it will
  // be set to the smallest possible value given |size| and |format|.
  ImageData(ImageFormat format, const vec2i& size, DataContainer data,
            std::size_t stride = 0);

  // Returns true if no actual image data is stored.
  bool IsEmpty() const { return data_.GetNumBytes() == 0; }

  // Returns the format of the image.
  ImageFormat GetFormat() const { return format_; }

  // Returns the dimensions of the image. Some formats (eg. astc) may return
  // a size of (0, 0) as they are hardware compressed formats.
  vec2i GetSize() const { return size_; }

  // Returns the number of bytes of data that make up the image.
  std::size_t GetNumBytes() const { return data_.GetNumBytes(); }

  // Returns the number of bytes between consecutive rows of pixels.
  std::size_t GetStride() const { return stride_; }

  // Returns the stride in pixels. That is the number of pixels per row,
  // including image padding unused as part of the image.
  int GetStrideInPixels() const;

  // Returns the alignment per row of pixel data.
  int GetRowAlignment() const;

  // Gets a const pointer to the image data as bytes.
  const std::byte* GetData() const { return data_.GetBytes(); }

  // Creates and returns a copy of the Image. This is useful in cases where the
  // ImageData may have been created on the stack, but needs to be heap
  // allocated in order to pass to a worker thread or the GPU.
  ImageData Clone() const;

  // Creates an ImageData around a shared_ptr to an ImageData. This allows the
  // returned ImageData to be used as a movable value type (ie. passed to the
  // render thread) while preserving the shared_ptr behaviour.
  static ImageData Rebind(std::shared_ptr<ImageData> image);

 private:
  ImageFormat format_ = ImageFormat::Invalid;
  vec2i size_ = vec2i::Zero();
  DataContainer data_;
  std::size_t stride_ = 0;
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_IMAGE_DATA_H_
