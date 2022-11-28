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

#include "redux/engines/text/internal/sdf_computer.h"

#include "redux/modules/base/data_builder.h"
#include "redux/modules/math/vector.h"

// We can choose between two different implementations where the trade-off is
// between speed and quality.
#define REDUX_FAST_SDF_DISTANCE_CALCULATIONS 0

namespace redux {

// We do not want to do sdf calculations using simd.
using SdfVec2i = Vector<int, 2, false>;
using SdfVec2f = Vector<float, 2, false>;

namespace {

// Manages a 2x2 grid of values using an std::vector as the underlying storage.
template <typename T>
class Grid {
 public:
  void Reset(const SdfVec2i& size, const T& initial_value) {
    size_ = size;
    data_.clear();
    data_.resize(size.x * size.y, initial_value);
  }

  const SdfVec2i& GetSize() const { return size_; }

  void Set(const SdfVec2i& pos, T value) {
    data_[pos.x + pos.y * size_.x] = value;
  }

  const T& Get(const SdfVec2i& pos) const {
    return data_[pos.x + pos.y * size_.x];
  }

 private:
  std::vector<T> data_;
  SdfVec2i size_ = SdfVec2i::Zero();
};
}  // namespace

// Represents a large distance during computation.
static const float kLargeDistance = 1e6;

static float Clamp(float x, float lower, float upper) {
  return std::max(lower, std::min(x, upper));
}

// Approximates the distance to an image edge from a pixel using the pixel
// value and the local gradient.
static float ApproximateDistanceToEdge(float value, const SdfVec2f& gradient) {
  if (gradient.x == 0.0f || gradient.y == 0.0f) {
    // Approximate the gradient linearly using the middle of the range.
    return 0.5f - value;
  } else {
    // Since the gradients are symmetric with respect to both sign and X/Y
    // transposition, do the work in the first octant (positive gradients, x
    // gradient >= y gradient) for simplicity.
    SdfVec2f g =
        SdfVec2f(std::fabs(gradient.x), std::fabs(gradient.y)).Normalized();

    // The following isnan checks are needed because if the gradients_ Grid
    // is inverted then gradients_->Get will do a FLT_MAX - value, which will
    // mean |gradient| will contain FLT_MAX, which will cause Normalized()
    // to return NaNs on some platforms (iOS being one).  This is a very
    // common case, not a rare case, and happens for pixels near the glyph
    // edge.  If we hit this case, then we approximate linearly as above.
    if (std::isnan(g.x) || std::isnan(g.y)) {
      return 0.5f - value;
    }
    if (g.x < g.y) {
      std::swap(g.x, g.y);
    }

    const float gradient_value = 0.5f * g.y / g.x;
    if (value < gradient_value) {
      // 0 <= value < gradient_value.
      return 0.5f * (g.x + g.y) - std::sqrt(2.0f * g.x * g.y * value);
    } else if (value < 1.0f - gradient_value) {
      // gradient_value <= value <= 1 - gradient_value.
      return (0.5f - value) * g.x;
    } else {
      // 1 - gradient_value < value <= 1.
      return -0.5f * (g.x + g.y) + std::sqrt(2.0f * g.x * g.y * (1.0f - value));
    }
  }
}

class SdfComputer::Impl {
 public:
  ImageData Compute(const std::byte* bytes, const SdfVec2i& size, int padding) {
    SetSource(bytes, size, padding);
    InitializeGrids();
    ComputeGradients();
    ComputeDistances<true>(&inner_distances_);
    ComputeDistances<false>(&outer_distances_);
    SetSource();  // Clear the source data; not necessary but better to be safe.
    return GenerateImage();
  }

 private:
  void SetSource(const std::byte* bytes = nullptr,
                 const SdfVec2i& size = SdfVec2i::Zero(), int padding = 0) {
    src_image_ = bytes;
    src_size_ = size;
    src_padding_ = padding;
  }

  void InitializeGrids() {
    const SdfVec2i padded_size = src_size_ + SdfVec2i(2 * src_padding_);
    gradients_.Reset(padded_size, SdfVec2f::Zero());
    inner_distances_.Reset(padded_size, 0.0f);
    outer_distances_.Reset(padded_size, 0.0f);
  }

  template <bool Invert>
  uint8_t GetSourceValue(const SdfVec2i& pos) const {
    uint8_t value = 0;

    const int unpadded_x = pos.x - src_padding_;
    const int unpadded_y = pos.y - src_padding_;
    if (unpadded_x >= 0 && unpadded_y >= 0 && unpadded_x < src_size_.x &&
        unpadded_y < src_size_.y) {
      const int index = unpadded_x + unpadded_y * src_size_.x;
      value = static_cast<uint8_t>(src_image_[index]);
    }

    if constexpr (Invert) {
      constexpr uint8_t max = std::numeric_limits<uint8_t>::max();
      return max - value;
    } else {
      return value;
    }
  }

