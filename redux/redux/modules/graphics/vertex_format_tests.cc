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
#include "redux/modules/graphics/vertex_format.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(VertexAttribute, Compare) {
  VertexAttribute va1 = {VertexUsage::Position, VertexType::Vec3f};
  VertexAttribute va2 = {VertexUsage::Position, VertexType::Vec3f};
  VertexAttribute va3 = {VertexUsage::Position, VertexType::Vec2f};
  VertexAttribute va4 = {VertexUsage::Color0, VertexType::Vec3f};
  EXPECT_TRUE(va1 == va2);
  EXPECT_TRUE(va1 != va3);
  EXPECT_TRUE(va1 != va4);
  EXPECT_TRUE(va2 != va3);
  EXPECT_FALSE(va1 != va2);
  EXPECT_FALSE(va1 == va3);
  EXPECT_FALSE(va1 == va4);
  EXPECT_FALSE(va2 == va3);
}

TEST(VertexFormat, Empty) {
  const VertexFormat empty({});
  EXPECT_THAT(empty.GetVertexSize(), Eq(0));
  EXPECT_THAT(empty.GetNumAttributes(), Eq(0));
  EXPECT_TRUE(empty == empty);
}

TEST(VertexFormat, Basics) {
  const VertexFormat format({
      {VertexUsage::Position, VertexType::Vec3f},
      {VertexUsage::Normal, VertexType::Vec3f},
      {VertexUsage::Color0, VertexType::Vec4f},
  });

  EXPECT_THAT(format.GetNumAttributes(), Eq(3));
  EXPECT_THAT(format.GetAttributeAt(0)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(format.GetAttributeAt(0)->usage, Eq(VertexUsage::Position));
  EXPECT_THAT(format.GetAttributeAt(1)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(format.GetAttributeAt(1)->usage, Eq(VertexUsage::Normal));
  EXPECT_THAT(format.GetAttributeAt(2)->type, Eq(VertexType::Vec4f));
  EXPECT_THAT(format.GetAttributeAt(2)->usage, Eq(VertexUsage::Color0));
  EXPECT_THAT(format.GetAttributeAt(3), Eq(nullptr));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Position)->type,
              Eq(VertexType::Vec3f));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Position)->usage,
              Eq(VertexUsage::Position));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Normal)->type,
              Eq(VertexType::Vec3f));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Normal)->usage,
              Eq(VertexUsage::Normal));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Color0)->type,
              Eq(VertexType::Vec4f));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Color0)->usage,
              Eq(VertexUsage::Color0));
  EXPECT_THAT(format.GetAttributeWithUsage(VertexUsage::Color1), Eq(nullptr));
}

TEST(VertexFormat, Append) {
  VertexFormat format;
  format.AppendAttribute({VertexUsage::Position, VertexType::Vec3f});
  format.AppendAttribute({VertexUsage::Normal, VertexType::Vec3f});
  format.AppendAttribute({VertexUsage::Color0, VertexType::Vec4f});

  EXPECT_THAT(format.GetNumAttributes(), Eq(3));
  EXPECT_THAT(format.GetAttributeAt(0)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(format.GetAttributeAt(0)->usage, Eq(VertexUsage::Position));
  EXPECT_THAT(format.GetAttributeAt(1)->type, Eq(VertexType::Vec3f));
  EXPECT_THAT(format.GetAttributeAt(1)->usage, Eq(VertexUsage::Normal));
  EXPECT_THAT(format.GetAttributeAt(2)->type, Eq(VertexType::Vec4f));
  EXPECT_THAT(format.GetAttributeAt(2)->usage, Eq(VertexUsage::Color0));
  EXPECT_THAT(format.GetAttributeAt(3), Eq(nullptr));
}

TEST(VertexFormat, GetAttributeOffsetAt) {
  const VertexFormat format({
      {VertexUsage::Position, VertexType::Vec3f},
      {VertexUsage::Color0, VertexType::Vec4ub},
      {VertexUsage::Orientation, VertexType::Vec4f},
      {VertexUsage::TexCoord0, VertexType::Vec2f},
      {VertexUsage::TexCoord1, VertexType::Vec2f},
  });

  size_t offset = 0;
  EXPECT_THAT(format.GetAttributeOffsetAt(0), Eq(offset));

  offset += 3 * sizeof(float);
  EXPECT_THAT(format.GetAttributeOffsetAt(1), Eq(offset));

  offset += 4 * sizeof(unsigned char);
  EXPECT_THAT(format.GetAttributeOffsetAt(2), Eq(offset));

  offset += 4 * sizeof(float);
  EXPECT_THAT(format.GetAttributeOffsetAt(3), Eq(offset));

  offset += 2 * sizeof(float);
  EXPECT_THAT(format.GetAttributeOffsetAt(4), Eq(offset));
}

TEST(VertexFormat, Compare) {
  const VertexFormat format1({
      {VertexUsage::Position, VertexType::Vec3f},
      {VertexUsage::Normal, VertexType::Vec3f},
      {VertexUsage::Color0, VertexType::Vec4f},
  });
  const VertexFormat format2({
      {VertexUsage::Position, VertexType::Vec3f},
      {VertexUsage::Normal, VertexType::Vec3f},
      {VertexUsage::Color0, VertexType::Vec4f},
  });
  const VertexFormat format3({
      {VertexUsage::Position, VertexType::Vec3f},
      {VertexUsage::Color0, VertexType::Vec4f},
      {VertexUsage::Normal, VertexType::Vec3f},
  });

  EXPECT_TRUE(format1 == format2);
  EXPECT_TRUE(format1 != format3);
  EXPECT_FALSE(format1 != format2);
  EXPECT_FALSE(format1 == format3);
}

TEST(VertexFormat, AppendDeath) {
  VertexFormat format;
  for (int i = 0; i < VertexFormat::kMaxAttributes; ++i) {
    format.AppendAttribute({VertexUsage::Position, VertexType::Vec3f});
  }
  EXPECT_DEATH(
      format.AppendAttribute({VertexUsage::Position, VertexType::Vec3f}), "");
}

}  // namespace
}  // namespace redux
