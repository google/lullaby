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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/graphics/vertex.h"

namespace redux {
namespace {

using ::testing::Eq;

using Pos3f = VertexElement<VertexType::Vec3f, VertexUsage::Position>;
using Nor3f = VertexElement<VertexType::Vec3f, VertexUsage::Normal>;
using Uv02f = VertexElement<VertexType::Vec2f, VertexUsage::TexCoord0>;

TEST(Vertex, SetPosition) {
  Vertex<Pos3f> v;
  v.Position().Set(1, 2, 3);

  // This cast is probably a bit sketchy...
  float* arr = reinterpret_cast<float*>(&v);
  CHECK(sizeof(v) == sizeof(float) * 3);
  EXPECT_THAT(arr[0], Eq(1.f));
  EXPECT_THAT(arr[1], Eq(2.f));
  EXPECT_THAT(arr[2], Eq(3.f));
}

TEST(Vertex, SetPosition2) {
  Vertex<Pos3f, Nor3f> v;
  v.Position().Set(1, 2, 3);

  // This cast is probably a bit sketchy...
  float* arr = reinterpret_cast<float*>(&v);
  CHECK(sizeof(v) == sizeof(float) * 6);
  EXPECT_THAT(arr[0], Eq(1.f));
  EXPECT_THAT(arr[1], Eq(2.f));
  EXPECT_THAT(arr[2], Eq(3.f));
  EXPECT_THAT(arr[3], Eq(0.f));
  EXPECT_THAT(arr[4], Eq(0.f));
  EXPECT_THAT(arr[5], Eq(0.f));
}

TEST(Vertex, SetPosition3) {
  Vertex<Nor3f> v;
  v.Position().Set(1, 2, 3);

  // This cast is probably a bit sketchy...
  float* arr = reinterpret_cast<float*>(&v);
  CHECK(sizeof(v) == sizeof(float) * 3);
  EXPECT_THAT(arr[0], Eq(0.f));
  EXPECT_THAT(arr[1], Eq(0.f));
  EXPECT_THAT(arr[2], Eq(0.f));
}

TEST(Vertex, GetVertexFormat) {
  Vertex<Pos3f> v1;
  VertexFormat f1 = v1.GetVertexFormat();
  EXPECT_THAT(f1.GetVertexSize(), Eq(sizeof(v1)));
  EXPECT_THAT(f1.GetNumAttributes(), Eq(1));
  EXPECT_THAT(f1.GetAttributeAt(0)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(f1.GetAttributeAt(0)->usage, Eq(VertexUsage::Position));

  Vertex<Pos3f, Nor3f> v2;
  VertexFormat f2 = v2.GetVertexFormat();
  EXPECT_THAT(f2.GetVertexSize(), Eq(sizeof(v2)));
  EXPECT_THAT(f2.GetNumAttributes(), Eq(2));
  EXPECT_THAT(f2.GetAttributeAt(0)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(f2.GetAttributeAt(0)->usage, Eq(VertexUsage::Position));
  EXPECT_THAT(f2.GetAttributeAt(1)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(f2.GetAttributeAt(1)->usage, Eq(VertexUsage::Normal));

  Vertex<Nor3f> v3;
  VertexFormat f3 = v3.GetVertexFormat();
  EXPECT_THAT(f3.GetVertexSize(), Eq(sizeof(v3)));
  EXPECT_THAT(f3.GetNumAttributes(), Eq(1));
  EXPECT_THAT(f3.GetAttributeAt(0)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(f3.GetAttributeAt(0)->usage, Eq(VertexUsage::Normal));
}

}  // namespace
}  // namespace redux
