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

#ifndef REDUX_ENGINES_ANIMATION_SPLINE_BULK_SPLINE_EVALUATOR_H_
#define REDUX_ENGINES_ANIMATION_SPLINE_BULK_SPLINE_EVALUATOR_H_

#include <vector>
#include "absl/time/time.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/modules/math/bounds.h"

namespace redux {

// Traverses through a set of splines in a performant way.
//
// This class should be used if you have hundreds or more splines that you need
// to traverse in a uniform manner. It stores the spline data so that this
// traversal is very fast, when done in bulk, and so we can take advantage of
// SIMD on supported processors.
//
// This class maintains a current `x` value for each spline, and a current
// cubic-curve for the segment of the spline corresponding to that `x`.
// In AdvanceFrame, the `x`s are incremented. If this increment pushes us to
// the next segment of a spline, the cubic-curve is reinitialized to the next
// segment of the spline. The splines are evaluated at the current `x` in bulk.
//
// Similar to SplinePlayback, but everything is in spline time (ie. floats).
struct SplinePlayback {
  float playback_rate = 1.f;
  float blend_x = 0.f;
  float start_x = 0.f;
  float y_offset = 0.f;
  float y_scale = 1.f;
  bool repeat = false;
};

class BulkSplineEvaluator {
 public:
  using Index = int;

  BulkSplineEvaluator() = default;

  // Return the number of indices currently allocated. Each index is one
  // spline that's being evaluated.
  Index NumIndices() const { return static_cast<Index>(sources_.size()); }

  // Increase or decrease the total number of indices processed.
  //
  // This class holds a set of splines, each is given an index
  // from 0 to size - 1.
  //
  // The number of splines can be increased or decreased with SetNumIndices().
  //   - splines are allocated or removed at the highest indices
  void SetNumIndices(const Index num_indices);

  // Move the data at `old_index` into `new_index`. Move `count` indices total.
  //
  // Unused indices are still processed every frame. You can fill these index
  // holes with MoveIndex(), to move items from the last index into the hole.
  // Once all holes have been moved to the highest indices, you can call
  // SetNumIndices() to stop processing these highest indices. Note that this
  // is exactly what fplutil::IndexAllocator does. You should use that class to
  // keep your indices contiguous.
  void MoveIndices(const Index old_index, const Index new_index,
                   const Index count);

  // Copy the data at `src` into `dst`, using `alloc` to allocate new
  // CompactSplines. Copy `count` indices total.
  //
  // Because the BulkSplineEvaluator owns no CompactSpline memory, the caller
  // must provide a function capable of allocating a CompactSpline that is a
  // copy of the provided CompactSpline. The caller can use the provided
  // Index to delete the CompactSpline when it is no longer in use.
  template <typename AllocFn>
  void CopyIndices(Index dst, Index src, Index count, const AllocFn& alloc) {
    MoveIndices(src, dst, count);
    for (int i = 0; i < count; ++i) {
      sources_[dst + i].spline = alloc(dst + i, sources_[src + i].spline);
    }
  }

  // Initialize `index` to normalize into the `modular_range` range, whenever
  // the spline segment is initialized. While travelling along a segment,
  // note that the value may exit the `modular_range` range. For example, you
  // can ensure an angle stays near the [-pi, pi) range by passing that range
  // as the `modular_range` for this `index`.
  // If !modular_range.Valid(), then modular arithmetic is not used.
  void SetYRanges(const Index index, const Index count,
                  const Interval& modular_range);

  // Initialize `index` to process `s.spline` starting from `s.start_x`.
  // The Y() and Derivative() values are immediately available.
  void SetSplines(const Index index, const Index count,
                  const CompactSpline* splines, const SplinePlayback& playback);

  // Mark spline range as invalid.
  void ClearSplines(const Index index, const Index count);

  // Reposition the spline at `index` evaluate from `x`.
  // Same as calling SetSpline() with the same spline and
  // `playback.start_x = x`.
  void SetXs(const Index index, const Index count, const float x);

  // Set conversion rate from AdvanceFrame's delta_x to the speed at which
  // we traverse the spline.
  //     0   ==> paused
  //     0.5 ==> half speed (slow motion)
  //     1   ==> authored speed
  //     2   ==> double speed (fast forward)
  void SetPlaybackRates(const Index index, const Index count,
                        float playback_rate);

  // Set repeat state for splines.
  void SetRepeating(const Index index, const Index count, bool repeat);

  // Increment x and update the Y() and Derivative() values for all indices.
  // Process all indices in bulk to efficiently traverse memory and allow SIMD
  // instructions to be effective.
  void AdvanceFrame(const float delta_x);

  // Return true if the spline for `index` has valid spline data.
  bool Valid(const Index index) const;

  // Return the current x value for the spline at `index`.
  float X(const Index index) const {
    return CubicStartX(index) + cubic_xs_[index];
  }

  // Return the current y value for the spline at `index`.
  float Y(const Index index) const { return ys_[index]; }

