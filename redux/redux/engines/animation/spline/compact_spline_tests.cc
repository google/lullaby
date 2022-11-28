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
#include "redux/engines/animation/spline/compact_spline.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;

static const Interval kAngleInterval(-kPi, kPi);
static const float kEpsilonDerivative = 0.01f;
static const float kEpsilonX = 0.0001f;
static const float kEpsilonY = 0.0001f;

// Use a ridiculous index that will never hit when doing a search.
// We use this to test the binary search algorithm, not the cache.
static const CompactSplineIndex kRidiculousSplineIndex = 10000;

static const UncompressedNode kUncompressedSpline[] = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.5f, 0.03f},
    {1.5f, 0.6f, 0.02f},
    {3.0f, 0.0f, -0.04f},
};

static const UncompressedNode kUniformSpline[] = {
    {0.0f, 0.0f, 0.0f},   {1.0f, 0.5f, 0.03f},   {2.0f, 0.6f, 0.02f},
    {3.0f, 0.0f, -0.04f}, {4.0f, 0.03f, -0.02f}, {5.0f, 0.9f, -0.1f},
};

inline constexpr auto kNumUncompressedNodes =
    ABSL_ARRAYSIZE(kUncompressedSpline);

inline constexpr auto kNumUniformSplineNodes = ABSL_ARRAYSIZE(kUniformSpline);

class CompactSplineTests : public ::testing::Test {
 protected:
  void SetUp() override {
    spline_.Init(Interval(0.0f, 1.0f), 0.01f);
    spline_.AddNode(0.0f, 0.1f, 0.0f, kAddWithoutModification);
    spline_.AddNode(1.0f, 0.4f, 0.0f, kAddWithoutModification);
    spline_.AddNode(4.0f, 0.2f, 0.0f, kAddWithoutModification);
    spline_.AddNode(40.0f, 0.2f, 0.0f, kAddWithoutModification);
    spline_.AddNode(100.0f, 1.0f, 0.0f, kAddWithoutModification);
  }
  CompactSpline spline_;
};

// Test in-place creation and destruction.
TEST_F(CompactSplineTests, InPlaceCreation) {
  // Create a buffer with a constant fill.
  static const uint8_t kTestFill = 0xAB;
  uint8_t buffer[1024];
  memset(buffer, kTestFill, sizeof(buffer));

  // Dynamically create a spline in the buffer.
  static const int kTestMaxNodes = 3;
  static const size_t kSplineSize = CompactSpline::Size(kTestMaxNodes);
  assert(kSplineSize < sizeof(buffer));  // Strictly less to test for overflow.
  CompactSpline* spline = CompactSpline::CreateInPlace(kTestMaxNodes, buffer);

  EXPECT_THAT(spline->num_nodes(), Eq(0));
  EXPECT_THAT(spline->max_nodes(), Eq(kTestMaxNodes));

  // Create spline and ensure it now has the max size.
  spline->Init(kAngleInterval, 1.0f);
  for (int i = 0; i < kTestMaxNodes; ++i) {
    spline->AddNode(static_cast<float>(i), 0.0f, 0.0f, kAddWithoutModification);
  }

  EXPECT_THAT(spline->num_nodes(), Eq(kTestMaxNodes));
  EXPECT_THAT(spline->max_nodes(), Eq(kTestMaxNodes));

  // Ensure the spline hasn't overflowed its buffer.
  for (size_t j = kSplineSize; j < sizeof(buffer); ++j) {
    EXPECT_THAT(buffer[j], Eq(kTestFill));
  }

  // Test node destruction.
  spline->~CompactSpline();
}

// Ensure the index lookup is accurate for x's before the range.
TEST_F(CompactSplineTests, IndexForXBefore) {
  EXPECT_THAT(spline_.IndexForX(-1.0f, kRidiculousSplineIndex),
              Eq(kBeforeSplineIndex));
}

// Ensure the index lookup is accurate for x's barely before the range.
TEST_F(CompactSplineTests, IndexForXJustBefore) {
  EXPECT_THAT(spline_.IndexForX(-0.0001f, kRidiculousSplineIndex), Eq(0));
}

// Ensure the index lookup is accurate for x's barely before the range.
TEST_F(CompactSplineTests, IndexForXBiggerThanGranularityAtStart) {
  EXPECT_THAT(spline_.IndexForX(-0.011f, kRidiculousSplineIndex), Eq(0));
}

// Ensure the index lookup is accurate for x's after the range.
TEST_F(CompactSplineTests, IndexForXAfter) {
  EXPECT_THAT(spline_.IndexForX(101.0f, kRidiculousSplineIndex),
              Eq(kAfterSplineIndex));
}

