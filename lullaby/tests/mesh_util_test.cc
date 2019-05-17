/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/modules/render/mesh_util.h"
#include "gtest/gtest.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace {

constexpr float kEpsilon = 1.0E-5f;

using testing::NearMathfu;

TEST(TesselatedQuadDeathTest, SanityChecks) {
  constexpr const char* kNotEnoughVertsMessage = "Failed to reserve";

  // We need at least 2 verts in each dimension.
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 1, /* num_verts_y = */ 2, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 2, /* num_verts_y = */ 1, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 1,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 0, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 1,
          /* corner_radius = */ 0, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);

  // We need at least 4 verts in each dimension if we have rounded corners.
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 2, /* num_verts_y = */ 4, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 4, /* num_verts_y = */ 2, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 4,
          /* corner_radius = */ 1, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 4,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 1, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);

  // Check that we're not asking for negative corner vertices.
  constexpr const char* kNegativeCornerVertsMessage =
      "Must have >= 0 corner vertices.";
  PORT_EXPECT_DEBUG_DEATH(CalculateTesselatedQuadIndices(
                              /* num_verts_x = */ 2, /* num_verts_y = */ 2,
                              /* corner_verts = */ -2),
                          kNegativeCornerVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 1, /* corner_verts = */ -1),
      kNegativeCornerVertsMessage);

  // Check that negatively-sized quads are not allowed.
  constexpr const char* kNegativeSizeMessage =
      "Size of quad has to be >= than 0.0";
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ -1, /* size_y = */ -1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 0, /* corner_verts = */ 0),
      kNegativeSizeMessage);
}

TEST(TesselatedQuad, CheckVerticesNoCorners) {
  const float size_x = 2.f;
  const float size_y = 4.f;
  const int verts_x = 5;
  const int verts_y = 7;

  const auto vertices = CalculateTesselatedQuadVertices<VertexPTN>(
      size_x, size_y, verts_x, verts_y, 0.f, 0);

  EXPECT_EQ(static_cast<int>(vertices.size()), verts_x * verts_y);

  const int bottom_left_ind = 0;
  const int top_left_ind = verts_y - 1;
  const int top_right_ind = (verts_x)*verts_y - 1;
  const int bottom_right_ind = (verts_x - 1) * verts_y;

  {
    // Check positions.
    const mathfu::vec3 bottom_left = GetPosition(vertices[bottom_left_ind]);
    const mathfu::vec3 top_left = GetPosition(vertices[top_left_ind]);
    const mathfu::vec3 top_right = GetPosition(vertices[top_right_ind]);
    const mathfu::vec3 bottom_right = GetPosition(vertices[bottom_right_ind]);

    EXPECT_NEAR(bottom_left.x, -size_x / 2.f, kEpsilon);
    EXPECT_NEAR(top_left.x, -size_x / 2.f, kEpsilon);
    EXPECT_NEAR(top_right.x, size_x / 2.f, kEpsilon);
    EXPECT_NEAR(bottom_right.x, size_x / 2.f, kEpsilon);

    EXPECT_NEAR(bottom_left.y, -size_y / 2.f, kEpsilon);
    EXPECT_NEAR(top_left.y, size_y / 2.f, kEpsilon);
    EXPECT_NEAR(top_right.y, size_y / 2.f, kEpsilon);
    EXPECT_NEAR(bottom_right.y, -size_y / 2.f, kEpsilon);

    EXPECT_NEAR(bottom_left.z, 0, kEpsilon);
    EXPECT_NEAR(top_left.z, 0, kEpsilon);
    EXPECT_NEAR(top_right.z, 0, kEpsilon);
    EXPECT_NEAR(bottom_right.z, 0, kEpsilon);
  }
  {
    // Check normals.
    const mathfu::vec3 bottom_left = GetNormal(vertices[bottom_left_ind]);
    const mathfu::vec3 top_left = GetNormal(vertices[top_left_ind]);
    const mathfu::vec3 top_right = GetNormal(vertices[top_right_ind]);
    const mathfu::vec3 bottom_right = GetNormal(vertices[bottom_right_ind]);

    EXPECT_NEAR(bottom_left.x, 0.0f, kEpsilon);
    EXPECT_NEAR(top_left.x, 0.0f, kEpsilon);
    EXPECT_NEAR(top_right.x, 0.0f, kEpsilon);
    EXPECT_NEAR(bottom_right.x, 0.0f, kEpsilon);

    EXPECT_NEAR(bottom_left.y, 0.0f, kEpsilon);
    EXPECT_NEAR(top_left.y, 0.0f, kEpsilon);
    EXPECT_NEAR(top_right.y, 0.0f, kEpsilon);
    EXPECT_NEAR(bottom_right.y, 0.0f, kEpsilon);

    EXPECT_NEAR(bottom_left.z, 1.0f, kEpsilon);
    EXPECT_NEAR(top_left.z, 1.0f, kEpsilon);
    EXPECT_NEAR(top_right.z, 1.0f, kEpsilon);
    EXPECT_NEAR(bottom_right.z, 1.0f, kEpsilon);
  }
}

