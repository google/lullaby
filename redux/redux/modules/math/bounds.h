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

#ifndef REDUX_MODULES_MATH_BOUNDS_H_
#define REDUX_MODULES_MATH_BOUNDS_H_

#include "absl/types/span.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/math.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Two points in an N-dimensional space denoting a range/area/volume within
// that space.
//
// In 1D space, this is an interval/range on the line number. In 2D space, this
// can be visualized as an axis-aligned rectangle. And in 3D space, this can
// be visualized as an axis-aligned box.
//
// The dimensionality of the space is defined by the template argument, which
// must be int, float or a Vector type (e.g. vec2i, vec3f, etc.).
template <typename T>
struct Bounds {
  using Point = T;
  using Scalar = typename detail::ScalarType<T>::Type;

  // Creates a bounds that contains just the origin.
  Bounds() : min(Scalar(0)), max(Scalar(0)) {}

  // Creates a bounds that contains just the single point. Specifically, this
  // returns the Bounds range [point, point].
  explicit Bounds(const T& point) : min(point), max(point) {}

  // Creates a bounds that emcompassed the two points. The two points must
  // be defined in min/max order. Behaviour is undefined otherwise.
  Bounds(const T& min, const T& max) : min(min), max(max) {}

  // Creates a bounds that emcompasses the given points.
  explicit Bounds(absl::Span<const T> points) {
    CHECK_GT(points.size(), 0);
    min = points[0];
    max = points[0];
    for (size_t i = 0; i < points.size(); ++i) {
      *this = Included(points[i]);
    }
  }

  // Returns the distance between the two limits of the bounds.
  T Size() const { return max - min; }

  // Returns the center point of the bounds.
  T Center() const { return (max + min) / Scalar(2); }

  // Return true if `x` is in [min, max], i.e. the **inclusive** range.
  bool Contains(const T& x) const { return min <= x && x <= max; }

  // Returns a range that is scaled by `scale`. Values larger than 1 make a
  // larger range, values smaller than 1 make a smaller range.
  Bounds Scaled(Scalar scale) const {
    const T extra = Size() * (scale / Scalar(2));
    return Bounds(min - extra, max + extra);
  }

  // Returns a copy of the Bounds such that it also includes `x`. If `x` is
  // outside the bounds, the bounds are extended to include `x`, otherwise just
  // returns the original bounds.
  Bounds Included(const T& x) const { return Bounds(Min(min, x), Max(max, x)); }

  // An "Empty" bounds is one that contains no points, realized by a lower bound
  // all of whose elements are larger than the corresponding elements in the
  // upper bound. Such a state is singular and must not generally be used
  // directly; it is mainly used a sentinal value for, say, search operations
  // that do not find a result. Generally, you must first obtain a valid Bounds
  // by calling `Included` (which returns a bounds large enough to include at
  // least one point).
  static Bounds Empty() {
    if constexpr (std::is_integral_v<Scalar>) {
      const auto min_inf = std::numeric_limits<Scalar>::min();
      const auto max_inf = std::numeric_limits<Scalar>::max();
      return Bounds(T(max_inf), T(min_inf));
    } else {
      const auto min_inf = -std::numeric_limits<Scalar>::infinity();
      const auto max_inf = std::numeric_limits<Scalar>::infinity();
      return Bounds(T(max_inf), T(min_inf));
    }
  }

  bool operator==(const Bounds& rhs) const {
    return min == rhs.min && max == rhs.max;
  }
  bool operator!=(const Bounds& rhs) const {
    return min != rhs.min || max != rhs.max;
  }

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(min, ConstHash("min"));
    archive(max, ConstHash("max"));
  }

  // The two end points for the bounds. Each element of min must be less than
  // or equal to the corresponding element of max. If this invariant is not
  // maintained, then some operations may have undefined behaviour. We aren't
  // going to tell you anything more concrete, so best to just avoid such
  // values.
  T min;
  T max;
};

using Bounds1i = Bounds<int>;
using Bounds1f = Bounds<float>;
using Bounds2i = Bounds<vec2i>;
using Bounds2f = Bounds<vec2>;
using Bounds3i = Bounds<vec3i>;
using Bounds3f = Bounds<vec3>;
using Interval = Bounds1f;
using Box = Bounds3f;

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Bounds1i);
REDUX_SETUP_TYPEID(redux::Bounds1f);
REDUX_SETUP_TYPEID(redux::Bounds2i);
REDUX_SETUP_TYPEID(redux::Bounds2f);
REDUX_SETUP_TYPEID(redux::Bounds3i);
REDUX_SETUP_TYPEID(redux::Bounds3f);

#endif  // REDUX_MODULES_MATH_BOUNDS_H_