// Ensure the index lookup is accurate for x's barely after the range.
TEST_F(CompactSplineTests, IndexForXJustAfter) {
  EXPECT_THAT(spline_.IndexForX(100.0001f, kRidiculousSplineIndex),
              Eq(spline_.LastSegmentIndex()));
}

// Ensure the index lookup is accurate for x right at start.
TEST_F(CompactSplineTests, IndexForXStart) {
  EXPECT_THAT(spline_.IndexForX(0.0f, kRidiculousSplineIndex), Eq(0));
}

// Ensure the index lookup is accurate for x right at end.
TEST_F(CompactSplineTests, IndexForXEnd) {
  EXPECT_THAT(spline_.IndexForX(100.0f, kRidiculousSplineIndex),
              Eq(spline_.LastSegmentIndex()));
}

// Ensure the index lookup is accurate for x just inside end.
TEST_F(CompactSplineTests, IndexForXAlmostEnd) {
  EXPECT_THAT(spline_.IndexForX(99.9999f, kRidiculousSplineIndex),
              Eq(spline_.LastSegmentIndex()));
}

// Ensure the index lookup is accurate for x just inside end.
TEST_F(CompactSplineTests, IndexForXBiggerThanGranularityAtEnd) {
  EXPECT_THAT(spline_.IndexForX(99.99f, kRidiculousSplineIndex), Eq(3));
}

// Ensure the index lookup is accurate for x in middle, right on the node.
TEST_F(CompactSplineTests, IndexForXMidOnNode) {
  EXPECT_THAT(spline_.IndexForX(1.0f, kRidiculousSplineIndex), Eq(1));
}

// Ensure the index lookup is accurate for x in middle, in middle of segment.
TEST_F(CompactSplineTests, IndexForXMidAfterNode) {
  EXPECT_THAT(spline_.IndexForX(1.1f, kRidiculousSplineIndex), Eq(1));
}

// Ensure the index lookup is accurate for x in middle, in middle of segment.
TEST_F(CompactSplineTests, IndexForXMidSecondLast) {
  EXPECT_THAT(spline_.IndexForX(4.1f, kRidiculousSplineIndex), Eq(2));
}

// YCalculatedSlowly should return the key-point Y values at key-point X values.
TEST_F(CompactSplineTests, YSlowAtNodes) {
  for (CompactSplineIndex i = 0; i < spline_.num_nodes(); ++i) {
    const float y = spline_.YCalculatedSlowly(spline_.NodeX(i));
    EXPECT_THAT(spline_.NodeY(i), FloatNear(y, kEpsilonY));
  }
}

// BulkYs should return the proper start and end values.
TEST_F(CompactSplineTests, BulkYsStartAndEnd) {
  // Get bulk data at several delta_xs, but always starting at the start of the
  // spline and ending at the end of the spline.
  // Then compare returned `ys` with start end end values of spline.
  static const int kMaxBulkYs = 5;
  for (size_t num = 2; num < kMaxBulkYs; ++num) {
    const float delta_x = spline_.EndX() / (num - 1);

    std::vector<float> ys(num);
    std::vector<float> ds(num);
    CompactSpline::BulkYs(&spline_, 1, 0.0f, delta_x, num, ys.data(),
                          ds.data());

    EXPECT_THAT(ys.front(), FloatNear(spline_.StartY(), kEpsilonY));
    EXPECT_THAT(ys.back(), FloatNear(spline_.EndY(), kEpsilonY));
    EXPECT_THAT(ds.front(),
                FloatNear(spline_.StartDerivative(), kEpsilonDerivative));
    EXPECT_THAT(ds.back(),
                FloatNear(spline_.EndDerivative(), kEpsilonDerivative));
  }
}

// BulkYs should return the proper start and end values.
TEST_F(CompactSplineTests, BulkYsVsSlowYs) {
  // Get bulk data at several delta_xs, but always starting at 3 delta_x
  // prior to start of the spline and ending at 3 delta_x after the end
  // of the spline.
  // Then compare returned `ys` with start end end values of spline.
  static const int kMaxBulkYs = 21;
  static const int kExtraSamples = 6;
  for (size_t num = 2; num < kMaxBulkYs - kExtraSamples; ++num) {
    // Collect `num_ys` evenly-spaced samples from spline_.
    const float delta_x = spline_.EndX() / (num - 1);
    const float start_x = 0.0f - (kExtraSamples / 2) * delta_x;
    const size_t num_points = num + kExtraSamples;

    std::vector<float> ys(num_points);
    std::vector<float> ds(num_points);
    CompactSpline::BulkYs(&spline_, 1, start_x, delta_x, num_points, ys.data(),
                          ds.data());

    // Compare bulk samples to slowly calcuated samples.
    float x = start_x;
    for (size_t j = 0; j < ys.size(); ++j) {
      const float expect_y = spline_.YCalculatedSlowly(x);
      EXPECT_THAT(ys[j], FloatNear(expect_y, kEpsilonY));
      x += delta_x;
    }
  }
}

