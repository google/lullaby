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

#ifndef REDUX_ENGINES_ANIMATION_SPLINE_COMPACT_SPLINE_H_
#define REDUX_ENGINES_ANIMATION_SPLINE_COMPACT_SPLINE_H_

#include <cstdint>
#include <memory>

#include "absl/base/attributes.h"
#include "redux/engines/animation/spline/compact_spline_node.h"
#include "redux/engines/animation/spline/cubic_curve.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/vector.h"

namespace redux {

class BulkSplineEvaluator;

// Index into the spline. Some high values have special meaning (see below).
using CompactSplineIndex = uint16_t;
static constexpr CompactSplineIndex kInvalidSplineIndex =
    static_cast<CompactSplineIndex>(-1);
static constexpr CompactSplineIndex kBeforeSplineIndex =
    static_cast<CompactSplineIndex>(-2);
static constexpr CompactSplineIndex kAfterSplineIndex =
    static_cast<CompactSplineIndex>(-3);
static constexpr CompactSplineIndex kMaxSplineIndex =
    static_cast<CompactSplineIndex>(-4);

// Return true if `index` is not an index into the spline.
inline bool OutsideSpline(CompactSplineIndex index) {
  return index >= kAfterSplineIndex;
}

inline float NormalizeWildValueWithinInterval(const Interval& range, float x) {
  // Use (expensive) division to determine how many lengths we are away from
  // the normalized range.
  const float length = range.Size();
  const float units = (x - range.min) / length;
  const float whole_units = floor(units);

  // Subtract off those units to get something that (mathematically) should
  // be normalized. Due to Ting point error, it sometimes is slightly
  // outside the bounds, so we need to do a standard normalization afterwards.
  const float close = x - whole_units * length;
  const float adjustment = x <= range.min  ? length
                           : x > range.max ? -length
                                           : 0.f;
  const float normalized = close + adjustment;
  // DCHECK(ContainsExcludingStart(normalized));
  return normalized;
}

inline float NormalizeCloseValueWithinInterval(const Interval& range, float x) {
  static const int kMaxAdjustments = 4;

  // Return without change if `x` is already normalized.
  const bool below = x <= range.min;
  const bool above = x > range.max;
  if (!below && !above) {
    return x;
  }

  // Each time through the loop, we'll adjust by one length closer to the
  // valid interval.
  const float length = range.Size();
  int num_adjustments = 0;

  if (below) {
    // Keep adding until we're in the range.
    do {
      x += length;
      if (num_adjustments++ > kMaxAdjustments) {
        return NormalizeWildValueWithinInterval(range, x);
      }
    } while (x <= range.min);
  } else {
    // Keep subtracting until we're in the range.
    do {
      x -= length;
      if (num_adjustments++ > kMaxAdjustments) {
        return NormalizeWildValueWithinInterval(range, x);
      }
    } while (x > range.max);
  }
  return x;
}

enum CompactSplineAddMethod {
  kAddWithoutModification,  // Add node straight-up. No changes.
  kEnsureCubicWellBehaved,  // Insert an intermediate node, if required,
                            // to ensure cubic splines have uniform curvature.
};

// Float representation of a point on the spline.
//
// This node represents the x, y, and derivative values of a data point.
// Users can pass in an array of such nodes to CompactSpline::InitFromNodes().
// Useful when you want to specify a reasonably short spline in code.
struct UncompressedNode {
  float x = 0.f;
  float y = 0.f;
  float derivative = 0.f;
};

class CompactSpline;
using CompactSplinePtr =
    std::unique_ptr<CompactSpline, std::function<void(CompactSpline*)>>;

// Represents a smooth curve in a small amount of memory.
//
// This spline interpolates a series of (x, y, derivative) nodes to create a
// smooth curve.
//
// This class holds a series of such nodes, and aids with the construction of
// that series by inserting extra nodes when extra smoothness is required.
//
// The data in this class is compacted as quantized values. It's not intended
// to be read directly. You should use the BulkSplineEvaluator to update
// and read values from the splines in a performant manner.
class CompactSpline {
 public:
  // When a `CompactSpline` is created on the stack, it will have this many
  // nodes. This amount is sufficient for the vast majority of cases where
  // you are procedurally generating a spline. We used a fixed number instead
  // of an `std::vector` to avoid dynamic memory allocation.
  static constexpr CompactSplineIndex kDefaultMaxNodes = 7;