TEST(TesselatedQuad, CheckIndicesNoCorners) {
  const int verts_x = 5;
  const int verts_y = 7;
  const auto indices = CalculateTesselatedQuadIndices(verts_x, verts_y, 0);
  EXPECT_EQ(static_cast<int>(indices.size()),
            (verts_x - 1) * (verts_y - 1) * 6);
}

TEST(TesselatedQuad, CheckVerticesWithCorners) {
  const float size_x = 8.f;
  const float size_y = 4.f;
  const float half_size_x = size_x / 2.f;
  const float half_size_y = size_y / 2.f;
  const int verts_x = 8;
  const int verts_y = 4;
  const float corner_radius = 1.f;
  const int corner_verts = 1;
  const auto vertices = CalculateTesselatedQuadVertices<VertexPT>(
      size_x, size_y, verts_x, verts_y, corner_radius, corner_verts);

  const int vertex_count = ((verts_x * verts_y) - 4 + (corner_verts * 4));
  EXPECT_EQ(static_cast<int>(vertices.size()), vertex_count);
  // From radiused corner vertices we expect that:
  //  a) The minimum and maximum x value will be -/+ size_x / 2
  //  b) The minimum and maximum y value will be -/+ size_y / 2
  //  c) The minimum and maximum u value will be [0, 1], and at the appropriate
  //      position in extremes in x.
  //  d) The minimum and maximum v value will be [0, 1], and at the appropriate
  //      position in extremes in y.
  //  e) all z values are nonzero
  //  f) no vertex having a min or max in one dimension will have a min or
  //     max in the other dimension (meaning that the actual corners are not
  //     within the geometry)
  float min_x_value = size_x;
  float max_x_value = -size_x;
  float min_y_value = size_y;
  float max_y_value = -size_y;
  float min_u_value = 1.0f;
  float max_u_value = 0.0f;
  float min_v_value = 1.0f;
  float max_v_value = 0.0f;
  for (int i = 0; i < vertex_count; ++i) {
    const int ind = i;
    // Validate x value assumptions.
    min_x_value = std::min(min_x_value, vertices[ind].x);
    max_x_value = std::max(max_x_value, vertices[ind].x);
    if (std::abs(vertices[ind].x - half_size_x) < kEpsilon) {
      // The y value should not also be near its extreme.
      EXPECT_GT(std::abs(vertices[ind].y - half_size_y), kEpsilon);
      // We are at an extreme of x, make sure that the u value corresponds.
      if (vertices[ind].x < 0) {
        EXPECT_NEAR(vertices[ind].u0, 0.f, kEpsilon);
      } else {
        EXPECT_NEAR(vertices[ind].u0, 1.f, kEpsilon);
      }
    }

    // Validate y value assumptions.
    min_y_value = std::min(min_y_value, vertices[ind].y);
    max_y_value = std::max(max_y_value, vertices[ind].y);
    if (std::abs(vertices[ind].y - half_size_y) < kEpsilon) {
      // The x value should not also be near its extreme.
      EXPECT_GT(std::abs(vertices[ind].x - half_size_x), kEpsilon);
      // We are at the extreme of y, make sure that the v value corresponds.
      if (vertices[ind].y < 0) {
        EXPECT_NEAR(vertices[ind].v0, 1.f, kEpsilon);
      } else {
        EXPECT_NEAR(vertices[ind].v0, 0.f, kEpsilon);
      }
    }

    // The z value should always be very near zero.
    EXPECT_NEAR(vertices[ind].z, 0, kEpsilon);

    // Validate u value assumptions.
    min_u_value = std::min(min_u_value, vertices[ind].u0);
    max_u_value = std::max(max_u_value, vertices[ind].u0);

    // Validate v value assumptions.
    min_v_value = std::min(min_v_value, vertices[ind].v0);
    max_v_value = std::max(max_v_value, vertices[ind].v0);
  }

  // Check computed extrema for correctness.
  EXPECT_NEAR(-half_size_x, min_x_value, kEpsilon);
  EXPECT_NEAR(half_size_x, max_x_value, kEpsilon);
  EXPECT_NEAR(-half_size_y, min_y_value, kEpsilon);
  EXPECT_NEAR(half_size_y, max_y_value, kEpsilon);
  EXPECT_NEAR(0.f, min_u_value, kEpsilon);
  EXPECT_NEAR(1.f, max_u_value, kEpsilon);
  EXPECT_NEAR(0.f, min_v_value, kEpsilon);
  EXPECT_NEAR(1.f, max_v_value, kEpsilon);
}

