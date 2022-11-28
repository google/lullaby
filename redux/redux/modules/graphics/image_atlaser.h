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

#ifndef REDUX_MODULES_GRAPHICS_IMAGE_ATLASER_H_
#define REDUX_MODULES_GRAPHICS_IMAGE_ATLASER_H_

#include <cstddef>
#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/math/bounds.h"

namespace redux {

// Creates an image atlas by incrementally adding images into a single larger
// image.
//
// Images are packed into the atlas using a skyline texture packing algorithm.
class ImageAtlaser {
 public:
  // Creates the underlying image for that atlas with the given format and size.
  // The `padding` can be used to determine the number of pixels between
  // subimages to prevent potential bleeding.
  ImageAtlaser(ImageFormat format, const vec2i& size, int padding = 0);

  enum AddResult {
    kAddSuccessful,
    kAlreadyExists,
    kNoMoreSpace,
  };

  // Adds an image to the atlas with the given key id.
  AddResult Add(HashValue id, const ImageData& subimage);

  // Returns the number of images contained within the atlas.
  size_t GetNumSubimages() const;

  // Returns true if the atlas contains an image with the given key id.
  bool Contains(HashValue id) const;

  // Returns the Bounds (in texture uv-space) of the image within the atlas.
  // (Texture UV space is from (0,0) to (1,1) with (0,0) being the bottom left
  // corner of the atlas.)
  Bounds2f GetUvBounds(HashValue id) const;

  // Returns the dimensions of the image atlas.
  vec2i GetSize() const;

  // Returns the image data for the atlas itself.
  ImageData GetImageData() const;

 private:
  static constexpr std::size_t kInvalidIndex = -1;

  // Objects are packed towards the bottom-left corner of the bin. Placing
  // objects lower is preferred over placing objects to the left. Once an object
  // is placed, we update the "skyline" which is effectively a series of line
  // segments that span across the top of all placed objects across the entire
  // width of the bin.
  //
  // Each line segment is placed such that there are no objects above it. And,
  // while the space immediately below the line segment is occupied, there may
  // be gaps further below. We do not worry about these gaps and consider them
  // "wasted".

  // Represents a segment of the skyline.
  struct SkylineSegment {
    SkylineSegment() = default;
    SkylineSegment(int x, int y, int w) : x(x), y(y), width(w) {}

    int x = 0;
    int y = 0;
    int width = 0;
  };

  // Finds a segment over which to place an object of the given size. Returns
  // true if such a segment found, as well as the index of the segment and the
  // position over the segment where to place the object.
  bool FindSegment(const vec2i& size, std::size_t* out_index,
                   vec2i* out_pos) const;

  // Returns true if an object of the given size can be placed over the segment
  // at the specified index. Returns the y-position over the segment where the
  // object could be placed.
  bool RectangleFitsOverSegment(std::size_t index, const vec2i& size,
                                int* y) const;

  // Adds a new object of the given size at the given position to the skyline.
  // A new segment will be created at the given index.  It is assumed that the
  // index is the correct index for the position.
  void AddSkyline(std::size_t index, vec2i pos, vec2i size);

  void TryMergingNeighbors(std::size_t left, std::size_t right);

  void CopySubimage(const ImageData& subimage, const vec2i& pos);
  void CopyPixel(const ImageData& subimage, const vec2i& pos, int x, int y);

  // Converts a position in the image array into a uv co-ordinate.
  vec2 ToUv(const vec2i pos) const;

  absl::flat_hash_map<HashValue, Bounds2f> uvs_;
  std::unique_ptr<std::byte[]> pixels_;
  std::vector<SkylineSegment> skyline_;
  vec2i size_;
  ImageFormat format_;
  int padding_ = 0;
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_IMAGE_ATLASER_H_