  CompactSpline()
      : x_granularity_(0.0f), num_nodes_(0), max_nodes_(kDefaultMaxNodes) {}
  CompactSpline(const Interval& y_range, const float x_granularity)
      : max_nodes_(kDefaultMaxNodes) {
    Init(y_range, x_granularity);
  }

  CompactSpline& operator=(const CompactSpline& rhs) {
    DCHECK(rhs.num_nodes_ <= max_nodes_);
    y_range_ = rhs.y_range_;
    x_granularity_ = rhs.x_granularity_;
    num_nodes_ = rhs.num_nodes_;
    memcpy(nodes_, rhs.nodes_, rhs.num_nodes_ * sizeof(nodes_[0]));
    return *this;
  }

  // The range of values for x and y must be specified at spline creation time
  // and cannot be changed afterwards. Empties all nodes, if we have any.
  //
  // @param y_range The upper and lower bounds for y-values in the nodes.
  //                The more narrow this is, the better the precision of the
  //                fixed point numbers. Note that you should add 10% padding
  //                here, since AddNode may insert a smoothing node that is
  //                slightly beyond the source y range.
  // @param x_granularity The minimum increment of x-values. If you're working
  //                      with a spline changes at most 30 times per second,
  //                      and your x is in units of 1/1000th of a second, then
  //                      x_granularity = 33 is a good baseline. You'll
  //                      probably want granularity around 1/50th of that
  //                      baseline value, though, since AddNode may insert
  //                      smoothing nodes at intermediate x's.
  //                      In our example here, you could set
  //                      x_granularity near 33 / 50. For ease of debugging,
  //                      an x_granularity of 0.5 or 1 is probably best.
  void Init(const Interval& y_range, const float x_granularity) {
    num_nodes_ = 0;
    y_range_ = y_range;
    x_granularity_ = x_granularity;
  }

  // Initialize the CompactSpline and add curve in the `nodes` array.
  //
  // @param nodes An array of uncompressed nodes.
  // @param num_nodes Length of the `nodes` array.
  void InitFromNodes(const UncompressedNode* nodes, size_t num_nodes);

  // Evaluate `spline` at uniform x intervals, where the distance between
  // consecutive x's is spline.LengthX() / (max_nodes() - 1). Initialize
  // this spline with the results.
  //
  // @param spline The source spline to evaluate at uniform x intervals.
  void InitFromSpline(const CompactSpline& spline);

  // Add a node to the end of the spline. Depending on the method, an
  // intermediate node may also be inserted.
  //
  // @param x Must be greater than the x-value of the last spline node. If not,
  //          this call is a nop.
  // @param y Must be within the `y_range` specified in Init().
  // @param derivative No restrictions, but excessively large values may still
  //                   result in overshoot, even with an intermediate node.
  // @param method If kAddWithoutModification, adds the node and does nothing
  //               else. If kEnsureCubicWellBehaved, adds the node and
  //               (if required) inserts another node in the middle so that
  //               the individual cubics have uniform curvature.
  //               Uniform curvature means always curving upward or always
  //               curving downward. See docs/dual_cubics.pdf for details.
  void AddNode(const float x, const float y, const float derivative,
               const CompactSplineAddMethod method = kEnsureCubicWellBehaved);

  // Add values without converting them. Useful when initializing from
  // precalculated data.
  void AddNodeVerbatim(const CompactSplineXGrain x, const CompactSplineYRung y,
                       const CompactSplineAngle angle) {
    AddNodeVerbatim(detail::CompactSplineNode(x, y, angle));
  }

  // Compress `nodes` and append them to the spline.
  //
  // @param nodes An array of uncompressed nodes.
  // @param num_nodes Length of the `nodes` array.
  void AddUncompressedNodes(const UncompressedNode* nodes, size_t num_nodes);