TEST(TesselatedQuad, CheckIndicesWithCorners) {
  const int verts_x = 17;
  const int verts_y = 7;
  const int corner_verts = 11;
  const auto indices =
      CalculateTesselatedQuadIndices(verts_x, verts_y, corner_verts);
  EXPECT_EQ(
      static_cast<int>(indices.size()),
      ((verts_x - 1) * (verts_y - 1) * 6) - 24 + (12 * (corner_verts + 1)));
}

TEST(TesselatedQuad, CornerMask) {
  const int verts_x = 17;
  const int verts_y = 7;
  const int corner_verts = 11;
  const auto indices =
      CalculateTesselatedQuadIndices(verts_x, verts_y, corner_verts);
  const auto verts = CalculateTesselatedQuadVertices<VertexPT>(
      /* size_x = */ 1, /* size_y = */ 1, verts_x, verts_y,
      /* corner_radius = */ 1, corner_verts,
      /* corner_mask = */ CornerMask::kNone);

  // Make sure that all the indices are valid.
  for (const auto& index : indices) {
    EXPECT_GE(index, 0U);
    EXPECT_LT(index, verts.size());
  }
}

static const int kIndicesPerQuad = 6;
static const int kIndicesPerTriangle = 3;
static const int kCornersPerQuad = 4;

TEST(TesselatedQuad, VertexIndexCountsSquareCorners) {
  const int verts_x = 5;
  const int verts_y = 7;
  const size_t vertex_count = GetTesselatedQuadVertexCount(verts_x, verts_y, 0);
  EXPECT_EQ(static_cast<int>(vertex_count), verts_x * verts_y);
  const size_t index_count = GetTesselatedQuadIndexCount(verts_x, verts_y, 0);
  EXPECT_EQ(static_cast<int>(index_count),
            (verts_x - 1) * (verts_y - 1) * kIndicesPerQuad);
}

TEST(TesselatedQuad, VertexIndexCountsRoundCorners) {
  const int verts_x = 5;
  const int verts_y = 7;
  const int corner_verts = 5;
  const size_t vertex_count =
      GetTesselatedQuadVertexCount(verts_x, verts_y, corner_verts);
  EXPECT_EQ(static_cast<int>(vertex_count),
            ((verts_x * verts_y) - kCornersPerQuad +
             (corner_verts * kCornersPerQuad)));
  const size_t index_count =
      GetTesselatedQuadIndexCount(verts_x, verts_y, corner_verts);
  EXPECT_EQ(static_cast<int>(index_count),
            ((verts_x - 1) * (verts_y - 1) * kIndicesPerQuad) -
                kIndicesPerQuad * kCornersPerQuad +
                (kIndicesPerTriangle * kCornersPerQuad * (corner_verts + 1)));
}