  template <bool Invert>
  SdfVec2f GetGradient(const SdfVec2i& pos) const {
    const SdfVec2f value = gradients_.Get(pos);
    if constexpr (Invert) {
      return SdfVec2f(std::numeric_limits<float>::max()) - value;
    } else {
      return value;
    }
  }

  ImageData GenerateImage() const {
    static constexpr float kSDFMultiplier = -16.0f;
    static constexpr float min =
        static_cast<float>(std::numeric_limits<uint8_t>::min());
    static constexpr float max =
        static_cast<float>(std::numeric_limits<uint8_t>::max());
    static constexpr float mid = 0.5f * (max + min);

    CHECK(inner_distances_.GetSize() == outer_distances_.GetSize());
    const int width = outer_distances_.GetSize().x;
    const int height = outer_distances_.GetSize().y;

    DataBuilder data(width * height * sizeof(uint8_t));
    auto buffer = data.GetAppendPtr(width * height);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const SdfVec2i pos(x, y);
        // Don't return negative distances.
        float value = outer_distances_.Get(pos) - inner_distances_.Get(pos);
        value = Clamp(value * kSDFMultiplier + mid, min, max);
        buffer[x + y * width] =
            static_cast<std::byte>(static_cast<uint8_t>(value));
      }
    }
    return ImageData(ImageFormat::Alpha8, {width, height}, data.Release());
  }

  // Computes the local gradients of an image using convolution filters.
  void ComputeGradients() {
    const int w = gradients_.GetSize().x;
    const int h = gradients_.GetSize().y;

    // The 3x3 kernel does not work at the edges, so skip those pixels.
    // TODO: Use a subset of the filter kernels for edge pixels.
    for (int y = 1; y < h - 1; ++y) {
      for (int x = 1; x < w - 1; ++x) {
        ComputeGradientAt({x, y});
      }
    }
  }

  // Applies a 3x3 filter kernel to an image pixel to get the gradients.
  void ComputeGradientAt(const SdfVec2i& pos) {
    const uint8_t value = GetSourceValue<false>(pos);
    // If the pixel is fully on or off, leave the gradient as (0,0).
    static constexpr uint8_t min = std::numeric_limits<uint8_t>::min();
    static constexpr uint8_t max = std::numeric_limits<uint8_t>::max();
    if (value == min && value == max) {
      return;
    }

    // 3x3 filter kernel. The X gradient uses the array as is and the Y gradient
    // uses the transpose.
    static const float kSqrt2 = std::sqrt(2.0f);
    static const float kFilter[3][3] = {
        {-1.0f, 0.0f, 1.0f},
        {-kSqrt2, 0.0f, kSqrt2},
        {-1.0f, 0.0f, 1.0f},
    };

    SdfVec2f filtered = SdfVec2f::Zero();
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        const float val = GetSourceValue<false>(pos + SdfVec2i(j - 1, i - 1));
        filtered[0] += kFilter[i][j] * val;
        filtered[1] += kFilter[j][i] * val;
      }
    }
    gradients_.Set(pos, filtered.Normalized());
  }

  // Computes distances.
  template <bool Invert>
  void ComputeDistances(Grid<float>* distances) {
    constexpr uint8_t max = std::numeric_limits<uint8_t>::max();

    const int w = distances->GetSize().x;
    const int h = distances->GetSize().y;

    // Do a general approximation of the distances as a first pass using the
    // calculated gradients.
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const SdfVec2i pos(x, y);
        const uint8_t src = GetSourceValue<Invert>(pos);
        const float nv = static_cast<float>(src) / max;
        const float dist = nv <= 0.0f   ? kLargeDistance
                           : nv >= 1.0f ? 0.0f
                                        : ApproximateDistanceToEdge(
                                              nv, GetGradient<Invert>(pos));
        distances->Set(pos, dist);
      }
    }

    // Keep processing while distances are being modified.
    edge_distances_.Reset(distances->GetSize(), SdfVec2i::Zero());
    bool updated = false;

#if REDUX_FAST_SDF_DISTANCE_CALCULATIONS

    do {
      updated = false;
      // Propagate from top down, starting with the second row.
      for (int y = 1; y < h; ++y) {
        // Propagate distances to the right.
        for (int x = 1; x < w; ++x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0f) {
            updated |= UpdateDistance<Invert>(distances, {x, y}, {0, -1}, dist);
          }
        }
        // Propagate distances to the left (skip the rightmost pixel).
        for (int x = w - 2; x >= 0; --x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0f) {
            updated |= UpdateDistance<Invert>(distances, {x, y}, {1, 0}, dist);
          }
        }
      }
    } while (updated);

    do {
      updated = false;
      // Propagate from bottom up, starting with the second row from the bottom.
      for (int y = h - 2; y >= 0; --y) {
        // Propagate distances to the left.
        for (int x = w - 1; x >= 0; --x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0f) {
            updated |= UpdateDistance<Invert>(distances, {x, y}, {0, 1}, dist);
          }
        }
        // Propagate distances to the right (skip the leftmost pixel).
        for (int x = 1; x < w; ++x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0f) {
            updated |= UpdateDistance<Invert>(distances, {x, y}, {-1, 0}, dist);
          }
        }
      }
    } while (updated);