  // Indicate that we have stopped adding nodes and want to release the
  // remaining memory. Useful for when we have one giant buffer from which
  // we want to add many splines of (potentially unknown) various sizes.
  // We can do something like,
  //  \code{.cpp}
  //  size_t CreateSplines(char* memory_buffer, size_t memory_buffer_size) {
  //    char* buf = memory_buffer;
  //    const char* end = memory_buffer + memory_buffer_size;
  //
  //    while (MoreSplinesToCreate()) {
  //      // Allocate a spline that can hold as many nodes as buf can hold.
  //      CompactSpline* spline =
  //          CompactSpline::CreateInPlaceMaxNodes(buf, end - buf);
  //
  //      while (MoreNodesToAdd()) {
  //        // Ensure we haven't reached the end of the buffer.
  //        if (spline->num_splines() == spline->max_splines()) break;
  //
  //        // ... spline creation logic ...
  //        spline->AddNode(...);
  //      }
  //
  //      // Shrink `spline` to be the size that it actually is.
  //      spline->Finalize();
  //
  //      // Advance pointer so next spline starts where this one ends.
  //      buf += spline->Size();
  //    }
  //
  //    // Return the total bytes consumed from `memory_buffer`.
  //    return end - buf;
  //  }
  //  \endcode
  void Finalize() { max_nodes_ = num_nodes_; }

  // Remove all nodes from the spline.
  void Clear() { num_nodes_ = 0; }

  // Returns the memory occupied by this spline.
  size_t Size() const { return Size(max_nodes_); }

  // Use on an array of splines created by CreateArrayInPlace().
  // Returns the next spline in the array.
  CompactSpline* Next() { return NextAtIdx(1); }
  const CompactSpline* Next() const { return NextAtIdx(1); }

  // Use on an array of splines created by CreateArrayInPlace().
  // Returns the idx'th spline in the array.
  CompactSpline* NextAtIdx(int idx) {
    // Use union to avoid potential aliasing bugs.
    union {
      CompactSpline* spline;
      uint8_t* ptr;
    } p;
    p.spline = this;
    p.ptr += idx * Size();
    return p.spline;
  }
  const CompactSpline* NextAtIdx(int idx) const {
    return const_cast<CompactSpline*>(this)->NextAtIdx(idx);
  }

  // Return index of the first node before `x`.
  // If `x` is before the first node, return kBeforeSplineIndex.
  // If `x` is past the last node, return kAfterSplineIndex.
  //
  // @param x x-value in the spline. Most often, the x-axis represents time.
  // @param guess_index Best guess at what the index for `x` will be.
  //                    Often the caller will be traversing from low to high x,
  //                    so a good guess is the index after the current index.
  //                    If you have no idea, set to 0.
  CompactSplineIndex IndexForX(const float x,
                               const CompactSplineIndex guess_index) const;

  // If `repeat` is true, loop to x = 0 when `x` >= EndX().
  // If `repeat` is false, same as IndexForX().
  CompactSplineIndex IndexForXAllowingRepeat(
      const float x, const CompactSplineIndex guess_index, const bool repeat,
      float* final_x) const;

  // Returns closest index between 0 and NumNodes() - 1.
  // Clamps `x` to a value in the range of index.
  // `index` must be a valid value: i.e. kBeforeSplineIndex, kAfterSplineIndex,
  //  or between 0..NumNodes()-1.
  CompactSplineIndex ClampIndex(const CompactSplineIndex index, float* x) const;

  // First and last x, y, and derivatives in the spline.
  float StartX() const { return Front().X(x_granularity_); }
  float StartY() const { return Front().Y(y_range_); }
  float StartDerivative() const { return nodes_[0].Derivative(); }

  float EndX() const { return Back().X(x_granularity_); }
  float EndY() const { return Back().Y(y_range_); }
  float EndDerivative() const { return Back().Derivative(); }
  float NodeX(const CompactSplineIndex index) const;
  float NodeY(const CompactSplineIndex index) const;
  float NodeDerivative(const CompactSplineIndex index) const {
    DCHECK(index < num_nodes_);
    return nodes_[index].Derivative();
  }
  float LengthX() const { return EndX() - StartX(); }
  Interval IntervalX() const { return Interval(StartX(), EndX()); }
  const Interval& IntervalY() const { return y_range_; }