TEST(TessellatedQuad, CreateQuadMesh) {
  constexpr float kSizeX = 2.0f;
  constexpr float kSizeY = 1.5f;
  constexpr float kCornerRadius = 0.2f;
  constexpr int kNumVertsX = 5;
  constexpr int kNumVertsY = 7;
  constexpr int kNumCornerVerts = 5;
  const std::vector<VertexPTN> vertices =
      CalculateTesselatedQuadVertices<VertexPTN>(kSizeX, kSizeY, kNumVertsX,
                                                 kNumVertsY, kCornerRadius,
                                                 kNumCornerVerts);
  const std::vector<uint16_t> indices =
      CalculateTesselatedQuadIndices(kNumVertsX, kNumVertsY, kNumCornerVerts);

  MeshData mesh = CreateQuadMesh<VertexPTN>(
      kSizeX, kSizeY, kNumVertsX, kNumVertsY, kCornerRadius, kNumCornerVerts);
  EXPECT_EQ(mesh.GetVertexFormat(), VertexPTN::kFormat);
  EXPECT_EQ(mesh.GetNumVertices(), vertices.size());
  EXPECT_EQ(mesh.GetNumIndices(), indices.size());

  const VertexPTN* vertex_data = mesh.GetMutableVertexData<VertexPTN>();
  ASSERT_TRUE(vertex_data != nullptr);
  for (size_t i = 0; i < vertices.size(); ++i) {
    EXPECT_EQ(vertex_data[i].x, vertices[i].x);
    EXPECT_EQ(vertex_data[i].y, vertices[i].y);
    EXPECT_EQ(vertex_data[i].z, vertices[i].z);
    EXPECT_EQ(vertex_data[i].nx, vertices[i].nx);
    EXPECT_EQ(vertex_data[i].ny, vertices[i].ny);
    EXPECT_EQ(vertex_data[i].nz, vertices[i].nz);
    EXPECT_EQ(vertex_data[i].u0, vertices[i].u0);
    EXPECT_EQ(vertex_data[i].v0, vertices[i].v0);
  }

  const uint16_t* index_data = mesh.GetIndexData<uint16_t>();
  ASSERT_TRUE(index_data != nullptr);
  for (size_t i = 0; i < indices.size(); ++i) {
    EXPECT_EQ(index_data[i], indices[i]);
  }
}

TEST(ApplyDeformation, IsAppliedAsExpectedToMesh) {
  VertexPT vertices[] = {
      VertexPT(1.0f, 2.0f, 3.0f, 0.1f, 0.2f),
      VertexPT(4.0f, 5.0f, 6.0f, 0.3f, 0.4f),
      VertexPT(7.0f, 8.0f, 9.0f, 0.5f, 0.6f),
  };
  DataContainer vertex_data(
      DataContainer::DataPtr(reinterpret_cast<uint8_t*>(vertices),
                             [](const uint8_t*) {}),
      sizeof(vertices), sizeof(vertices), DataContainer::kAll);

  MeshData mesh(MeshData::kPoints, VertexPT::kFormat, std::move(vertex_data));

  ApplyDeformation(&mesh, [](const mathfu::vec3& pos) { return -2.0f * pos; });
  EXPECT_THAT(GetPosition(vertices[0]),
              NearMathfu(mathfu::vec3(-2.0f, -4.0f, -6.0f), kEpsilon));
  EXPECT_THAT(GetPosition(vertices[1]),
              NearMathfu(mathfu::vec3(-8.0f, -10.0f, -12.0f), kEpsilon));
  EXPECT_THAT(GetPosition(vertices[2]),
              NearMathfu(mathfu::vec3(-14.0f, -16.0f, -18.0f), kEpsilon));

  EXPECT_EQ(vertices[0].u0, 0.1f);
  EXPECT_EQ(vertices[0].v0, 0.2f);
  EXPECT_EQ(vertices[1].u0, 0.3f);
  EXPECT_EQ(vertices[1].v0, 0.4f);
  EXPECT_EQ(vertices[2].u0, 0.5f);
  EXPECT_EQ(vertices[2].v0, 0.6f);
}

