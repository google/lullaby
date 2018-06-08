/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_TESTS_MATHFU_MATCHERS_H_
#define LULLABY_TESTS_MATHFU_MATCHERS_H_

#include "gmock/gmock.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace testing {

// Traits classes and specializations for deducing the type and number of
// elements stored withing a mathfu type.
template <typename MathfuType>
struct MathfuArrayTraits {};

template <typename T, int N>
struct MathfuArrayTraits<::mathfu::Vector<T, N>> {
  using ElementType = T;
  static const int kSize = N;
};

template <typename T>
struct MathfuArrayTraits<::mathfu::Quaternion<T>> {
  using ElementType = T;
  static const int kSize = 4;
};

template <typename T, int N, int M>
struct MathfuArrayTraits<::mathfu::Matrix<T, N, M>> {
  using ElementType = T;
  static const int kSize = N * M;
};

template <typename MathfuType>
class MathfuArrayMatcher;

// These overloads return a gMock matcher that tests for exact equality between
// mathfu types. This matcher uses an element-wise comparison, which may not be
// the most accurate method for comparing some of these types and does not
// account for types with differing elements that have equivalent physical
// interpretations. When testing types that have floating point elements, prefer
// to use the NearMathfu functions below.
//
// const mathfu::vec3 v1(1.0f, 2.0f, 3.0f);
// const mathfu::vec3 v2(1.0f, 2.0f, 3.01f);
//
// EXPECT_THAT(v1, EqualsMathfu(v1));
// EXPECT_THAT(v1, Not(EqualsMathfu(v2)));
//
template <typename MathfuType>
::testing::Matcher<MathfuType> EqualsMathfu(const MathfuType& actual) {
  return ::testing::MakeMatcher(new MathfuArrayMatcher<MathfuType>(actual));
}

// Overloads of the templated EqualsMathfu matcher which allows you to specify
// the expected value directly in the call.  For example,
//
// EXPECT_THAT(v1, EqualsMathfuVec3({1.f, 2.f, 3.f}));
// EXPECT_THAT(q1, EqualsMathfuQuat({1.f, 2.f, 3.f, 4.f}));
inline ::testing::Matcher<mathfu::vec2> EqualsMathfuVec2(
    const mathfu::vec2& actual) {
  return EqualsMathfu(actual);
}

inline ::testing::Matcher<mathfu::vec3> EqualsMathfuVec3(
    const mathfu::vec3& actual) {
  return EqualsMathfu(actual);
}

inline ::testing::Matcher<mathfu::vec4> EqualsMathfuVec4(
    const mathfu::vec4& actual) {
  return EqualsMathfu(actual);
}

inline ::testing::Matcher<mathfu::quat> EqualsMathfuQuat(
    const mathfu::quat& actual) {
  return EqualsMathfu(actual);
}

inline ::testing::Matcher<mathfu::mat4> EqualsMathfuMat4(
    const mathfu::mat4& actual) {
  return EqualsMathfu(actual);
}

// These overloads return a gMock matcher that tests for approximate equality
// between mathfu types within the specified error tolerance. Like the matcher
// above this one uses an element-wise comparison, and does not account for
// types with differing elements that have equivalent physical interpretations.
//
// const mathfu::vec3 v1(1.0f, 2.0f, 3.0f);
// const mathfu::vec3 v2(1.0f, 2.0f, 3.01f);
// const mathfu::vec3 v3(1.0f, 2.0f, 3.03f);
//
// EXPECT_THAT(v1, NearMathfu(v2, 0.02f));
// EXPECT_THAT(v1, Not(NearMathfu(v3, 0.02f)));
//
template <typename MathfuType>
::testing::Matcher<MathfuType> NearMathfu(
    const MathfuType& actual,
    typename MathfuArrayTraits<MathfuType>::ElementType tolerance) {
  return ::testing::MakeMatcher(
      new MathfuArrayMatcher<MathfuType>(actual, tolerance));
}