  // Calls CalculatedSlowly at `x`, with `kCurveValue` to evaluate the y value.
  // If calling from inside a loop, replace the loop with one call to Ys(),
  // which is significantly faster.
  float YCalculatedSlowly(const float x) const;

  // Fast evaluation of a subset of the x-domain of the spline.
  // Spline is evaluated from `start_x` and subsequent intervals of `delta_x`.
  // Evaluated values are returned in `ys` and, if not nullptr, `derivatives`.
  void Ys(const float start_x, const float delta_x, const size_t num_points,
          float* ys, float* derivatives = nullptr) const;

  // The start and end x-values covered by the segment after `index`.
  Interval IntervalX(const CompactSplineIndex index) const;

  // Initialization parameters for a cubic curve that starts at `index` and
  // ends at `index` + 1. Or a constant curve if `index` is kBeforeSplineIndex
  // or kAfterSplineIndex.
  CubicInit CreateCubicInit(const CompactSplineIndex index) const;

  // Returns the index of the last node in the spline.
  CompactSplineIndex LastNodeIndex() const {
    DCHECK(num_nodes_ >= 1);
    return num_nodes_ - 1;
  }

  // Returns the start index of the last segment in the spline.
  CompactSplineIndex LastSegmentIndex() const {
    DCHECK(num_nodes_ >= 2);
    return num_nodes_ - 2;
  }

  // Returns the number of nodes in this spline.
  CompactSplineIndex num_nodes() const { return num_nodes_; }
  CompactSplineIndex max_nodes() const { return max_nodes_; }

  // Return const versions of internal values. For serialization.
  const detail::CompactSplineNode* nodes() const { return nodes_; }
  const Interval& y_range() const { return y_range_; }
  float x_granularity() const { return x_granularity_; }

  // Allocate memory for a spline using global `new`.
  // @param max_nodes The maximum number of nodes that this spline class
  //                  can hold. Memory is allocated so that these nodes are
  //                  held contiguously in memory with the rest of the
  //                  class.
  static CompactSplinePtr Create(CompactSplineIndex max_nodes) {
    uint8_t* buffer = new uint8_t[Size(max_nodes)];
    CompactSpline* spline = CreateInPlace(max_nodes, buffer);
    return CompactSplinePtr(spline, [](CompactSpline* ptr) {
      delete[] reinterpret_cast<uint8_t*>(ptr);
    });
  }

  // Create a CompactSpline in the memory provided by `buffer`.
  // @param buffer chunk of memory of size CompactSpline::Size(max_nodes)
  //
  // Useful for creating small splines on the stack.
  static CompactSpline* CreateInPlace(CompactSplineIndex max_nodes,
                                      void* buffer) {
    CompactSpline* spline = new (buffer) CompactSpline();
    spline->max_nodes_ = max_nodes;
    return spline;
  }

  // Allocate memory using global `new`, and initialize it with `nodes`.
  // @param nodes An array holding the curve, in uncompressed floats.
  // @param num_nodes The length of the `nodes` array, and max nodes in the
  //                  returned spline.
  static CompactSplinePtr CreateFromNodes(const UncompressedNode* nodes,
                                          size_t num_nodes) {
    DCHECK(num_nodes <= kMaxSplineIndex);
    CompactSplinePtr spline =
        Create(static_cast<CompactSplineIndex>(num_nodes));
    spline->InitFromNodes(nodes, num_nodes);
    return spline;
  }

  // Create a CompactSpline from `nodes` in the memory provided by `buffer`.
  // @param nodes array of node data, uncompressed as floats.
  // @param num_nodes length of the `nodes` array.
  // @param buffer chunk of memory of size CompactSpline::Size(num_nodes).
  //
  // The returned CompactSpline does not need to be destroyed, but once the
  // backing memory `buffer` disappears (e.g. if `buffer` is an array on the
  // stack), you must stop referencing the returned CompactSpline.
  static CompactSpline* CreateFromNodesInPlace(const UncompressedNode* nodes,
                                               size_t num_nodes, void* buffer) {
    DCHECK(num_nodes <= kMaxSplineIndex);
    CompactSpline* spline =
        CreateInPlace(static_cast<CompactSplineIndex>(num_nodes), buffer);
    spline->InitFromNodes(nodes, num_nodes);
    return spline;
  }