TEST(ApplyDeformationDeathTest, FailsWithInsufficientAccess) {
  auto deform = [](const mathfu::vec3& pos) {
    LOG(FATAL);
    return pos;
  };

  uint8_t data_buf[8] = {0};
  DataContainer unreadable_data(
      DataContainer::DataPtr(data_buf, [](const uint8_t*) {}), sizeof(data_buf),
      sizeof(data_buf), DataContainer::kWrite);
  MeshData unreadable_mesh(MeshData::kPoints, VertexP::kFormat,
                           std::move(unreadable_data));
  PORT_EXPECT_DEBUG_DEATH(ApplyDeformation(&unreadable_mesh, deform), "");

  DataContainer unwriteable_data(
      DataContainer::DataPtr(data_buf, [](const uint8_t*) {}), sizeof(data_buf),
      sizeof(data_buf), DataContainer::kRead);
  MeshData unwriteable_mesh(MeshData::kPoints, VertexP::kFormat,
                            std::move(unwriteable_data));
  PORT_EXPECT_DEBUG_DEATH(ApplyDeformation(&unwriteable_mesh, deform), "");
}

TEST(CreateLatLonSphereDeathTest, CatchesBadArguments) {
  const float radius = 1.0f;
  PORT_EXPECT_DEATH(CreateLatLonSphere(radius, /* num_parallels = */ 0,
                                       /* num_meridians = */ 3),
                    "");
  PORT_EXPECT_DEATH(CreateLatLonSphere(radius, /* num_parallels = */ 1,
                                       /* num_meridians = */ 2),
                    "");
}

TEST(CreateLatLonSphereTest, GeneratesCorrectNumbersOfVerticesAndIndices) {
  const float radius = 1.0f;
  MeshData mesh = CreateLatLonSphere(radius, /* num_parallels = */ 1,
                                     /* num_meridians = */ 3);
  EXPECT_EQ(mesh.GetPrimitiveType(), MeshData::kTriangles);
  EXPECT_EQ(mesh.GetNumVertices(), 6U);
  EXPECT_EQ(mesh.GetNumIndices(), 3U * 6U);

  mesh = CreateLatLonSphere(radius, /* num_parallels = */ 1,
                            /* num_meridians = */ 7);
  EXPECT_EQ(mesh.GetPrimitiveType(), MeshData::kTriangles);
  EXPECT_EQ(mesh.GetNumVertices(), 10U);
  EXPECT_EQ(mesh.GetNumIndices(), 3U * 14U);

  mesh = CreateLatLonSphere(radius, /* num_parallels = */ 5,
                            /* num_meridians = */ 3);
  EXPECT_EQ(mesh.GetPrimitiveType(), MeshData::kTriangles);
  EXPECT_EQ(mesh.GetNumVertices(), 22U);
  EXPECT_EQ(mesh.GetNumIndices(), 3U * (6U + 24U));
}

TEST(CreateLatLonSphereTest, GeneratesPositionsThatHaveRadiusLength) {
  float radius = 2.5f;
  MeshData mesh = CreateLatLonSphere(radius, /* num_parallels = */ 3,
                                     /* num_meridians = */ 5);
  for (size_t i = 0; i < mesh.GetNumVertices(); ++i) {
    const VertexPTN& v = mesh.GetVertexData<VertexPTN>()[i];
    EXPECT_NEAR(GetPosition(v).Length(), radius, kDefaultEpsilon);
  }

  radius = 8.3f;
  mesh = CreateLatLonSphere(radius, /* num_parallels = */ 4,
                            /* num_meridians = */ 4);
  for (size_t i = 0; i < mesh.GetNumVertices(); ++i) {
    const VertexPTN& v = mesh.GetVertexData<VertexPTN>()[i];
    EXPECT_NEAR(GetPosition(v).Length(), radius, kDefaultEpsilon);
  }
}

