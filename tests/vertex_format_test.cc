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

#include "gtest/gtest.h"

#include "lullaby/modules/render/vertex.h"
#include "lullaby/modules/render/vertex_format.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

struct TestVertex2f {
  static const VertexFormat kFormat;

  float x;
  float y;
};

const VertexFormat TestVertex2f::kFormat({VertexAttribute(
    0, VertexAttribute::kPosition, 2, VertexAttribute::kFloat32)});

TEST(VertexFormat, Empty) {
  const VertexFormat empty({});
  EXPECT_EQ(empty.GetVertexSize(), 0U);
  EXPECT_EQ(empty.GetNumAttributes(), 0U);
  EXPECT_TRUE(empty == empty);
}

TEST(VertexFormat, VertexSize) {
  EXPECT_EQ(TestVertex2f::kFormat.GetVertexSize(), 8U);
  EXPECT_EQ(VertexP::kFormat.GetVertexSize(), 12U);
  EXPECT_EQ(VertexPC::kFormat.GetVertexSize(), 16U);
  EXPECT_EQ(VertexPN::kFormat.GetVertexSize(), 24U);
  EXPECT_EQ(VertexPT::kFormat.GetVertexSize(), 20U);
  EXPECT_EQ(VertexPTT::kFormat.GetVertexSize(), 28U);
  EXPECT_EQ(VertexPTC::kFormat.GetVertexSize(), 24U);
  EXPECT_EQ(VertexPTI::kFormat.GetVertexSize(), 24U);
  EXPECT_EQ(VertexPTN::kFormat.GetVertexSize(), 32U);
}

TEST(VertexFormat, AttributeSorting) {
  const VertexFormat test_format(
      {VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                       VertexAttribute::kFloat32),
       VertexAttribute(0, VertexAttribute::kPosition, 3,
                       VertexAttribute::kFloat32)});
  CHECK_EQ(test_format.GetNumAttributes(), 2U);
  CHECK_EQ(test_format.GetAttributeAt(0).offset, 0U);
  CHECK_EQ(test_format.GetAttributeAt(1).offset, 12U);
}

TEST(VertexFormat, GetAttributeWithUsage) {
  // Test some formats for attributes we know they have.
  const VertexFormat kFormats[] = {VertexP::kFormat, VertexPTC::kFormat,
                                   VertexPTI::kFormat, VertexPTN::kFormat,
                                   VertexPTT::kFormat};
  const size_t kNumFormats = sizeof(kFormats) / sizeof(kFormats[0]);

  for (size_t i = 0; i < kNumFormats; ++i) {
    const VertexFormat& format = kFormats[i];
    for (size_t k = 0; k < format.GetNumAttributes(); ++k) {
      const VertexAttribute& attribute = format.GetAttributeAt(k);
      EXPECT_EQ(format.GetAttributeWithUsage(attribute.usage, attribute.index),
                &attribute);
    }
  }

  // Then test for some attributes we know they lack.
  EXPECT_EQ(VertexP::kFormat.GetAttributeWithUsage(VertexAttribute::kTexCoord),
            nullptr);
  EXPECT_EQ(VertexP::kFormat.GetAttributeWithUsage(VertexAttribute::kColor),
            nullptr);
  EXPECT_EQ(VertexP::kFormat.GetAttributeWithUsage(VertexAttribute::kIndex),
            nullptr);
  EXPECT_EQ(VertexP::kFormat.GetAttributeWithUsage(VertexAttribute::kNormal),
            nullptr);

  EXPECT_EQ(VertexPTC::kFormat.GetAttributeWithUsage(VertexAttribute::kTexCoord,
                                                     /* usage_index = */ 1),
            nullptr);
  EXPECT_EQ(VertexPTC::kFormat.GetAttributeWithUsage(VertexAttribute::kIndex),
            nullptr);
  EXPECT_EQ(VertexPTC::kFormat.GetAttributeWithUsage(VertexAttribute::kNormal),
            nullptr);
}