// Overloads of the templated EqualsMathfu matcher which allows you to specify
// the expected value directly in the call.  For example,
//
// EXPECT_THAT(v1, NearMathfuVec3({1.f, 2.f, 3.f}, 0.001f));
// EXPECT_THAT(q1, NearMathfuQuat({1.f, 2.f, 3.f, 4.f}, 0.001f));
inline ::testing::Matcher<mathfu::vec2> NearMathfuVec2(
    const mathfu::vec2& actual, float tolerance) {
  return NearMathfu(actual, tolerance);
}

inline ::testing::Matcher<mathfu::vec3> NearMathfuVec3(
    const mathfu::vec3& actual, float tolerance) {
  return NearMathfu(actual, tolerance);
}

inline ::testing::Matcher<mathfu::vec4> NearMathfuVec4(
    const mathfu::vec4& actual, float tolerance) {
  return NearMathfu(actual, tolerance);
}

inline ::testing::Matcher<mathfu::quat> NearMathfuQuat(
    const mathfu::quat& actual, float tolerance) {
  return NearMathfu(actual, tolerance);
}

inline ::testing::Matcher<mathfu::mat4> NearMathfuMat4(
    const mathfu::mat4& actual, float tolerance) {
  return NearMathfu(actual, tolerance);
}

// gMock matcher that supports mathfu types with the subscript operator and an
// optional element-wise error tolerance.
template <typename MathfuType>
class MathfuArrayMatcher : public ::testing::MatcherInterface<MathfuType> {
  using ElementType = typename MathfuArrayTraits<MathfuType>::ElementType;

 public:
  explicit MathfuArrayMatcher(const MathfuType& expected,
                              ElementType tolerance = 0)
      : expected_(expected), tolerance_(tolerance) {
    CHECK(tolerance_ >= 0) << "Tolerance must not be negative";
  }

  // Returns true iff the elements of the actual type matches the elements of
  // the expected type within the specified error tolerance.
  bool MatchAndExplain(
      MathfuType actual,
      ::testing::MatchResultListener* listener) const override {
    for (int i = 0; i < MathfuArrayTraits<MathfuType>::kSize; ++i) {
      const ElementType diff = std::abs(expected_[i] - actual[i]);
      if (diff > tolerance_) {
        *listener << "with an error of " << diff << " at element " << i;
        return false;
      }
    }
    return true;
  }

  // Prints a description of this matcher to the given ostream.
  void DescribeTo(std::ostream* os) const override {
    if (tolerance_ > 0) {
      *os << "is approximately ";
      PrintTo(expected_, os);
      *os << " with a minimum per-element error of " << tolerance_;
    } else {
      *os << "is equal to ";
      PrintTo(expected_, os);
    }
  }

 private:
  MathfuType expected_;
  ElementType tolerance_;
};

}  // namespace testing
}  // namespace lull

namespace mathfu {
// Provide some custom print functions for use with gMock. gMock first looks for
// overloads of the stream operator then for a PrintTo function. For ADL to find
// either of them they must be in the mathfu namespace, otherwise the default
// gMock print function is utilized.
template <typename T, int D>
void PrintTo(const ::mathfu::Vector<T, D>& vec, std::ostream* os) {
  *os << "(" << vec[0];
  for (int i = 1; i < D; ++i) {
    *os << ", " << vec[i];
  }
  *os << ")";
}

template <typename T>
void PrintTo(const ::mathfu::Quaternion<T>& quat, std::ostream* os) {
  *os << "(" << quat[0];
  for (int i = 1; i < 4; ++i) {
    *os << ", " << quat[i];
  }
  *os << ")";
}

template <typename T, int R, int C>
void PrintTo(const ::mathfu::Matrix<T, R, C>& matrix, std::ostream* os) {
  *os << "(" << matrix[0];
  for (int i = 1; i < R * C; ++i) {
    *os << ", " << matrix[i];
  }
  *os << ")";
}
}  // namespace mathfu

#endif  // LULLABY_TESTS_MATHFU_MATCHERS_H_
