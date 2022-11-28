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

#ifndef REDUX_MODULES_GRAPHICS_VERTEX_H_
#define REDUX_MODULES_GRAPHICS_VERTEX_H_

#include <tuple>
#include <type_traits>

#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/graphics/vertex_layout.h"
#include "redux/modules/graphics/vertex_utils.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

template <VertexType Type, VertexUsage Usage>
struct VertexElement : detail::VertexPayload<Type> {
  static constexpr VertexType kType = Type;
  static constexpr VertexUsage kUsage = Usage;
};

// A compile-time vertex object that supports a variety of vertex formats that
// can be used with templated functions to generate meshes.
//
// First, you declare the attributes/elements you want in your vertex:
//   using Pos3f = VertexElement<VertexType::Vec3f, VertexUsage::Position>;
//   using Uv2f = VertexElement<VertexType::Vec2f, VertexUsage::TexCoord0>;
//
// Next, define the vertex itself:
//   using MyVertex = Vertex<Pos3f, Uv2f>;
//
// Finally, add values to your vertex:
//   MyVertex v;
//   v.Position().Set(1, 2, 3);
//   v.Normal().Set(0, 1, 0);
//   v.TexCoord0().Set(0, 0);
//
// The resulting data layout will be a set of floats: [1, 2, 3, 0, 0].
//
// As you can see, setting the Normal is silently ignored since it was not part
// of the Vertex definition. This allows one to write generic code dealing with
// vertices without having to know the exact format of the vertex.
template <typename... Elems>
class Vertex {
 public:
  auto& Position() { return GetElement<VertexUsage::Position>(); }
  auto& Normal() { return GetElement<VertexUsage::Normal>(); }
  auto& Tangent() { return GetElement<VertexUsage::Tangent>(); }
  auto& Orientation() { return GetElement<VertexUsage::Orientation>(); }
  auto& Color0() { return GetElement<VertexUsage::Color0>(); }
  auto& Color1() { return GetElement<VertexUsage::Color1>(); }
  auto& Color2() { return GetElement<VertexUsage::Color2>(); }
  auto& Color3() { return GetElement<VertexUsage::Color3>(); }
  auto& TexCoord0() { return GetElement<VertexUsage::TexCoord0>(); }
  auto& TexCoord1() { return GetElement<VertexUsage::TexCoord1>(); }
  auto& TexCoord2() { return GetElement<VertexUsage::TexCoord2>(); }
  auto& TexCoord3() { return GetElement<VertexUsage::TexCoord3>(); }
  auto& TexCoord4() { return GetElement<VertexUsage::TexCoord4>(); }
  auto& TexCoord5() { return GetElement<VertexUsage::TexCoord5>(); }
  auto& TexCoord6() { return GetElement<VertexUsage::TexCoord6>(); }
  auto& TexCoord7() { return GetElement<VertexUsage::TexCoord7>(); }
  auto& BoneIndices() { return GetElement<VertexUsage::BoneIndices>(); }
  auto& BoneWeights() { return GetElement<VertexUsage::BoneWeights>(); }

  // Returns the VertexFormat that is defined by the template arguments.
  VertexFormat GetVertexFormat() const {
    VertexFormat format;
    static_assert(VertexFormat::kMaxAttributes == 12);
    TryAppendAttribute<0>(&format);
    TryAppendAttribute<1>(&format);
    TryAppendAttribute<2>(&format);
    TryAppendAttribute<3>(&format);
    TryAppendAttribute<4>(&format);
    TryAppendAttribute<5>(&format);
    TryAppendAttribute<6>(&format);
    TryAppendAttribute<7>(&format);
    TryAppendAttribute<8>(&format);
    TryAppendAttribute<9>(&format);
    TryAppendAttribute<10>(&format);
    TryAppendAttribute<11>(&format);
    return format;
  }

  // Sets the tangent for this vertex from given the normal.
  void TangentFromNormal(const vec3& normal) {
    using TangentT = typename std::decay<decltype(Tangent())>::type;
    if constexpr (TangentT::kType != VertexType::Invalid) {
      const vec4 tangent = CalculateTangent(normal);
      Tangent().Set(tangent.x, tangent.y, tangent.z, tangent.w);
    }
  }

  // Sets the orientation for this vertex from given the normal.
  void OrientationFromNormal(const vec3& normal) {
    using OrientationT = typename std::decay<decltype(Orientation())>::type;
    if constexpr (OrientationT::kType != VertexType::Invalid) {
      const vec4 orientation = CalculateOrientation(normal);
      Orientation().Set(orientation.x, orientation.y, orientation.z,
                        orientation.w);
    }
  }

 private:
  static constexpr size_t kNumElems = sizeof...(Elems);

  // We will return an invalid object when trying to assign a value to a
  // unspecified element to support generic programming.
  using InvalidT = VertexElement<VertexType::Invalid, VertexUsage::Invalid>;
  static InvalidT& Invalid() {
    static InvalidT dummy;
    return dummy;
  }

  template <size_t N>
  auto NthType() const {
    if constexpr (N < kNumElems) {
      return std::get<N>(data_);
    } else {
      return Invalid();
    }
  }

  template <size_t N>
  void TryAppendAttribute(VertexFormat* format) const {
    if constexpr (N < kNumElems) {
      using ElementType = decltype(NthType<N>());
      format->AppendAttribute({ElementType::kUsage, ElementType::kType});
    }
  }

  template <VertexUsage Usage>
  auto& GetElement() {
    static_assert(kNumElems < 8);

    if constexpr (decltype(NthType<0>())::kUsage == Usage) {
      return std::get<0>(data_);
    } else if constexpr (decltype(NthType<1>())::kUsage == Usage) {
      return std::get<1>(data_);
    } else if constexpr (decltype(NthType<2>())::kUsage == Usage) {
      return std::get<2>(data_);
    } else if constexpr (decltype(NthType<3>())::kUsage == Usage) {
      return std::get<3>(data_);
    } else if constexpr (decltype(NthType<4>())::kUsage == Usage) {
      return std::get<4>(data_);
    } else if constexpr (decltype(NthType<5>())::kUsage == Usage) {
      return std::get<5>(data_);
    } else if constexpr (decltype(NthType<6>())::kUsage == Usage) {
      return std::get<6>(data_);
    } else if constexpr (decltype(NthType<7>())::kUsage == Usage) {
      return std::get<7>(data_);
    } else {
      return Invalid();
    }
  }

  // We store the elements directly in an std::tuple which should provide a
  // packed layout.
  using TupleType = std::tuple<Elems...>;
  TupleType data_;
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_VERTEX_H_