#else

    do {
      updated = false;

      // Propagate from top down, starting with the second row.
      for (int y = 1; y < h; ++y) {
        // Propagate distances to the right.
        for (int x = 0; x < w; ++x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0f) {
            updated |= UpdateDistance<Invert>(distances, pos, {0, -1}, dist);
            if (x > 0) {
              updated |= UpdateDistance<Invert>(distances, pos, {-1, 0}, dist);
              updated |= UpdateDistance<Invert>(distances, pos, {-1, -1}, dist);
            }
            if (x < w - 1) {
              updated |= UpdateDistance<Invert>(distances, pos, {1, -1}, dist);
            }
          }
        }

        // Propagate distances to the left (skip the rightmost pixel).
        for (auto x = w - 2; x >= 0; --x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0f) {
            updated |= UpdateDistance<Invert>(distances, pos, {1, 0}, dist);
          }
        }
      }

      // Propagate from bottom up, starting with the second row from the bottom.
      for (auto y = h - 2; y >= 0; --y) {
        // Propagate distances to the left.
        for (auto x = w - 1; x >= 0; --x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0) {
            updated |= UpdateDistance<Invert>(distances, pos, {0, 1}, dist);
            if (x > 0) {
              updated |= UpdateDistance<Invert>(distances, pos, {-1, 1}, dist);
            }
            if (x < w - 1) {
              updated |= UpdateDistance<Invert>(distances, pos, {1, 0}, dist);
              updated |= UpdateDistance<Invert>(distances, pos, {1, 1}, dist);
            }
          }
        }

        // Propagate distances to the right (skip the leftmost pixel).
        for (auto x = 1; x < w; ++x) {
          const SdfVec2i pos(x, y);
          const float dist = distances->Get(pos);
          if (dist > 0.0) {
            updated |= UpdateDistance<Invert>(distances, pos, {-1, 0}, dist);
          }
        }
      }
    } while (updated);

#endif
  }

  // Computes the distance from |pos| to an edge pixel based on the information
  // at the pixel at |pos + offset|. If the new distance is smaller than the
  // previously calculated distance, update the distance and return true.
  template <bool Invert>
  bool UpdateDistance(Grid<float>* distances, const SdfVec2i& pos,
                      const SdfVec2i& offset, float prev_dist) {
    const SdfVec2i test_pixel = pos + offset;
    const SdfVec2i xy_dist = edge_distances_.Get(test_pixel);
    const SdfVec2i edge_pixel = test_pixel - xy_dist;
    const SdfVec2i new_xy_dist = xy_dist - offset;

    // Compute the new distance to the edge.
    float new_dist = kLargeDistance;

    // If the pixel value is zero, return kLargeDistance so that processing will
    // continue.
    const uint8_t source_value = GetSourceValue<Invert>(edge_pixel);
    if (source_value > 0) {
      // Use the length of the vector to the edge pixel to estimate the real
      // distance to the edge.
      // Optimization: This code was reworked to avoid fcmp since it is
      // expensive.
      float length = 0.f;
      if (new_xy_dist.x == 0) {
        length = static_cast<float>(std::abs(new_xy_dist.y));
      } else if (new_xy_dist.y == 0) {
        length = static_cast<float>(std::abs(new_xy_dist.x));
      } else {
        length = std::sqrt(static_cast<float>((new_xy_dist.x * new_xy_dist.x) +
                                              (new_xy_dist.y * new_xy_dist.y)));
      }

      const SdfVec2f gradient = length > 0.0f ? SdfVec2f(new_xy_dist)
                                              : GetGradient<Invert>(edge_pixel);
      const float normalized_value = static_cast<float>(source_value) /
                                     std::numeric_limits<uint8_t>::max();
      const float dist = ApproximateDistanceToEdge(normalized_value, gradient);
      new_dist = length + dist;
    }

    static const float kEpsilon = 1e-3f;
    if (new_dist >= prev_dist - kEpsilon) {
      return false;
    }

    distances->Set(pos, new_dist);
    edge_distances_.Set(pos, new_xy_dist);
    return true;
  }

  const std::byte* src_image_ = nullptr;
  SdfVec2i src_size_ = SdfVec2i::Zero();
  int src_padding_ = 0;

  Grid<SdfVec2f> gradients_;       // Local gradients in X and Y.
  Grid<SdfVec2i> edge_distances_;  // Pixel distances in X and Y to edges.
  Grid<float> inner_distances_;    // Final inner distance values.
  Grid<float> outer_distances_;    // Final outer distance values.
};

SdfComputer::SdfComputer() : impl_(std::make_unique<Impl>()) {}

SdfComputer::~SdfComputer() = default;

ImageData SdfComputer::Compute(const std::byte* bytes, const SdfVec2i& size,
                               int padding) {
  return impl_->Compute(bytes, size, padding);
}
}  // namespace redux
