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

#include "redux/modules/graphics/image_atlaser.h"

#include "redux/modules/graphics/image_utils.h"

namespace redux {

ImageAtlaser::ImageAtlaser(ImageFormat format, const vec2i& size, int padding)
    : size_(size), format_(format), padding_(padding) {
  // To begin, there is a single skyline that spans the bottom of the bin.
  skyline_.emplace_back(0, 0, size_.x);

  const int bytes_per_pixel = GetBitsPerPixel(format) / 8;
  const int num_bytes = size.x * size.y * bytes_per_pixel;
  pixels_ = std::make_unique<std::byte[]>(num_bytes);
}

vec2i ImageAtlaser::GetSize() const { return size_; }

std::size_t ImageAtlaser::GetNumSubimages() const { return uvs_.size(); }

bool ImageAtlaser::Contains(HashValue id) const { return uvs_.contains(id); }

Bounds2f ImageAtlaser::GetUvBounds(HashValue id) const {
  auto iter = uvs_.find(id);
  return iter != uvs_.end() ? iter->second : Bounds2f();
}

ImageData ImageAtlaser::GetImageData() const {
  const int bytes_per_pixel = GetBitsPerPixel(format_) / 8;
  std::size_t num_bytes = size_.x * size_.y * bytes_per_pixel;
  auto data = DataContainer::WrapData(pixels_.get(), num_bytes);
  return ImageData(format_, size_, std::move(data));
}

vec2 ImageAtlaser::ToUv(const vec2i pos) const {
  const float u = static_cast<float>(pos.x) / static_cast<float>(size_.x);
  const float v = static_cast<float>(pos.y) / static_cast<float>(size_.y);
  return vec2(u, v);
}

ImageAtlaser::AddResult ImageAtlaser::Add(HashValue id,
                                          const ImageData& subimage) {
  CHECK(subimage.GetFormat() == format_)
      << "Invalid image format: " << ToString(subimage.GetFormat());
  if (uvs_.contains(id)) {
    return kAlreadyExists;
  }

  vec2i pos = {0, 0};
  std::size_t index = 0;
  vec2i size = subimage.GetSize() + vec2i(2 * padding_);
  while (!FindSegment(size, &index, &pos)) {
    return kNoMoreSpace;
  }

  AddSkyline(index, pos, size);

  const vec2i uv_min = pos + vec2i(padding_, padding_);
  const vec2i uv_max = uv_min + subimage.GetSize();
  CopySubimage(subimage, uv_min);
  uvs_.emplace(id, Bounds2f(ToUv(uv_min), ToUv(uv_max)));
  return kAddSuccessful;
}

void ImageAtlaser::CopySubimage(const ImageData& subimage, const vec2i& pos) {
  const vec2i size = subimage.GetSize();
  CHECK(pos.x + size.x <= size_.x);
  CHECK(pos.y + size.y <= size_.y);

  const int bytes_per_pixel = GetBitsPerPixel(format_) / 8;
  const int bytes_per_row = size.x * bytes_per_pixel;

  const int dst_stride = size_.x * bytes_per_pixel;

  const std::byte* src_row = subimage.GetData();
  std::byte* dst_row =
      pixels_.get() + ((pos.y * dst_stride) + (pos.x * bytes_per_pixel));
  for (int y = 0; y < size.y; ++y) {
    std::memcpy(dst_row, src_row, bytes_per_row);
    src_row += subimage.GetStride();
    dst_row += dst_stride;
  }
}

bool ImageAtlaser::FindSegment(const vec2i& size, std::size_t* out_index,
                               vec2i* out_pos) const {
  std::size_t best_index = kInvalidIndex;
  int best_width = std::numeric_limits<int>::max();
  int best_height = std::numeric_limits<int>::max();

  // Look at each skyline over which the object can fit. Choose the one that is
  // closest to bottom which is also.
  for (std::size_t i = 0; i < skyline_.size(); ++i) {
    int y = 0;
    if (RectangleFitsOverSegment(i, size, &y)) {
      const SkylineSegment& segment = skyline_[i];
      const int height = y + size.y;
      if (height < best_height ||
          (height == best_height && segment.width < best_width)) {
        best_width = segment.width;
        best_height = height;
        best_index = i;
        *out_pos = vec2i(segment.x, y);
      }
    }
  }
  if (best_index == kInvalidIndex) {
    return false;
  } else {
    *out_index = best_index;
    return true;
  }
}

bool ImageAtlaser::RectangleFitsOverSegment(std::size_t index,
                                            const vec2i& size, int* y) const {
  // Segment is too close to the right edge to fit the object.
  const int x = skyline_[index].x;
  if (x + size.x > size_.x) {
    return false;
  }

  // We need to see if the skyline starting at this segment is big enough to
  // have the object above it. Ideally, this skyline segment on its own is wide
  // enough and has enough space directly above it (in which case we'll only
  // go through the loop once). However, we may need to span across multiple
  // skyline segments in which case we have to fit above the highest one, but
  // starting at the x-position of this first segment.
  *y = skyline_[index].y;
  int width_remaining = size.x;

  while (width_remaining > 0) {
    CHECK_LT(index, skyline_.size());
    const SkylineSegment& segment = skyline_[index];

    // Move our position up for higher segments.
    if (segment.y > *y) {
      *y = segment.y;
    }

    // We're too high now so we will not be able to fit this object above the
    // queried segment.
    if (*y + size.y > size_.y) {
      return false;
    }

    width_remaining -= segment.width;
    ++index;
  }
  return true;
}

void ImageAtlaser::AddSkyline(std::size_t index, vec2i pos, vec2i size) {
  const SkylineSegment segment(pos.x, pos.y + size.y, size.x);
  CHECK_LE(segment.x + segment.width, size_.x);
  CHECK_LE(segment.y, size_.y);

  // Insertion should keep the skyline sorted from left to right.
  CHECK_EQ(skyline_[index].x, pos.x);
  CHECK_LE(skyline_[index].y, pos.y);
  skyline_.insert(skyline_.begin() + index, segment);

  // This new segment will "eat" into the airspace of its neighbouring segment.
  for (std::size_t i = index + 1; i < skyline_.size(); ++i) {
    SkylineSegment& prev_segment = skyline_[i - 1];
    SkylineSegment& next_segment = skyline_[i];
    CHECK_LE(prev_segment.x, next_segment.x);

    const int prev_right_edge = prev_segment.x + prev_segment.width;
    const int next_left_edge = next_segment.x;
    CHECK(prev_right_edge >= next_left_edge);

    // Segments line up, we're done this loop.
    if (prev_right_edge == next_left_edge) {
      break;
    }

    // Reduce the width of the next segment and shift it over so that it is
    // adjacent to the prev segment.
    next_segment.width = next_segment.x + next_segment.width - prev_right_edge;
    next_segment.x = prev_right_edge;
    if (next_segment.width <= 0) {
      skyline_.erase(skyline_.begin() + i);
      --i;
    } else {
      // We really shouldn't need this break because the segments line up now,
      // but breaking now is slightly more effecient.
      break;
    }
  }

  // Finally, try merging the segment with its neighbors (ie. if it is the same
  // height as its neighbor).
  if (index + 1 < skyline_.size()) {
    TryMergingNeighbors(index, index + 1);
  }
  if (index > 0) {
    TryMergingNeighbors(index - 1, index);
  }
}

void ImageAtlaser::TryMergingNeighbors(std::size_t left, std::size_t right) {
  CHECK_EQ(left + 1, right);
  CHECK_LT(right, skyline_.size());

  SkylineSegment& left_segment = skyline_[left];
  SkylineSegment& right_segment = skyline_[right];
  if (left_segment.y == right_segment.y) {
    // Merge by making the left segment larger (by the width of the right
    // segment) and removing the right segment from the skyline.
    left_segment.width += right_segment.width;
    skyline_.erase(skyline_.begin() + right);
  }
}

}  // namespace redux
