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

#ifndef REDUX_MODULES_MATH_VECTOR_H_
#define REDUX_MODULES_MATH_VECTOR_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <numeric>

#include "redux/modules/base/typeid.h"
#include "redux/modules/math/constants.h"
#include "redux/modules/math/detail/vector_layout.h"
#include "vectorial/simd4f.h"

namespace redux {

template <typename Layout>
class VectorImpl;

template <typename Scalar, int Dimensions, bool AllowSimd>
using Vector = VectorImpl<detail::VectorLayout<Scalar, Dimensions, AllowSimd>>;

// Common vector type aliases.
using vec2i = Vector<int, 2, kEnableSimdByDefault>;
using vec3i = Vector<int, 3, kEnableSimdByDefault>;
using vec4i = Vector<int, 4, kEnableSimdByDefault>;
using vec2 = Vector<float, 2, kEnableSimdByDefault>;
using vec3 = Vector<float, 3, kEnableSimdByDefault>;
using vec4 = Vector<float, 4, kEnableSimdByDefault>;

// Similar to VectorImpl, but only extracts the Scalar type from the layout.
template <typename Layout>
using ScalarImpl = typename Layout::Scalar;

// An N-dimensional vector which uses the underlying 'Layout' to determine the
// data type (eg. int, float, double) and dimensionality of the vector.
template <typename Layout>
class VectorImpl : public Layout {
 public:
  using Scalar = typename Layout::Scalar;
  static constexpr const int kDims = Layout::kDims;
  static constexpr const bool kSimd = Layout::kSimd;

  // Creates a zero-initialized vector.
  constexpr VectorImpl() {}

  // Copies a vector from another vector, copying each element.
  constexpr VectorImpl(const VectorImpl& rhs);

  // Assigns a vector from another vector, copying each element.
  constexpr VectorImpl& operator=(const VectorImpl& rhs);

  // Creates a vector with all elements set to the given scalar value.
  explicit constexpr VectorImpl(Scalar s);

  // Creates a vector from an array of scalar values.
  // IMPORTANT: This function does not perform any bounds checking and assumes
  // the array contains the correct number of elements.
  explicit constexpr VectorImpl(const Scalar* a);

  // Creates a 2D vector from two scalar values.
  constexpr VectorImpl(Scalar s1, Scalar s2);

  // Creates a 3D vector from three scalar values.
  constexpr VectorImpl(Scalar s1, Scalar s2, Scalar s3);

  // Creates a 4D vector from four scalar values.
  constexpr VectorImpl(Scalar s1, Scalar s2, Scalar s3, Scalar s4);

  // Creates a 3D vector from one 2D vector and one scalar value.
  constexpr VectorImpl(const Vector<Scalar, 2, kSimd>& v12, Scalar s3);

  // Creates a 4D vector from one 3D vector and one scalar value.
  constexpr VectorImpl(const Vector<Scalar, 3, kSimd>& v123, Scalar s4);

  // Creates a 4D vector from two 2D vectors.
  constexpr VectorImpl(const Vector<Scalar, 2, kSimd>& v12,
                       const Vector<Scalar, 2, kSimd>& v34);

  // Creates a vector from another vector of a different type by copying each
  // element, casting if necessary. If the other vector is of smaller
  // dimensionality, then the created vector will be padded with zero-valued
  // elements.
  template <class U>
  explicit constexpr VectorImpl(const VectorImpl<U>& rhs) {
    using Other = VectorImpl<U>;
    static_assert(!std::is_same_v<Other, VectorImpl>);

    if constexpr (Other::kDims < kDims) {
      for (int i = 0; i < Other::kDims; ++i) {
        this->data[i] = static_cast<Scalar>(rhs.data[i]);
      }
      for (int i = Other::kDims; i < kDims; ++i) {
        this->data[i] = static_cast<Scalar>(0);
      }
    } else {
      for (int i = 0; i < kDims; ++i) {
        this->data[i] = static_cast<Scalar>(rhs.data[i]);
      }
    }
  }