TEST(VertexFormat, VertexMatching) {
  const VertexFormat empty({});
  EXPECT_FALSE(empty.Matches<TestVertex2f>());

  // Make sure each vertex matches its own format.
  EXPECT_TRUE(TestVertex2f::kFormat.Matches<TestVertex2f>());
  EXPECT_TRUE(VertexP::kFormat.Matches<VertexP>());
  EXPECT_TRUE(VertexPC::kFormat.Matches<VertexPC>());
  EXPECT_TRUE(VertexPN::kFormat.Matches<VertexPN>());
  EXPECT_TRUE(VertexPT::kFormat.Matches<VertexPT>());
  EXPECT_TRUE(VertexPTT::kFormat.Matches<VertexPTT>());
  EXPECT_TRUE(VertexPTC::kFormat.Matches<VertexPTC>());
  EXPECT_TRUE(VertexPTI::kFormat.Matches<VertexPTI>());
  EXPECT_TRUE(VertexPTN::kFormat.Matches<VertexPTN>());

  // Test that mismatched vertices don't match other formats.
  EXPECT_FALSE(TestVertex2f::kFormat.Matches<VertexP>());
  EXPECT_FALSE(VertexP::kFormat.Matches<TestVertex2f>());
  EXPECT_FALSE(VertexPC::kFormat.Matches<VertexP>());
  EXPECT_FALSE(VertexPN::kFormat.Matches<VertexPC>());
  EXPECT_FALSE(VertexPT::kFormat.Matches<VertexPN>());
  EXPECT_FALSE(VertexPTT::kFormat.Matches<VertexPT>());
  EXPECT_FALSE(VertexPTC::kFormat.Matches<VertexPTT>());
  EXPECT_FALSE(VertexPTI::kFormat.Matches<VertexPTC>());
  EXPECT_FALSE(VertexPTN::kFormat.Matches<VertexPTI>());
}

TEST(VertexFormat, EqualityOperator) {
  const VertexFormat empty({});
  EXPECT_FALSE(empty == TestVertex2f::kFormat);
  EXPECT_FALSE(empty == VertexP::kFormat);
  EXPECT_FALSE(empty == VertexPC::kFormat);
  EXPECT_FALSE(empty == VertexPN::kFormat);
  EXPECT_FALSE(empty == VertexPT::kFormat);
  EXPECT_FALSE(empty == VertexPTT::kFormat);
  EXPECT_FALSE(empty == VertexPTC::kFormat);
  EXPECT_FALSE(empty == VertexPTI::kFormat);
  EXPECT_FALSE(empty == VertexPTN::kFormat);

  // Make sure each format equals itself.
  EXPECT_TRUE(empty == empty);
  EXPECT_TRUE(TestVertex2f::kFormat == TestVertex2f::kFormat);
  EXPECT_TRUE(VertexP::kFormat == VertexP::kFormat);
  EXPECT_TRUE(VertexPC::kFormat == VertexPC::kFormat);
  EXPECT_TRUE(VertexPN::kFormat == VertexPN::kFormat);
  EXPECT_TRUE(VertexPT::kFormat == VertexPT::kFormat);
  EXPECT_TRUE(VertexPTT::kFormat == VertexPTT::kFormat);
  EXPECT_TRUE(VertexPTC::kFormat == VertexPTC::kFormat);
  EXPECT_TRUE(VertexPTI::kFormat == VertexPTI::kFormat);
  EXPECT_TRUE(VertexPTN::kFormat == VertexPTN::kFormat);

  // Construct a new vertex format that matches an existing one but in a
  // different declaration order, and test them for equality.
  const VertexFormat test_pt_format(
      {VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                       VertexAttribute::kFloat32),
       VertexAttribute(0, VertexAttribute::kPosition, 3,
                       VertexAttribute::kFloat32)});
  EXPECT_TRUE(test_pt_format == VertexPT::kFormat);
}

TEST(VertexFormatDeathTest, RangeChecks) {
  PORT_EXPECT_DEBUG_DEATH(
      VertexFormat({
          VertexAttribute(), VertexAttribute(), VertexAttribute(),
          VertexAttribute(), VertexAttribute(), VertexAttribute(),
          VertexAttribute(), VertexAttribute(), VertexAttribute(),
          VertexAttribute(), VertexAttribute(), VertexAttribute(),
          VertexAttribute(), VertexAttribute(), VertexAttribute(),
          VertexAttribute(), VertexAttribute(), VertexAttribute(),
          VertexAttribute(), VertexAttribute(),
      }),
      "Cannot exceed max attributes size");

  const VertexFormat empty({});
  PORT_EXPECT_DEATH(empty.GetAttributeAt(0), "Index out of bounds!");
}

}  // namespace
}  // namespace lull
