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

#ifndef REDUX_MODULES_MATH_TESTING_H_
#define REDUX_MODULES_MATH_TESTING_H_

#include <iostream>

#include "gmock/gmock.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

template <typename Scalar, int Dimensions, bool AllowSimd>
bool TestMatch(const Vector<Scalar, Dimensions, AllowSimd>& actual,
               const Vector<Scalar, Dimensions, AllowSimd>& expected,
               Scalar tolerance, ::testing::MatchResultListener* listener) {
  for (int i = 0; i < Dimensions; ++i) {
    const Scalar diff = std::abs(actual[i] - expected[i]);
    if (diff > tolerance) {
      *listener << " with an error of " << diff << " at " << i;
      return false;
    }
  }
  return true;
}

template <typename Scalar, bool AllowSimd>
bool TestMatch(const Quaternion<Scalar, AllowSimd>& actual,
               const Quaternion<Scalar, AllowSimd>& expected, Scalar tolerance,
               ::testing::MatchResultListener* listener) {
  for (int i = 0; i < 4; ++i) {
    const Scalar diff = std::abs(actual[i] - expected[i]);
    if (diff > tolerance) {
      *listener << " with an error of " << diff << " at " << i;
      return false;
    }
  }
  return true;
}

template <typename Scalar, int Rows, int Cols, bool AllowSimd>
bool TestMatch(const Matrix<Scalar, Rows, Cols, AllowSimd>& actual,
               const Matrix<Scalar, Rows, Cols, AllowSimd>& expected,
               Scalar tolerance, ::testing::MatchResultListener* listener) {
  for (int cc = 0; cc < Cols; ++cc) {
    for (int rr = 0; rr < Rows; ++rr) {
      const Scalar diff = std::abs(actual(rr, cc) - expected(rr, cc));
      if (diff > tolerance) {
        *listener << " with an error of " << diff << " at " << rr << "," << cc;
        return false;
      }
    }
  }
  return true;
}

template <typename Scalar, int Dimensions, bool AllowSimd>
void PrintTo(const Vector<Scalar, Dimensions, AllowSimd>& value,
             std::ostream* os) {
  *os << "(";
  for (int i = 0; i < Dimensions; ++i) {
    const bool last = (i == Dimensions - 1);
    *os << value[i] << (last ? ")" : ", ");
  }
}

template <typename Scalar, bool AllowSimd>
void PrintTo(const Quaternion<Scalar, AllowSimd>& value, std::ostream* os) {
  *os << "(";
  for (int i = 0; i < 4; ++i) {
    const bool last = (i == 3);
    *os << value[i] << (last ? ")" : ", ");
  }
}

template <typename Scalar, int Rows, int Cols, bool AllowSimd>
void PrintTo(const Matrix<Scalar, Rows, Cols, AllowSimd>& value,
             std::ostream* os) {
  *os << "(";
  for (int cc = 0; cc < Cols; ++cc) {
    for (int rr = 0; rr < Rows; ++rr) {
      const bool last = (cc == Cols - 1 && rr == Rows - 1);
      *os << value(rr, cc) << (last ? ")" : ", ");
    }
  }
}

template <typename T, typename U>
void DescribeTestMatch(std::ostream* os, const T& value, U tolerance) {
  if (tolerance > 0) {
    *os << "is approximately ";
    PrintTo(value, os);
    *os << " with a tolerance of " << tolerance;
  } else {
    *os << "is equal to ";
    PrintTo(value, os);
  }
}

template <typename Type>
class MathTestMatcher : public ::testing::MatcherInterface<Type> {
 public:
  using Scalar = typename Type::Scalar;
  using Listener = ::testing::MatchResultListener;

  explicit MathTestMatcher(const Type& expected, Scalar tolerance = 0)
      : expected_(expected), tolerance_(tolerance) {
    CHECK(tolerance_ >= 0) << "Tolerance must not be negative";
  }

  bool MatchAndExplain(Type actual, Listener* listener) const override {
    return TestMatch(actual, expected_, tolerance_, listener);
  }

  void DescribeTo(std::ostream* os) const override {
    DescribeTestMatch(os, expected_, tolerance_);
  }

 private:
  Type expected_;
  Scalar tolerance_;
};

template <typename T>
::testing::Matcher<T> MathEq(const vec2& actual) {
  return ::testing::MakeMatcher(new MathTestMatcher<T>(actual));
}

template <typename T, typename U>
::testing::Matcher<T> MathNear(const T& actual, U tolerance) {
  return ::testing::MakeMatcher(new MathTestMatcher<T>(actual, tolerance));
}

}  // namespace redux

#endif  // REDUX_MODULES_MATH_TESTING_H_