  // Accesses the n-th element of the vector.
  // IMPORTANT: These functions do not perform any bounds checking.
  constexpr Scalar& operator[](int i) { return this->data[i]; }
  constexpr const Scalar& operator[](int i) const { return this->data[i]; }

  // Returns the length of this vector.
  constexpr auto Length() const -> Scalar;

  // Returns the squared length of this vector.
  constexpr auto LengthSquared() const -> Scalar;

  // Returns a normalized copy of this vector.
  constexpr auto Normalized() const -> VectorImpl;

  // Normalizes this vector, returning its prenormalized length.
  constexpr auto SetNormalized() -> Scalar;

  // Returns the dot product of this vector and another vector.
  constexpr auto Dot(const VectorImpl& other) const -> Scalar;

  // Returns the hadamard (or component-wise) product of this vector and another
  // vector.
  constexpr auto Hadamard(const VectorImpl& other) const -> VectorImpl;

  // Returns the cross product of this vector and another vector.
  constexpr auto Cross(const VectorImpl& other) const -> VectorImpl;

  // GLSL-style swizzle operations. Only valid for specific types (ie. you can't
  // call xyz() on a 2D vector).
  constexpr auto xy() const;
  constexpr auto zw() const;
  constexpr auto xyz() const;
  constexpr auto xyw() const;
  constexpr auto xzw() const;
  constexpr auto yzw() const;

  // Useful vector constants.
  static constexpr VectorImpl Zero();
  static constexpr VectorImpl One();
  static constexpr VectorImpl XAxis();
  static constexpr VectorImpl YAxis();
  static constexpr VectorImpl ZAxis();
  static constexpr VectorImpl WAxis();
};

// Copies a vector from another vector, copying each element.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(const VectorImpl& rhs) {
  if constexpr (kSimd) {
    this->simd = rhs.simd;
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] = rhs.data[i];
    }
  }
}

// Assigns a vector from another vector, copying each element.
template <typename T>
constexpr VectorImpl<T>& VectorImpl<T>::operator=(const VectorImpl& rhs) {
  if constexpr (kSimd) {
    this->simd = rhs.simd;
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] = rhs.data[i];
    }
  }
  return *this;
}

// Creates a vector with all elements set to the given scalar value.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(Scalar s) {
  if constexpr (kSimd) {
    this->simd = simd4f_splat(s);
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] = s;
    }
  }
}

// Creates a vector from an array of scalar values.
// IMPORTANT: This function does not perform any bounds checking and assumes
// the array contains the correct number of elements.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(const Scalar* a) {
  for (int i = 0; i < kDims; ++i) {
    this->data[i] = a[i];
  }
}

// Creates a 2D vector from two scalar values.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(Scalar s1, Scalar s2) {
  static_assert(kDims == 2);
  this->data[0] = s1;
  this->data[1] = s2;
}

// Creates a 3D vector from three scalar values.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(Scalar s1, Scalar s2, Scalar s3) {
  static_assert(kDims == 3);
  this->data[0] = s1;
  this->data[1] = s2;
  this->data[2] = s3;
}

// Creates a 4D vector from four scalar values.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(Scalar s1, Scalar s2, Scalar s3,
                                    Scalar s4) {
  static_assert(kDims == 4);
  this->data[0] = s1;
  this->data[1] = s2;
  this->data[2] = s3;
  this->data[3] = s4;
}

// Creates a 3D vector from one 2D vector and one scalar value.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(const Vector<Scalar, 2, kSimd>& v12,
                                    Scalar s3) {
  static_assert(kDims == 3);
  this->data[0] = v12.data[0];
  this->data[1] = v12.data[1];
  this->data[2] = s3;
}

// Creates a 4D vector from one 3D vector and one scalar value.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(const Vector<Scalar, 3, kSimd>& v123,
                                    Scalar s4) {
  static_assert(kDims == 4);
  this->data[0] = v123.data[0];
  this->data[1] = v123.data[1];
  this->data[2] = v123.data[2];
  this->data[3] = s4;
}