TEST(CreateLatLonSphereTest,
     GeneratesExternallyFacingTrianglesWhenGivenAPositiveRadius) {
  MeshData mesh =
      CreateLatLonSphere(/* radius = */ 1.0f, /* num_parallels = */ 1,
                         /* num_meridians = */ 3);
  const auto* indices = mesh.GetIndexData<uint32_t>();
  EXPECT_EQ(mesh.GetPrimitiveType(), MeshData::kTriangles);
  for (size_t i = 0; i < mesh.GetNumIndices(); i += 3) {
    const VertexPTN* vertices = mesh.GetVertexData<VertexPTN>();
    const mathfu::vec3 p0 = GetPosition(vertices[indices[i + 0]]);
    const mathfu::vec3 p1 = GetPosition(vertices[indices[i + 1]]);
    const mathfu::vec3 p2 = GetPosition(vertices[indices[i + 2]]);
    const mathfu::vec3 d1 = p1 - p0;
    const mathfu::vec3 d2 = p2 - p0;
    EXPECT_THAT(d1, Not(NearMathfu(d2, kEpsilon)));
    const mathfu::vec3 normal = mathfu::vec3::CrossProduct(d1, d2).Normalized();
    EXPECT_GT(mathfu::vec3::DotProduct(p0, normal), 0.0f);
    EXPECT_GT(mathfu::vec3::DotProduct(p1, normal), 0.0f);
    EXPECT_GT(mathfu::vec3::DotProduct(p2, normal), 0.0f);
  }
}

TEST(CreateLatLonSphereTest,
     GeneratesInternallyFacingTrianglesWhenGivenANegativeRadius) {
  MeshData mesh =
      CreateLatLonSphere(/* radius = */ -1.0f, /* num_parallels = */ 1,
                         /* num_meridians = */ 3);
  const auto* indices = mesh.GetIndexData<uint32_t>();
  EXPECT_EQ(mesh.GetPrimitiveType(), MeshData::kTriangles);
  for (size_t i = 0; i < mesh.GetNumIndices(); i += 3) {
    const VertexPTN* vertices = mesh.GetVertexData<VertexPTN>();
    const mathfu::vec3 p0 = GetPosition(vertices[indices[i + 0]]);
    const mathfu::vec3 p1 = GetPosition(vertices[indices[i + 1]]);
    const mathfu::vec3 p2 = GetPosition(vertices[indices[i + 2]]);
    const mathfu::vec3 d1 = p1 - p0;
    const mathfu::vec3 d2 = p2 - p0;
    EXPECT_THAT(d1, Not(NearMathfu(d2, kEpsilon)));
    const mathfu::vec3 normal = mathfu::vec3::CrossProduct(d1, d2).Normalized();
    EXPECT_LT(mathfu::vec3::DotProduct(p0, normal), 0.0f);
    EXPECT_LT(mathfu::vec3::DotProduct(p1, normal), 0.0f);
    EXPECT_LT(mathfu::vec3::DotProduct(p2, normal), 0.0f);
  }
}

TEST(CreateLatLonSphereTest, GeneratesUniqueVerticesExceptForWhenUWraps) {
  MeshData mesh =
      CreateLatLonSphere(/* radius = */ 2.5f, /* num_parallels = */ 3,
                         /* num_meridians = */ 5);
  float min_wrap_v = 1.0f;
  float max_wrap_v = 0.0f;
  for (size_t i = 0; i < mesh.GetNumVertices(); ++i) {
    const VertexPTN& v1 = mesh.GetVertexData<VertexPTN>()[i];

    for (size_t k = i + 1; k < mesh.GetNumVertices(); ++k) {
      const VertexPTN& v2 = mesh.GetVertexData<VertexPTN>()[k];
      const mathfu::vec3 pos_delta = GetPosition(v1) - GetPosition(v2);
      if (pos_delta.Length() < kDefaultEpsilon) {
        EXPECT_EQ(v1.v0, v2.v0);
        EXPECT_TRUE((v1.u0 == 0.0f && v2.u0 == 1.0f) ||
                    (v1.u0 == 1.0f && v2.u0 == 0.0f));
        max_wrap_v = std::max(v1.v0, max_wrap_v);
        min_wrap_v = std::min(v1.v0, min_wrap_v);
      } else {
        EXPECT_TRUE(v1.u0 != v2.u0 || v1.v0 != v2.v0);
      }
    }
  }

  EXPECT_LT(min_wrap_v, max_wrap_v);
}