// BulkYs should return the proper start and end values.
TEST_F(CompactSplineTests, BulkYsVec3) {
  static const int kDimensions = 3;
  static const int kNumYs = 16;

  // Make three copies of the spline data.
  CompactSpline splines[kDimensions];
  for (size_t d = 0; d < kDimensions; ++d) {
    splines[d] = spline_;
  }

  // Collect `num_ys` evenly-spaced samples from spline_.
  using float3 = Vector<float, 3, false>;
  float3 ys[kNumYs];
  memset(ys, 0xFF, sizeof(ys));
  const float delta_x = spline_.EndX() / (kNumYs - 1);
  CompactSpline::BulkYs<3>(splines, 0.0f, delta_x, kNumYs, ys);

  // Ensure all the values are being calculated.
  for (int j = 0; j < kNumYs; ++j) {
    const vec3 y(ys[j]);
    EXPECT_THAT(y.x, Eq(y.y));
    EXPECT_THAT(y.y, Eq(y.z));
  }
}

static void CheckUncompressedSplineNodes(const CompactSpline& spline,
                                         const UncompressedNode* nodes,
                                         size_t num_nodes) {
  for (size_t i = 0; i < num_nodes; ++i) {
    const UncompressedNode& n = nodes[i];
    const float x = spline.NodeX(static_cast<CompactSplineIndex>(i));
    const float y = spline.NodeY(static_cast<CompactSplineIndex>(i));
    const float d = spline.NodeDerivative(static_cast<CompactSplineIndex>(i));

    EXPECT_THAT(n.x, FloatNear(x, kEpsilonX));
    EXPECT_THAT(n.y, FloatNear(y, kEpsilonY));
    EXPECT_THAT(n.derivative, FloatNear(d, kEpsilonDerivative));
  }
}

// Uncompressed nodes should be evaluated pretty much unchanged.
TEST_F(CompactSplineTests, InitFromUncompressedNodes) {
  CompactSplinePtr spline = CompactSpline::CreateFromNodes(
      kUncompressedSpline, kNumUncompressedNodes);
  CheckUncompressedSplineNodes(*spline, kUncompressedSpline,
                               kNumUncompressedNodes);
}

// In-place construction from uncompressed nodes should be evaluated pretty
// much unchanged.
TEST_F(CompactSplineTests, InitFromUncompressedNodesInPlace) {
  uint8_t spline_buf[1024];
  assert(sizeof(spline_buf) >= CompactSpline::Size(kNumUncompressedNodes));
  CompactSpline* spline = CompactSpline::CreateFromNodesInPlace(
      kUncompressedSpline, kNumUncompressedNodes, spline_buf);
  CheckUncompressedSplineNodes(*spline, kUncompressedSpline,
                               kNumUncompressedNodes);
}

// CreateFromSpline of an already uniform spline should evaluate to the same
// spline.
TEST_F(CompactSplineTests, InitFromSpline) {
  CompactSplinePtr uniform_spline =
      CompactSpline::CreateFromNodes(kUniformSpline, kNumUniformSplineNodes);
  CompactSplinePtr spline =
      CompactSpline::CreateFromSpline(*uniform_spline, kNumUniformSplineNodes);
  CheckUncompressedSplineNodes(*spline, kUniformSpline, kNumUniformSplineNodes);
}

// CreateFromSpline of an already uniform spline should evaluate to the same
// spline. Test in-place construction.
TEST_F(CompactSplineTests, InitFromSplineInPlace) {
  uint8_t uniform_spline_buf[1024];
  uint8_t spline_buf[1024];
  assert(sizeof(spline_buf) >= CompactSpline::Size(kNumUniformSplineNodes) &&
         sizeof(uniform_spline_buf) >=
             CompactSpline::Size(kNumUniformSplineNodes));
  CompactSpline* uniform_spline = CompactSpline::CreateFromNodesInPlace(
      kUniformSpline, kNumUniformSplineNodes, uniform_spline_buf);
  CompactSpline* spline = CompactSpline::CreateFromSplineInPlace(
      *uniform_spline, kNumUniformSplineNodes, spline_buf);
  CheckUncompressedSplineNodes(*spline, kUniformSpline, kNumUniformSplineNodes);
}

}  // namespace
}  // namespace redux