// Creates a 4D vector from two 2D vectors.
template <typename T>
constexpr VectorImpl<T>::VectorImpl(const Vector<Scalar, 2, kSimd>& v12,
                                    const Vector<Scalar, 2, kSimd>& v34) {
  static_assert(kDims == 4);
  this->data[0] = v12[0];
  this->data[1] = v12[1];
  this->data[2] = v34[0];
  this->data[3] = v34[1];
}

// Returns the scalar length of this vector.
template <typename T>
constexpr auto VectorImpl<T>::Length() const -> Scalar {
  return std::sqrt(LengthSquared());
}

// Returns the scalar length of the vector.
template <typename T>
constexpr auto Length(const VectorImpl<T>& v) {
  return v.Length();
}

// Returns the scalar squared length of this vector.
template <typename T>
constexpr auto VectorImpl<T>::LengthSquared() const -> Scalar {
  return Dot(*this);
}

// Returns the scalar squared length of the vector.
template <typename T>
constexpr auto LengthSquared(const VectorImpl<T>& v) {
  return v.LengthSquared();
}

// Returns a normalized copy of this vector.
template <typename T>
constexpr auto VectorImpl<T>::Normalized() const -> VectorImpl {
  VectorImpl result;
  if constexpr (T::kSimd) {
    if constexpr (T::kDims == 2) {
      result.simd = simd4f_normalize2(this->simd);
    } else if constexpr (T::kDims == 3) {
      result.simd = simd4f_normalize3(this->simd);
    } else if constexpr (T::kDims == 4) {
      result.simd = simd4f_normalize4(this->simd);
    }
  } else {
    const auto length = Length();
    const auto one_over_length = Scalar(1) / length;
    result = *this * one_over_length;
  }
  return result;
}

// Returns a normalized copy of the vector.
template <typename T>
constexpr auto Normalized(const VectorImpl<T>& v) {
  return v.Normalized();
}

// Normalizes this vector and returns the length of the vector before
// normalization.
template <typename T>
constexpr auto VectorImpl<T>::SetNormalized() -> Scalar {
  const auto length = Length();
  const auto one_over_length = Scalar(1) / length;
  if constexpr (kSimd) {
    this->simd = simd4f_mul(this->simd, simd4f_splat(one_over_length));
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] *= one_over_length;
    }
  }
  return length;
}

// Compares two vectors for equality. Consider using AreNearlyEqual instead if
// floating-point precision is a concern.
template <typename T>
constexpr auto operator==(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  for (int i = 0; i < T::kDims; ++i) {
    if (v1.data[i] != v2.data[i]) {
      return false;
    }
  }
  return true;
}

// Compares two vectors for inequality. Consider using !AreNearlyEqual instead
// if floating-point precision is a concern.
template <typename T>
constexpr auto operator!=(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  return !(v1 == v2);
}

// Component-wise comparison of two vectors.
template <typename T>
constexpr auto operator<(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  for (int i = 0; i < T::kDims; ++i) {
    if (!(v1.data[i] < v2.data[i])) {
      return false;
    }
  }
  return true;
}
template <typename T>
constexpr auto operator<=(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  for (int i = 0; i < T::kDims; ++i) {
    if (!(v1.data[i] <= v2.data[i])) {
      return false;
    }
  }
  return true;
}
template <typename T>
constexpr auto operator>(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  for (int i = 0; i < T::kDims; ++i) {
    if (!(v1.data[i] > v2.data[i])) {
      return false;
    }
  }
  return true;
}
template <typename T>
constexpr auto operator>=(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  for (int i = 0; i < T::kDims; ++i) {
    if (!(v1.data[i] >= v2.data[i])) {
      return false;
    }
  }
  return true;
}