static void TestThatMeshUvsFollowLatLon(const MeshData& mesh) {
  for (size_t i = 0; i < mesh.GetNumVertices(); ++i) {
    const VertexPTN& v = mesh.GetVertexData<VertexPTN>()[i];
    const mathfu::vec3 pos = GetPosition(v);
    const mathfu::vec2 uv = GetUv0(v);
    const float lat = std::acos(pos.y / pos.Length());
    const float expected_v = lat / kPi;
    EXPECT_NEAR(uv.y, expected_v, kEpsilon);

    // Pole U values are expected to be .5, otherwise they should follow
    // longitude except for the seam vertices which have u=1.0 at lon=0.
    if (pos.x == 0.0f && pos.z == 0.0f) {
      EXPECT_EQ(uv.x, .5f);
    } else if (uv.x == 1.0f) {
      EXPECT_EQ(pos.z, 0.0f);
    } else {
      float lon = std::atan2(pos.z, pos.x);
      if (lon < 0.0f) {
        lon += 2.0f * kPi;
      }
      const float expected_u = lon / (2.0f * kPi);
      EXPECT_NEAR(uv.x, expected_u, kEpsilon);
    }
  }
}

TEST(CreateLatLonSphereTest, GeneratesUvsAccordingToLatLonRegardlessOfFacing) {
  constexpr float radius = 2.5f;
  constexpr int num_parallels = 3;
  constexpr int num_meridians = 5;

  MeshData external = CreateLatLonSphere(radius, num_parallels, num_meridians);
  MeshData internal = CreateLatLonSphere(-radius, num_parallels, num_meridians);

  TestThatMeshUvsFollowLatLon(external);
  TestThatMeshUvsFollowLatLon(internal);
}

TEST(GetBoundingBoxDeathTest, CatchesBadArguments) {
  VertexP vertices[] = {
      VertexP(0, 0, 0),
      VertexP(1, 1, 1),
  };

  VertexFormat invalid_vertex_format(
      {{VertexAttributeUsage_Position, VertexAttributeType_Scalar1f}});
  MeshData invalid_format_mesh(
      MeshData::kTriangles, invalid_vertex_format,
      DataContainer(DataContainer::DataPtr(reinterpret_cast<uint8_t*>(vertices),
                                           [](const uint8_t*) {}),
                    sizeof(vertices), sizeof(vertices), DataContainer::kWrite));

  PORT_EXPECT_DEBUG_DEATH(GetBoundingBox(invalid_format_mesh),
                          "Vertex format doesn't have pos3f");

  MeshData write_only_mesh(
      MeshData::kTriangles, VertexP::kFormat,
      DataContainer(DataContainer::DataPtr(reinterpret_cast<uint8_t*>(vertices),
                                           [](const uint8_t*) {}),
                    sizeof(vertices), sizeof(vertices), DataContainer::kWrite));
  PORT_EXPECT_DEBUG_DEATH(GetBoundingBox(write_only_mesh),
                          "without read access");
}

TEST(GetBoundingBox, BoundsOfAnEmptyMeshIsEmpty) {
  MeshData empty_mesh(MeshData::kTriangles, VertexP::kFormat, DataContainer());
  const Aabb box = GetBoundingBox(empty_mesh);
  EXPECT_EQ(box.min, mathfu::kZeros3f);
  EXPECT_EQ(box.max, mathfu::kZeros3f);
}

TEST(GetBoundingBox, MatchesExpectedOutput) {
  const VertexPT vertices[] = {
      VertexPT(0, 0, 5, 100, 200), VertexPT(1, 2, 0, 300, 400),
      VertexPT(0, 8, 2, 500, 600), VertexPT(-4, 3, -1, -100, -200),
      VertexPT(2, -9, -13, -300, -400)};

  MeshData mesh(
      MeshData::kTriangles, VertexPT::kFormat,
      DataContainer::WrapDataAsReadOnly(
          reinterpret_cast<const uint8_t*>(vertices), sizeof(vertices)));

  const Aabb box = GetBoundingBox(mesh);

  EXPECT_NEAR(box.min.x, -4, kEpsilon);
  EXPECT_NEAR(box.min.y, -9, kEpsilon);
  EXPECT_NEAR(box.min.z, -13, kEpsilon);
  EXPECT_NEAR(box.max.x, 2, kEpsilon);
  EXPECT_NEAR(box.max.y, 8, kEpsilon);
  EXPECT_NEAR(box.max.z, 5, kEpsilon);
}

}  // namespace
}  // namespace lull