  // Return the current y value for the spline at `index`, normalized to be
  // within the valid y_range.
  float NormalizedY(const Index index) const {
    return NormalizeY(index, ys_[index]);
  }

  // Return the current y value for splines, from index onward.
  // Since this is the most commonly called function, we keep it fast by
  // returning a pointer to the pre-calculated array. Note that we don't
  // recalculate the derivatives, etc., so that is why the interface is
  // different.
  const float* Ys(const Index index) const { return &ys_[index]; }

  // Return the current slope for the spline at `index`.
  float Derivative(const Index index) const {
    return PlaybackRate(index) * Cubic(index).Derivative(cubic_xs_[index]);
  }

  // Return the slopes for the `count` splines starting at `index`.
  // `out` is an array of length `count`.
  // TODO OPT: Write assembly versions of this function.
  void Derivatives(const Index index, const Index count, float* out) const {
    assert(Valid(index) && Valid(index + count - 1));
    for (Index i = 0; i < count; ++i) {
      out[i] = Derivative(index + i);
    }
  }

  // Return the current slope for the spline at `index`. Ignore the playback
  // rate. This is useful for times when the playback rate is 0, but you
  // still want to get information about the underlying spline.
  float DerivativeWithoutPlayback(const Index index) const {
    return Cubic(index).Derivative(cubic_xs_[index]);
  }

  // Return the slopes for the `count` splines starting at `index`, ignoring
  // the playback rate.
  // `out` is an array of length `count`.
  void DerivativesWithoutPlayback(const Index index, Index count,
                                  float* out) const {
    assert(Valid(index) && Valid(index + count - 1));
    for (Index i = 0; i < count; ++i) {
      out[i] = DerivativeWithoutPlayback(index + i);
    }
  }

  // Return the current playback rate of the spline at `index`.
  float PlaybackRate(const Index index) const { return sources_[index].rate; }

  // Return the spline that is currently being traversed at `index`.
  const CompactSpline* SourceSpline(const Index index) const {
    return sources_[index].spline;
  }

  // Return the splines currently playing back from `index` to `index + count`.
  // `splines` is an output array of length `count`.
  void Splines(const Index index, const Index count,
               const CompactSpline** splines) const;

  // Return the raw cubic curve for `index`. Useful if you need to calculate
  // the second or third derivatives (which are not calculated in
  // AdvanceFrame), or plot the curve for debug reasons.
  const CubicCurve& Cubic(const Index index) const { return cubics_[index]; }

  // Return the current x value for the current cubic. Each spline segment
  // is evaluated as a cubic that starts at x=0.
  float CubicX(const Index index) const { return cubic_xs_[index]; }

  // Return x-value at the end of the spline.
  float EndX(const Index index) const { return sources_[index].spline->EndX(); }

  // Return y-value at the end of the spline.
  float EndY(const Index index) const { return sources_[index].spline->EndY(); }

  // TODO OPT: Write assembly versions of this function.
  void EndYs(const Index index, const Index count, float* out) const {
    assert(Valid(index) && Valid(index + count - 1));
    for (Index i = 0; i < count; ++i) {
      out[i] = EndY(index + i);
    }
  }

  // Return slope at the end of the spline.
  float EndDerivative(const Index index) const {
    return PlaybackRate(index) * sources_[index].spline->EndDerivative();
  }

  // TODO OPT: Write assembly versions of this function.
  void EndDerivatives(const Index index, const Index count, float* out) const {
    assert(Valid(index) && Valid(index + count - 1));
    for (Index i = 0; i < count; ++i) {
      out[i] = EndDerivative(index + i);
    }
  }

  // Return slope at the end of the spline, at `index`. Ignore the playback
  // rate. This is useful for times when the playback rate is 0, but you
  // still want to get information about the underlying spline.
  float EndDerivativeWithoutPlayback(const Index index) const {
    return sources_[index].spline->EndDerivative();
  }

  // Return y-distance between current-y and end-y.
  // If using modular arithmetic, consider both paths to the target
  // (directly and wrapping around), and return the length of the shorter path.
  float YDifferenceToEnd(const Index index) const {
    return NormalizeY(index, EndY(index) - Y(index));
  }

  // TODO OPT: Write assembly versions of this function.
  void YDifferencesToEnd(const Index index, const Index count,
                         float* out) const {
    assert(Valid(index) && Valid(index + count - 1));
    for (Index i = 0; i < count; ++i) {
      out[i] = YDifferenceToEnd(index + i);
    }
  }

  // Apply modular arithmetic to ensure that `y` is within the valid y_range.
  float NormalizeY(const Index index, const float y) const {
    const YRange& r = y_ranges_[index];
    return r.modular_range.Size() > 0.f
               ? NormalizeCloseValueWithinInterval(r.modular_range, y)
               : y;
  }