// Compares two vectors for equality within a given threshold.
template <typename T>
constexpr bool AreNearlyEqual(const VectorImpl<T>& v1, const VectorImpl<T>& v2,
                              ScalarImpl<T> epsilon = kDefaultEpsilon) {
  for (int i = 0; i < T::kDims; ++i) {
    if (std::abs(v1.data[i] - v2.data[i]) > epsilon) {
      return false;
    }
  }
  return true;
}

// Negates all elements of the vector.
template <typename T>
constexpr auto operator-(const VectorImpl<T>& v) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_sub(simd4f_zero(), v.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = -v.data[i];
    }
  }
  return result;
}

// Adds a scalar to each element of a vector.
template <typename T>
constexpr auto operator+(ScalarImpl<T> s, const VectorImpl<T>& v) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_add(simd4f_splat(s), v.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = s + v.data[i];
    }
  }
  return result;
}

// Adds a scalar to each element of a vector.
template <typename T>
constexpr auto operator+(const VectorImpl<T>& v, ScalarImpl<T> s) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_add(v.simd, simd4f_splat(s));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v.data[i] + s;
    }
  }
  return result;
}

// Adds a vector to another vector.
template <typename T>
constexpr auto operator+(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_add(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v1.data[i] + v2.data[i];
    }
  }
  return result;
}

// Adds (in-place) a scalar to a vector. Returns a reference to
template <typename T>
constexpr auto& operator+=(VectorImpl<T>& v, ScalarImpl<T> s) {
  if constexpr (T::kSimd) {
    v.simd = simd4f_add(v.simd, simd4f_splat(s));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v.data[i] += s;
    }
  }
  return v;
}

// Adds (in-place) a vector to another vector.
template <typename T>
constexpr auto& operator+=(VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  if constexpr (T::kSimd) {
    v1.simd = simd4f_add(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v1.data[i] += v2.data[i];
    }
  }
  return v1;
}

// Subtracts a scalar from each element of a vector.
template <typename T>
constexpr auto operator-(ScalarImpl<T> s, const VectorImpl<T>& v) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_sub(simd4f_splat(s), v.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = s - v.data[i];
    }
  }
  return result;
}

// Subtracts a scalar from each element of a vector.
template <typename T>
constexpr auto operator-(const VectorImpl<T>& v, ScalarImpl<T> s) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_sub(v.simd, simd4f_splat(s));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v.data[i] - s;
    }
  }
  return result;
}

// Subtracts a vector from another vector.
template <typename T>
constexpr auto operator-(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_sub(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v1.data[i] - v2.data[i];
    }
  }
  return result;
}

// Subtracts (in-place) a scalar from a vector.
template <typename T>
constexpr auto& operator-=(VectorImpl<T>& v, ScalarImpl<T> s) {
  if constexpr (T::kSimd) {
    v.simd = simd4f_sub(v.simd, simd4f_splat(s));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v.data[i] -= s;
    }
  }
  return v;
}

// Subtract (in-place) a vector from another vector.
template <typename T>
constexpr auto& operator-=(VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  if constexpr (T::kSimd) {
    v1.simd = simd4f_sub(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v1.data[i] -= v2.data[i];
    }
  }
  return v1;
}

// Multiplies a scalar with each element of a vector.
template <typename T>
constexpr auto operator*(ScalarImpl<T> s, const VectorImpl<T>& v) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_mul(simd4f_splat(s), v.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = s * v.data[i];
    }
  }
  return result;
}

// Multiplies a scalar with each element of a vector.
template <typename T>
constexpr auto operator*(const VectorImpl<T>& v, ScalarImpl<T> s) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_mul(v.simd, simd4f_splat(s));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v.data[i] * s;
    }
  }
  return result;
}

// Multiplies each element of a vector with the elements of another vector.
template <typename T>
constexpr auto operator*(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_mul(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v1.data[i] * v2.data[i];
    }
  }
  return result;
}

