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

#include "lullaby/modules/render/image_decode_astc.h"

#include <atomic>
#include <thread>
#include <vector>

#include "astc-codec/astc-codec.h"
#include "lullaby/util/logging.h"
#include "mathfu/io.h"

namespace lull {

ImageData DecodeAstc(const mathfu::vec2i& size, const mathfu::vec2i& block,
                     int faces, const uint8_t* data, size_t len) {
  constexpr int num_footprints =
      static_cast<int>(astc_codec::FootprintType::kCount);
  static const std::pair<astc_codec::FootprintType, mathfu::vec2i>
      footprints[num_footprints] = {
          {astc_codec::FootprintType::k4x4, mathfu::vec2i(4, 4)},
          {astc_codec::FootprintType::k5x4, mathfu::vec2i(5, 4)},
          {astc_codec::FootprintType::k5x5, mathfu::vec2i(5, 5)},
          {astc_codec::FootprintType::k6x5, mathfu::vec2i(6, 5)},
          {astc_codec::FootprintType::k6x6, mathfu::vec2i(6, 6)},
          {astc_codec::FootprintType::k8x5, mathfu::vec2i(8, 5)},
          {astc_codec::FootprintType::k8x6, mathfu::vec2i(8, 6)},
          {astc_codec::FootprintType::k8x8, mathfu::vec2i(8, 8)},
          {astc_codec::FootprintType::k10x5, mathfu::vec2i(10, 5)},
          {astc_codec::FootprintType::k10x6, mathfu::vec2i(10, 6)},
          {astc_codec::FootprintType::k10x8, mathfu::vec2i(10, 8)},
          {astc_codec::FootprintType::k10x10, mathfu::vec2i(10, 10)},
          {astc_codec::FootprintType::k12x10, mathfu::vec2i(12, 10)},
          {astc_codec::FootprintType::k12x12, mathfu::vec2i(12, 12)},
      };
  auto found = std::find_if(
      footprints, footprints + num_footprints,
      [=](const std::pair<astc_codec::FootprintType, mathfu::vec2i>& x) {
        return x.second.x == block.x && x.second.y == block.y;
      });
  if (found == footprints + num_footprints) {
    LOG(FATAL) << "Invalid block size: " << block.x << " x " << block.y;
    return ImageData();
  }
  astc_codec::FootprintType footprint = found->first;

  int decoded_len = size.x * size.y * 4;
  uint8_t* decoded = new uint8_t[decoded_len * faces];
  int xblocks = (size.x + block.x - 1) / block.x;
  int yblocks = (size.y + block.y - 1) / block.y;
  int compressed_size = xblocks * yblocks * 16;
  CHECK(compressed_size <= len);
  std::vector<std::thread> threads;
  if (std::thread::hardware_concurrency() > 1) {
    threads.reserve(std::thread::hardware_concurrency());
  } else {
    // Assume systems running lullaby should have at least a few cores.
    threads.reserve(4);
  }
  int slice_rows =
      static_cast<int>((yblocks + threads.capacity() - 1) / threads.capacity());
  int decoded_row_size = block.y * size.x * 4;
  std::atomic<bool> success(true);
  for (int face = 0; face < faces; ++face) {
    for (int slice_row = 0; slice_row < yblocks; slice_row += slice_rows) {
      int rows = std::min(slice_rows, yblocks - slice_row);
      int slice_size = xblocks * rows * 16;
      int slice_height = std::min(rows * block.y, size.y - slice_row * block.y);
      int decoded_size = slice_height * size.x * 4;
      threads.emplace_back([=, &success]() {
        if (!astc_codec::ASTCDecompressToRGBA(
                data, slice_size, size.x, slice_height, footprint,
                decoded + face * decoded_len + slice_row * decoded_row_size,
                decoded_size, size.x * 4)) {
          success = false;
          return;
        }
      });
      data += slice_size;
    }
  }
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  if (!success) {
    LOG(DFATAL) << "Failed to decompress ASTC data";
    return ImageData();
  }

  mathfu::vec2i real_size(size.x, size.y * faces);
  DataContainer::DataPtr ptr(decoded, [=](const uint8_t* ptr) {
    CHECK(ptr == decoded);
    delete[] ptr;
  });
  decoded_len *= faces;
  return ImageData(ImageData::kRgba8888, real_size,
                   DataContainer(std::move(ptr), decoded_len, decoded_len,
                                 DataContainer::kRead));
}

}  // namespace lull