  // True if using modular arithmetic on this `index`. Modular arithmetic is
  // used for types such as angles, which are equivalent modulo 2pi
  // (e.g. -pi and +pi represent the same angle).
  bool ModularArithmetic(const Index index) const {
    return y_ranges_[index].modular_range.Size() > 0.f;
  }

  // The modular range for values that use ModularArithmetic(). Note that Y()
  // can be outside of this range. However, we always normalize to this range
  // before blending to a new spline.
  const Interval& ModularRange(const Index index) const {
    return y_ranges_[index].modular_range;
  }

 private:
  void InitCubic(const Index index, const float start_x);
  float SplineStartX(const Index index) const {
    return sources_[index].spline->StartX();
  }
  float CubicStartX(const Index index) const {
    const Source& s = sources_[index];
    assert(s.spline != nullptr);
    return s.spline->NodeX(s.x_index);
  }
  CubicInit CalculateBlendInit(const Index index, const CompactSpline& spline,
                               const SplinePlayback& playback) const;
  void BlendToSpline(const Index index, const CompactSpline& spline,
                     const SplinePlayback& playback);
  void JumpToSpline(const Index index, const CompactSpline& spline,
                    const SplinePlayback& playback);

  // These functions have C and assembly language variants.
  void UpdateCubicXsAndGetMask(const float delta_x, uint8_t* masks);
  void UpdateCubicXsAndGetMask_C(const float delta_x, uint8_t* masks);
  size_t UpdateCubicXs(const float delta_x, Index* indices_to_init);
  size_t UpdateCubicXs_TwoSteps(const float delta_x, Index* indices_to_init);
  size_t UpdateCubicXs_OneStep(const float delta_x, Index* indices_to_init);
  void EvaluateIndex(const Index index);
  void EvaluateCubics();
  void EvaluateCubics_C();

  struct Source {
    Source()
        : rate(1.0f),
          y_offset(0.0f),
          y_scale(1.0f),
          spline(nullptr),
          x_index(kInvalidSplineIndex),
          repeat(false) {}

    Source(float rate, float y_offset, float y_scale)
        : rate(rate),
          y_offset(y_offset),
          y_scale(y_scale),
          spline(nullptr),
          x_index(kInvalidSplineIndex),
          repeat(false) {}

    // Speed at which time flows, relative to the spline's authored rate.
    //     0   ==> paused
    //     0.5 ==> half speed (slow motion)
    //     1   ==> authored speed
    //     2   ==> double speed (fast forward)
    float rate;

    // Offset that we add to spline to shift it along the y-axis.
    float y_offset;

    // Factor by which we scale the spline along the y-axis. We first scale
    // the spline along the y-axis before shifting it.
    float y_scale;

    // Pointer to the source spline node. Spline data is owned externally.
    // We neither allocate or free this pointer here.
    const CompactSpline* spline;

    // Current index into `spline`. The cubics_ valid is instantiated from
    // spline[x_index].
    CompactSplineIndex x_index;

    // If true, start again at the beginning of the spline when we reach
    // the end.
    bool repeat;
  };

  struct YRange {
    // If using modular arithmetic, hold the min and max extents of the
    // modular range. Modular ranges are used for things like angles,
    // which wrap around from -pi to +pi.
    // By default, invalid. If invalid, do not use modular arithmetic.
    Interval modular_range;
  };

  // Data is organized in struct-of-arrays format to match the algorithm`s
  // consumption of the data.
  // - The algorithm that updates x values, and detects when we must transition
  //   to the next segment of the spline looks only at data in `cubic_xs_` and
  //   `cubic_x_ends_`.
  // - The algorithm that updates `ys_` looks only at the data in `cubic_xs_`,
  //   `cubics_`, and `y_ranges_`. It writes to `ys_`.
  // These vectors grow when SetNumIndices() is called, but they never shrink.
  // So, we`ll have a few reallocs (which are slow) until the highwater mark is
  // reached. Then the cost of reallocs disappears. In this way we have a
  // reasonable tradeoff between memory conservation and runtime performance.

  // Source spline nodes and our current index into these splines.
  std::vector<Source> sources_;

  // Define the valid output values. We can clamp to a range, or wrap around to
  // a range using modular arithmetic (two modes of operation).
  std::vector<YRange> y_ranges_;

  // The current `x` value at which `cubics_` are evaluated.
  //   ys_[i] = cubics_[i].Evaluate(cubic_xs_[i])
  std::vector<float> cubic_xs_;

  // The last valid x value in `cubics_`.
  std::vector<float> cubic_x_ends_;

  // Currently active segment of sources_.spline.
  // Instantiated from
  // sources_[i].spline->CreateInitCubic(sources_[i].x_index).
  std::vector<CubicCurve> cubics_;

  // Value of the spline at `cubic_xs_`, normalized and clamped to be within
  // `y_ranges_`. Evaluated in AdvanceFrame.
  std::vector<float> ys_;

  // Stratch buffer used for internal calculations.
  std::vector<Index> scratch_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_SPLINE_BULK_SPLINE_EVALUATOR_H_