// Multiplies (in-place) a scalar with a vector.
template <typename T>
constexpr auto& operator*=(VectorImpl<T>& v, ScalarImpl<T> s) {
  if constexpr (T::kSimd) {
    v.simd = simd4f_mul(v.simd, simd4f_splat(s));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v.data[i] *= s;
    }
  }
  return v;
}

// Multiplies (in-place) each element of a vector with the elements of another
// vector.
template <typename T>
constexpr auto& operator*=(VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  if constexpr (T::kSimd) {
    v1.simd = simd4f_mul(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v1.data[i] *= v2.data[i];
    }
  }
  return v1;
}

// Divides a scalar with each element of a vector.
template <typename T>
constexpr auto operator/(ScalarImpl<T> s, const VectorImpl<T>& v) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_div(simd4f_splat(s), v.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = s / v.data[i];
    }
  }
  return result;
}

// Divides a scalar with each element of a vector.
template <typename T>
constexpr auto operator/(const VectorImpl<T>& v, ScalarImpl<T> s) {
  VectorImpl<T> result;
  const auto inv = ScalarImpl<T>(1) / s;
  if constexpr (T::kSimd) {
    result.simd = simd4f_mul(v.simd, simd4f_splat(inv));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v.data[i] * inv;
    }
  }
  return result;
}

// Divides each element of a vector with the elements of another vector.
template <typename T>
constexpr auto operator/(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_div(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = v1.data[i] / v2.data[i];
    }
  }
  return result;
}

// Divides (in-place) a scalar with a vector.
template <typename T>
constexpr auto& operator/=(VectorImpl<T>& v, ScalarImpl<T> s) {
  const auto inv = ScalarImpl<T>(1) / s;
  if constexpr (T::kSimd) {
    v.simd = simd4f_mul(v.simd, simd4f_splat(inv));
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v.data[i] *= inv;
    }
  }
  return v;
}

// Divides (in-place) each element of a vector with the elements of another
// vector.
template <typename T>
constexpr auto& operator/=(VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  if constexpr (T::kSimd) {
    v1.simd = simd4f_div(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      v1.data[i] /= v2.data[i];
    }
  }
  return v1;
}

// Returns the dot product of this vector and another vector.
template <typename T>
constexpr auto VectorImpl<T>::Dot(const VectorImpl& other) const -> Scalar {
  if constexpr (T::kSimd) {
    if constexpr (T::kDims == 2) {
      return simd4f_get_x(simd4f_dot2(this->simd, other.simd));
    } else if constexpr (T::kDims == 3) {
      return simd4f_get_x(simd4f_dot3(this->simd, other.simd));
    } else if constexpr (T::kDims == 4) {
      return simd4f_get_x(simd4f_dot4(this->simd, other.simd));
    }
  } else {
    return std::inner_product(this->data, this->data + T::kDims, other.data,
                              Scalar{0});
  }
}

// Returns the dot product scalar of two vectors.
template <typename T>
constexpr auto Dot(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  return v1.Dot(v2);
}

// Returns the hadamard (or component-wise) product of this vector and another
// vector.
template <typename T>
constexpr auto VectorImpl<T>::Hadamard(const VectorImpl& other) const
    -> VectorImpl {
  VectorImpl result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_mul(this->simd, other.simd);
  } else {
    for (int i = 0; i < kDims; ++i) {
      result.data[i] = this->data[i] * other.data[i];
    }
  }
  return result;
}

// Returns the hadamard product vector of two vectors.
template <typename T>
constexpr auto Hadamard(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  return v1.Hadamard(v2);
}

// Returns the cross product of this vector and another vector.
template <typename T>
constexpr auto VectorImpl<T>::Cross(const VectorImpl& other) const
    -> VectorImpl {
  static_assert(T::kDims == 3, "Cross product only for 3D vectors.");
  VectorImpl result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_cross3(this->simd, other.simd);
  } else {
    result.data[0] =
        this->data[1] * other.data[2] - this->data[2] * other.data[1];
    result.data[1] =
        this->data[2] * other.data[0] - this->data[0] * other.data[2];
    result.data[2] =
        this->data[0] * other.data[1] - this->data[1] * other.data[0];
  }
  return result;
}