  // Allocate memory using global `new`, and initialize it by evaluating
  // `source_spline` at a uniform x-interval.
  // @param source_spline Spline to evaluate. The curve in the returned spline
  //                      matches `source_spline` with its x points spaced
  //                      uniformly.
  // @param num_nodes The number of uniform x-intervals in the returned spline.
  //                  Also the max_nodes of the returned spline.
  static CompactSplinePtr CreateFromSpline(const CompactSpline& source_spline,
                                           size_t num_nodes) {
    DCHECK(num_nodes <= kMaxSplineIndex);
    CompactSplinePtr spline =
        Create(static_cast<CompactSplineIndex>(num_nodes));
    spline->InitFromSpline(source_spline);
    return spline;
  }

  // Create a CompactSpline from `source_spline` in the memory provided by
  // `buffer`.
  // @param source_spline Spline to evaluate. The curve in the returned spline
  //                      matches `source_spline` with its x points spaced
  //                      uniformly.
  // @param num_nodes The number of uniform x-intervals in the returned spline.
  //                  Also the max_nodes of the returned spline.
  // @param buffer chunk of memory of size CompactSpline::Size(num_nodes).
  static CompactSpline* CreateFromSplineInPlace(
      const CompactSpline& source_spline, size_t num_nodes, void* buffer) {
    DCHECK(num_nodes <= kMaxSplineIndex);
    CompactSpline* spline =
        CreateInPlace(static_cast<CompactSplineIndex>(num_nodes), buffer);
    spline->InitFromSpline(source_spline);
    return spline;
  }

  // Returns the size, in bytes, of a CompactSpline class with `max_nodes`
  // nodes.
  //
  // This function is useful when you want to provide your own memory buffer
  // for splines, and then pass that buffer into CreateInPlace(). Your memory
  // buffer must be at least Size().
  static size_t Size(CompactSplineIndex max_nodes) {
    // Total size of the class must be rounded up to the nearest alignment
    // so that arrays of the class are properly aligned.
    // Largest type in the class is a float.
    const size_t kAlignMask = sizeof(float) - 1;
    const size_t size =
        kBaseSize + max_nodes * sizeof(detail::CompactSplineNode);
    const size_t aligned = (size + kAlignMask) & ~kAlignMask;
    return aligned;
  }

  // Returns the size, in bytes, of an array of CompactSplines (as allocated
  // with CreateArray(), say).
  //
  // This function is useful when allocating a buffer for splines on your own,
  // from which you can then call CreateArrayInPlace().
  static size_t ArraySize(size_t num_splines, size_t num_nodes) {
    return num_splines * kBaseSize +
           num_nodes * sizeof(detail::CompactSplineNode);
  }

  // Recommend a granularity given a maximal-x value. We want to have the
  // most precise granularity when quantizing x's.
  static float RecommendXGranularity(const float max_x);

  // Callback interface for BulkEvaluate(). AddPoint() will be called
  // `num_points` times, once for every x = start_x + n * delta_x,
  // where n = 0..num_points-1.
  class BulkOutput {
   public:
    virtual ~BulkOutput() {}
    virtual void AddPoint(int point_index,
                          const BulkSplineEvaluator& evaluator) = 0;
  };

  // Called by BulkYs with the an additional BulkOutputInterface
  // parameter. BulkOutputInterface specifies the type of evaluations
  // on the splines.
  static void BulkEvaluate(const CompactSpline* const splines,
                           const size_t num_splines, const float start_x,
                           const float delta_x, const size_t num_points,
                           BulkOutput* out);