// Returns the cross product vector of two 3D vectors.
template <typename T>
constexpr auto Cross(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  return v1.Cross(v2);
}

// Returns a vector where each element is the min of the element of the two
// given vectors.
template <typename T>
constexpr auto Min(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_min(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = std::min(v1.data[i], v2.data[i]);
    }
  }
  return result;
}

// Returns a vector where each element is the max of the element of the two
// given vectors.
template <typename T>
constexpr auto Max(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_max(v1.simd, v2.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = std::max(v1.data[i], v2.data[i]);
    }
  }
  return result;
}

// Returns a copy of vector |v| where each element is clamped between the
// corresponding element of |lower| and |upper|.
template <typename T>
constexpr auto Clamp(const VectorImpl<T>& v, const VectorImpl<T>& lower,
                     const VectorImpl<T>& upper) {
  return Max(lower, Min(v, upper));
}

// Returns a vector that is a linear interpolation between two vectors by the
// given percentage.
template <typename T>
constexpr auto Lerp(const VectorImpl<T>& v1, const VectorImpl<T>& v2,
                    ScalarImpl<T> percent) {
  VectorImpl<T> result;
  if constexpr (T::kSimd) {
    const auto simd_percent = simd4f_splat(percent);
    const auto simd_one = simd4f_splat(1.0f);
    const auto simd_one_minus_percent = simd4f_sub(simd_one, simd_percent);
    const auto a = simd4f_mul(simd_one_minus_percent, v1.simd);
    const auto b = simd4f_mul(simd_percent, v2.simd);
    result.simd = simd4f_add(a, b);
  } else {
    const auto one_minus_percent = ScalarImpl<T>(1) - percent;
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] =
          (one_minus_percent * v1.data[i]) + (percent * v2.data[i]);
    }
  }
  return result;
}

// Returns the scalar distance between two vectors.
template <typename T>
constexpr auto DistanceBetween(const VectorImpl<T>& v1,
                               const VectorImpl<T>& v2) {
  return Length(v2 - v1);
}

// Returns the scalar squared distance between two vectors.
template <typename T>
constexpr auto DistanceSquaredBetween(const VectorImpl<T>& v1,
                                      const VectorImpl<T>& v2) {
  return LengthSquared(v2 - v1);
}

// Returns the scalar angle (in radians) between two vectors.
template <typename T>
constexpr auto AngleBetween(const VectorImpl<T>& v1, const VectorImpl<T>& v2) {
  // Applying law of cosines.
  // https://stackoverflow.com/questions/10507620/finding-the-angle-between-vectors
  const ScalarImpl<T> divisor = Length(v1) * Length(v2);
  if (divisor == ScalarImpl<T>(0)) {
    return ScalarImpl<T>(0);
  }
  const ScalarImpl<T> cos_val = Dot(v1, v2) / divisor;
  // If floating point error makes cos_val > 1, then acos will return nan.
  return cos_val <= ScalarImpl<T>(1) ? std::acos(cos_val) : ScalarImpl<T>(0);
}

// Returns a vector that is perpendicular to the supplied vector.
template <typename T>
constexpr auto PerpendicularVector(const VectorImpl<T>& v) {
  static_assert(T::kDims == 3);

  // Crossing the vector with any other (non-parallel) vector will return a
  // perpendicular vector. First try using the x-axis and, if its parallel,
  // use the y-axis instead.
  //
  // We use a fairly high epsilon for parallel testing because, if it is small,
  // we'll get a better result from a cross product with the y-axis.
  constexpr auto epsilon = static_cast<typename T::Scalar>(0.05);

  const auto result = VectorImpl<T>::XAxis().Cross(v);
  if (result.LengthSquared() > epsilon) {
    return result;
  }
  return VectorImpl<T>::YAxis().Cross(v);
}

// Implementations of VectorImpl<T> swizzle functions.

template <typename T>
constexpr auto VectorImpl<T>::xy() const {
  static_assert(T::kDims >= 2);
  return Vector<typename T::Scalar, 2, T::kSimd>(this->data[0], this->data[1]);
}

template <typename T>
constexpr auto VectorImpl<T>::xyz() const {
  static_assert(T::kDims >= 3);
  return Vector<typename T::Scalar, 3, T::kSimd>(this->data[0], this->data[1],
                                                 this->data[2]);
}

template <typename T>
constexpr auto VectorImpl<T>::xyw() const {
  static_assert(T::kDims >= 3);
  return Vector<typename T::Scalar, 3, T::kSimd>(this->data[0], this->data[1],
                                                 this->data[3]);
}

template <typename T>
constexpr auto VectorImpl<T>::xzw() const {
  static_assert(T::kDims >= 3);
  return Vector<typename T::Scalar, 3, T::kSimd>(this->data[0], this->data[2],
                                                 this->data[3]);
}

template <typename T>
constexpr auto VectorImpl<T>::yzw() const {
  static_assert(T::kDims >= 3);
  return Vector<typename T::Scalar, 3, T::kSimd>(this->data[1], this->data[2],
                                                 this->data[3]);
}

template <typename T>
constexpr auto VectorImpl<T>::zw() const {
  static_assert(T::kDims >= 4);
  return Vector<typename T::Scalar, 2, T::kSimd>(this->data[2], this->data[3]);
}

// Implementations of VectorImpl<T> static functions.

template <typename T>
constexpr VectorImpl<T> VectorImpl<T>::Zero() {
  return VectorImpl(Scalar(0));
}

template <typename T>
constexpr VectorImpl<T> VectorImpl<T>::One() {
  return VectorImpl(Scalar(1));
}

template <typename T>
constexpr VectorImpl<T> VectorImpl<T>::XAxis() {
  if constexpr (T::kDims == 2) {
    return VectorImpl(Scalar(1), Scalar(0));
  }
  if constexpr (T::kDims == 3) {
    return VectorImpl(Scalar(1), Scalar(0), Scalar(0));
  }
  if constexpr (T::kDims == 4) {
    return VectorImpl(Scalar(1), Scalar(0), Scalar(0), Scalar(0));
  }
}

template <typename T>
constexpr VectorImpl<T> VectorImpl<T>::YAxis() {
  if constexpr (T::kDims == 2) {
    return VectorImpl(Scalar(0), Scalar(1));
  }
  if constexpr (T::kDims == 3) {
    return VectorImpl(Scalar(0), Scalar(1), Scalar(0));
  }
  if constexpr (T::kDims == 4) {
    return VectorImpl(Scalar(0), Scalar(1), Scalar(0), Scalar(0));
  }
}

template <typename T>
constexpr VectorImpl<T> VectorImpl<T>::ZAxis() {
  if constexpr (T::kDims == 3) {
    return VectorImpl(Scalar(0), Scalar(0), Scalar(1));
  }
  if constexpr (T::kDims == 4) {
    return VectorImpl(Scalar(0), Scalar(0), Scalar(1), Scalar(0));
  }
}

template <typename T>
constexpr VectorImpl<T> VectorImpl<T>::WAxis() {
  if constexpr (T::kDims == 4) {
    return VectorImpl(Scalar(0), Scalar(0), Scalar(0), Scalar(1));
  }
}
}  // namespace redux

REDUX_SETUP_TYPEID(redux::vec2);
REDUX_SETUP_TYPEID(redux::vec3);
REDUX_SETUP_TYPEID(redux::vec4);
REDUX_SETUP_TYPEID(redux::vec2i);
REDUX_SETUP_TYPEID(redux::vec3i);
REDUX_SETUP_TYPEID(redux::vec4i);

#endif  // REDUX_MODULES_MATH_VECTOR_H_