  // Fast evaluation of several splines.
  // @param splines input splines of length `num_splines`.
  // @param num_splines number of splines to evaluate.
  // @param start_x starting point for every spline.
  // @param delta_x increment for each output y.
  // @param num_points the upper dimension of the `ys` and `derivatives`
  //                   arrays.
  // @param ys two dimensional output array, ys[num_points][num_splines].
  //           ys[0] are `splines` evaluated at start_x.
  //           ys[num_points - 1] are `splines` evaluated at
  //           start_x + delta_x * num_points.
  // @param derivatives two dimensional output array, with the same indexing
  //                    as `ys`.
  static void BulkYs(const CompactSpline* const splines,
                     const size_t num_splines, const float start_x,
                     const float delta_x, const size_t num_points, float* ys,
                     float* derivatives = nullptr);

  // Fast evaluation of several splines, with mathfu::VectorPacked interface.
  // Useful for evaluate three splines which together form a mathfu::vec3,
  // for instance.
  template <int kDimensions>
  static void BulkYs(const CompactSpline* const splines, const float start_x,
                     const float delta_x, const size_t num_ys,
                     Vector<float, 3, false>* ys) {
    BulkYs(splines, kDimensions, start_x, delta_x, num_ys,
           reinterpret_cast<float*>(ys));
  }

 private:
  static const size_t kBaseSize;

  CompactSpline(const CompactSpline& rhs) : max_nodes_(rhs.max_nodes_) {
    *this = rhs;
  }

  // All other AddNode() functions end up calling this one.
  void AddNodeVerbatim(const detail::CompactSplineNode& node)
      ABSL_ATTRIBUTE_NO_SANITIZE_ADDRESS /* nodes_ has variable size */ {
    DCHECK(num_nodes_ < max_nodes_);
    nodes_[num_nodes_++] = node;
  }

  // Return true iff `x` is between the nodes at `index` and `index` + 1.
  bool IndexContainsX(const CompactSplineXGrain compact_x,
                      const CompactSplineIndex index) const;

  // Search the nodes to find the index of the first node before `x`.
  CompactSplineIndex BinarySearchIndexForX(
      const CompactSplineXGrain compact_x) const;

  // Return e.x - s.x, converted from quantized to external units.
  float WidthX(const detail::CompactSplineNode& s,
               const detail::CompactSplineNode& e) const {
    return (e.x() - s.x()) * x_granularity_;
  }

  // Create the initialization parameters for a cubic running from `s` to `e`.
  CubicInit CreateCubicInit(const detail::CompactSplineNode& s,
                            const detail::CompactSplineNode& e) const;

  const detail::CompactSplineNode& Front() const {
    DCHECK_GT(num_nodes_, 0);
    return nodes_[0];
  }

  const detail::CompactSplineNode& Back() const
      ABSL_ATTRIBUTE_NO_SANITIZE_ADDRESS /* nodes_ has variable size */ {
    DCHECK_GT(num_nodes_, 0);
    return nodes_[num_nodes_ - 1];
  }

  // Extreme values for y. See comments on Init() for details.
  Interval y_range_;

  // Minimum increment for x. See comments on Init() for details.
  float x_granularity_;

  // Length of the `nodes_` array.
  CompactSplineIndex num_nodes_;

  // Maximum length of the `nodes_` array. This may be different from
  // `kDefaultMaxNodes` if CreateInPlace() was called.
  CompactSplineIndex max_nodes_;

  // Array of key points (x, y, derivative) that describe the curve.
  // The curve is interpolated smoothly between these key points.
  // Key points are stored in quantized form, and converted back to world
  // co-ordinates by using `y_range_` and `x_granularity_`.
  // Note: This array can be longer or shorter than kDefaultMaxNodes if
  //       the class was created with CreateInPlace(). The actual length of
  //       this array is stored in max_nodes_.
  // Note: We only access using nodes_ to silence --config=asan
  //       --copt=-fsanitize=bounds, but just point to the array with the
  //       default size.
  //
  detail::CompactSplineNode* nodes_ = nodes_buffer_;
  detail::CompactSplineNode nodes_buffer_[kDefaultMaxNodes];
};

}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_SPLINE_COMPACT_SPLINE_H_
