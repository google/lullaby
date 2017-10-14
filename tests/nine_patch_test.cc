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

#include <vector>

#include "gtest/gtest.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/nine_patch.h"
#include "lullaby/modules/render/vertex.h"
#include "tests/mathfu_matchers.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;

constexpr float kEpsilon = 1.0E-5f;

void AssertCorrectTranslations(
    const NinePatch& nine_patch,
    const std::vector<lull::VertexPTT>& nine_patch_vertices,
    const float* expected_positions, const float* expected_uvs,
    size_t count = 0, size_t count_offset = 0, size_t stride = 1,
    float x_offset = 0, float y_offset = 0, float z_offset = 0) {
  if (count == 0) {
    count = nine_patch_vertices.size();
  }
  for (size_t vertex_index = 0; vertex_index < count; ++vertex_index) {
    const lull::VertexPTT& v =
        nine_patch_vertices[vertex_index * stride + count_offset];
    EXPECT_NEAR(v.x, expected_positions[vertex_index * 3 + 0] + x_offset,
                kEpsilon);
    EXPECT_NEAR(v.y, expected_positions[vertex_index * 3 + 1] + y_offset,
                kEpsilon);
    EXPECT_NEAR(v.z, expected_positions[vertex_index * 3 + 2] + z_offset,
                kEpsilon);
    EXPECT_NEAR(v.u0, expected_uvs[vertex_index * 2 + 0], kEpsilon);
    EXPECT_NEAR(v.v0, expected_uvs[vertex_index * 2 + 1], kEpsilon);
    EXPECT_NEAR(v.u1, v.x / nine_patch.size.x + .5f, kEpsilon);
    EXPECT_NEAR(v.v1, .5f - v.y / nine_patch.size.y, kEpsilon);
  }
}

// Creates a mesh that wraps around the nine patch's vertex and index data
// vectors.
MeshData BuildMeshFromNinePatchVerticesAndIndices(
    std::vector<VertexPTT>* nine_patch_vertices,
    std::vector<MeshData::Index>* nine_patch_indices) {
  DataContainer::DataPtr vertex_data_ptr(
      reinterpret_cast<uint8_t*>(nine_patch_vertices->data()),
      // Data is managed by the vector.
      [](const uint8_t* ptr) {});
  DataContainer vertex_data(std::move(vertex_data_ptr),
                            nine_patch_vertices->size() * sizeof(VertexPTT),
                            DataContainer::AccessFlags::kAll);

  DataContainer::DataPtr index_data_ptr(
      reinterpret_cast<uint8_t*>(nine_patch_indices->data()),
      // Data is managed by the vector.
      [](const uint8_t* ptr) {});
  DataContainer index_data(std::move(index_data_ptr),
                           nine_patch_indices->size() * sizeof(MeshData::Index),
                           DataContainer::AccessFlags::kAll);
  return MeshData(lull::MeshData::PrimitiveType::kTriangles,
                  lull::VertexPTT::kFormat, std::move(vertex_data),
                  std::move(index_data));
}

TEST(NinePatch, CheckCounts) {
  NinePatch nine_patch;

  // Default subdivision counts are (1, 1)
  EXPECT_EQ(nine_patch.subdivisions.x, 1);
  EXPECT_EQ(nine_patch.subdivisions.y, 1);

  // two rows and columns for the slices plus an extra row and column of
  // vertices to complete the mesh means 1 + 2 + 1 on each side, and 4 * 4 = 16.
  EXPECT_EQ(nine_patch.GetVertexCount(), 16);
  // three by three quads, 2 triangles per quad, 3 indices per triangle,
  // 9 * 2 * 3 = 54.
  EXPECT_EQ(nine_patch.GetIndexCount(), 54);

  // Now we test these counts with some subdivision.
  nine_patch.subdivisions = mathfu::vec2i(5, 7);

  EXPECT_EQ(nine_patch.GetVertexCount(),
            (nine_patch.subdivisions.x + 2 + 1) *
                (nine_patch.subdivisions.y + 2 + 1));
  EXPECT_EQ(nine_patch.GetIndexCount(), (nine_patch.subdivisions.x + 2) *
                                            (nine_patch.subdivisions.y + 2) *
                                            2 * 3);
}

TEST(NinePatch, CheckUnstretchedVertices) {
  NinePatch nine_patch;

  nine_patch.size = mathfu::vec2(1, 1);
  nine_patch.original_size = mathfu::vec2(1, 1);
  nine_patch.left_slice = .25f;
  nine_patch.right_slice = .25f;
  nine_patch.bottom_slice = .25f;
  nine_patch.top_slice = .25f;

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);

  GenerateNinePatchMesh(nine_patch, &mesh);

  float expected_positions[] = {
      -.5f, -.5f,  0, -.25f, -.5f,  0, .25f, -.5f,  0, .5f, -.5f,  0,
      -.5f, -.25f, 0, -.25f, -.25f, 0, .25f, -.25f, 0, .5f, -.25f, 0,
      -.5f, .25f,  0, -.25f, .25f,  0, .25f, .25f,  0, .5f, .25f,  0,
      -.5f, .5f,   0, -.25f, .5f,   0, .25f, .5f,   0, .5f, .5f,   0};

  float expected_uvs[] = {0, 1,    .25f, 1,    .75f, 1,    1, 1,
                          0, .75f, .25f, .75f, .75f, .75f, 1, .75f,
                          0, .25f, .25f, .25f, .75f, .25f, 1, .25f,
                          0, 0,    .25f, 0,    .75f, 0,    1, 0};

  AssertCorrectTranslations(nine_patch, nine_patch_vertices, expected_positions,
                            expected_uvs);
}

TEST(NinePatch, CheckStretchedVertices) {
  NinePatch nine_patch;

  nine_patch.size = mathfu::vec2(2, 2);
  nine_patch.original_size = mathfu::vec2(1, 1);
  nine_patch.left_slice = .25f;
  nine_patch.right_slice = .25f;
  nine_patch.bottom_slice = .25f;
  nine_patch.top_slice = .25f;

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);
  GenerateNinePatchMesh(nine_patch, &mesh);

  float expected_positions[] = {
      -1.f, -1.f,  0, -.75f, -1.f,  0, .75f, -1.f,  0, 1.f, -1.f,  0,
      -1.f, -.75f, 0, -.75f, -.75f, 0, .75f, -.75f, 0, 1.f, -.75f, 0,
      -1.f, .75f,  0, -.75f, .75f,  0, .75f, .75f,  0, 1.f, .75f,  0,
      -1.f, 1.f,   0, -.75f, 1.f,   0, .75f, 1.f,   0, 1.f, 1.f,   0};

  float expected_uvs[] = {0, 1,    .25f, 1,    .75f, 1,    1, 1,
                          0, .75f, .25f, .75f, .75f, .75f, 1, .75f,
                          0, .25f, .25f, .25f, .75f, .25f, 1, .25f,
                          0, 0,    .25f, 0,    .75f, 0,    1, 0};

  AssertCorrectTranslations(nine_patch, nine_patch_vertices, expected_positions,
                            expected_uvs);
}

TEST(NinePatch, CheckMinifiedVertices) {
  NinePatch nine_patch;

  nine_patch.size = mathfu::vec2(.5f, .5f);
  nine_patch.original_size = mathfu::vec2(1, 1);
  nine_patch.left_slice = .25f;
  nine_patch.right_slice = .25f;
  nine_patch.bottom_slice = .25f;
  nine_patch.top_slice = .25f;

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);
  GenerateNinePatchMesh(nine_patch, &mesh);

  float expected_positions[] = {
      -.25f, -.25f, 0, 0, -.25f, 0, 0, -.25f, 0, .25f, -.25f, 0,
      -.25f, 0,     0, 0, 0,     0, 0, 0,     0, .25f, 0,     0,
      -.25f, 0,     0, 0, 0,     0, 0, 0,     0, .25f, 0,     0,
      -.25f, .25f,  0, 0, .25f,  0, 0, .25f,  0, .25f, .25f,  0};

  float expected_uvs[] = {0, 1,    .25f, 1,    .75f, 1,    1, 1,
                          0, .75f, .25f, .75f, .75f, .75f, 1, .75f,
                          0, .25f, .25f, .25f, .75f, .25f, 1, .25f,
                          0, 0,    .25f, 0,    .75f, 0,    1, 0};

  AssertCorrectTranslations(nine_patch, nine_patch_vertices, expected_positions,
                            expected_uvs);
}

float ComputeMiddlePatchU(const NinePatch& nine_patch, float x) {
  const float middle_patch_uv_width =
      1.0f - nine_patch.left_slice - nine_patch.right_slice;
  const float left_patch_width =
      nine_patch.left_slice * nine_patch.original_size.x;
  const float right_patch_width =
      nine_patch.right_slice * nine_patch.original_size.x;
  const float right_slice_position = nine_patch.size.x - right_patch_width;
  const float middle_patch_width = right_slice_position - left_patch_width;
  const float distance_in_middle_patch = x - left_patch_width;
  return nine_patch.left_slice +
         middle_patch_uv_width * distance_in_middle_patch / middle_patch_width;
}

// This test subdivides the nine patch and positions the slices such that the
// extra subdivisions are distributed in (and interpolated across) the middle
// horizontally stretched region of the nine patch.
TEST(NinePatch, CheckMiddlePatchSubdivision) {
  NinePatch nine_patch;

  nine_patch.size = mathfu::vec2(2, 2);
  nine_patch.original_size = mathfu::vec2(1, 1);
  nine_patch.left_slice = .2f;
  nine_patch.right_slice = .2f;
  nine_patch.bottom_slice = .2f;
  nine_patch.top_slice = .2f;
  nine_patch.subdivisions = mathfu::vec2i(3, 3);

  // Ascii art of a row cutting through the middle of this nine-patch in 1-D:
  // -  | means vertex due to subdivision.
  // -  { or } means vertex due to 9-patch slices.
  // -  total width is 2, with 3 evenly spaced subdivisions and slices .2 from
  //    the sides.
  // |--{-----|--------|-----}--|
  // 0  .2    2/3     4/3   1.8 2  <-- x from 0, actual will be centered.
  // UVs in the interior are determined by the proportion of UV within
  // the middle patch.
  float middle_vertex_u[2];
  middle_vertex_u[0] = ComputeMiddlePatchU(nine_patch, 2.f / 3.f);
  middle_vertex_u[1] = ComputeMiddlePatchU(nine_patch, 4.f / 3.f);

  // 2 extra divisions for the slices plus an extra row and column of vertices
  // to make complete quads on the ends means 3 + 2 + 1 on each side.
  EXPECT_THAT(nine_patch.GetVertexCount(), Eq((3 + 2 + 1) * (3 + 2 + 1)));
  // five by five quads, 2 triangles per quad, 3 indices per triangle
  EXPECT_THAT(nine_patch.GetIndexCount(), Eq(5 * 5 * 2 * 3));

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);
  GenerateNinePatchMesh(nine_patch, &mesh);

  // Just check the top row.
  constexpr float expected_positions[] = {
      -1.f,    -1.f, 0, -.8f, -1.f, 0, -.33333f, -1.f, 0,
      .33333f, -1.f, 0, .8f,  -1.f, 0, 1.f,      -1.f, 0,
  };

  const float expected_uvs[] = {
      0, 1, .2f, 1, middle_vertex_u[0], 1, middle_vertex_u[1], 1, .8f, 1, 1, 1,
  };

  constexpr int kVerticesInRow = 6;

  AssertCorrectTranslations(nine_patch, nine_patch_vertices, expected_positions,
                            expected_uvs, kVerticesInRow);
}

float ComputeLeftPatchU(const NinePatch& nine_patch, float x) {
  const float left_slice_width =
      nine_patch.left_slice * nine_patch.original_size.x;
  return nine_patch.left_slice * x / left_slice_width;
}

float ComputeRightPatchU(const NinePatch& nine_patch, float x) {
  const float right_patch_width =
      nine_patch.right_slice * nine_patch.original_size.x;
  const float right_slice_position = nine_patch.size.x - right_patch_width;
  return 1.0f - nine_patch.right_slice +
         nine_patch.right_slice * (x - right_slice_position) /
             right_patch_width;
}

TEST(NinePatch, CheckEdgePatchSubdivision) {
  NinePatch nine_patch;

  // These test numbers may seem weird, but they generate a patch which has its
  // slices in the middle (.5) and only resizes a small amount, so that vertices
  // of subdivision will fall in the edge patches for this test.
  nine_patch.size = mathfu::vec2(7, 7);
  nine_patch.original_size = mathfu::vec2(6, 6);
  nine_patch.left_slice = .5f;
  nine_patch.right_slice = .5f;
  nine_patch.bottom_slice = .5f;
  nine_patch.top_slice = .5f;
  nine_patch.subdivisions = mathfu::vec2i(3, 3);

  // Ascii art of this nine-patch in 1-D:
  // -  | means vertex due to subdivision.
  // -  { or } means vertex due to 9-patch slices.
  // -  total width is 2, with 3 evenly spaced subdivisions and slices at 0.5.
  // |---------|--{----}--|---------|
  // 0       2.33 3    4  4.66      7  <-- x from 0
  // The above numbers are spatial positions of each vertex growing from 0.
  // Final vertices will be with the mesh centered rather than growing from 0.
  // UVs in the edges are determined by the proportion of edge UV a particular
  // vertex corresponds to in the edge patches.  For the 2nd to left vertex:
  const float first_u = ComputeLeftPatchU(nine_patch, 2.0f + 1.0f / 3.0f);
  // And for the 2nd to right vertex, the UV of the rest of the NinePatch has
  // to be added as well:
  const float second_u = ComputeRightPatchU(nine_patch, 4.0f + 2.0f / 3.0f);

  // 2 extra divisions for the slices plus an extra row and column of vertices
  // to make complete quads on the ends means 3 + 2 + 1 on each side.
  EXPECT_THAT(nine_patch.GetVertexCount(), Eq((3 + 2 + 1) * (3 + 2 + 1)));
  // five by five quads, 2 triangles per quad, 3 indices per triangle
  EXPECT_THAT(nine_patch.GetIndexCount(), Eq(5 * 5 * 2 * 3));

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);
  GenerateNinePatchMesh(nine_patch, &mesh);

  // Test verts will be centered during the checks below
  constexpr int kVerticesInRow = 6;
  size_t count_offset = 0;
  size_t stride = 1;
  const float x_offset = -3.5f;
  const float y_offset = -3.5f;

  // Check the first row.
  float positions_first_row[] = {0, 0, 0, 2.f + (1.f / 3.f), 0, 0, 3, 0, 0,
                                 4, 0, 0, 4.f + (2.f / 3.f), 0, 0, 7, 0, 0};

  float uvs_first_row[] = {
      0, 1, first_u, 1, .5f, 1, .5f, 1, second_u, 1, 1, 1,
  };

  AssertCorrectTranslations(nine_patch, nine_patch_vertices,
                            positions_first_row, uvs_first_row, kVerticesInRow,
                            count_offset, stride, x_offset, y_offset);

  // Check the last row.
  float positions_last_row[] = {0, 7, 0, 2.f + (1.f / 3.f), 7, 0, 3, 7, 0,
                                4, 7, 0, 4.f + (2.f / 3.f), 7, 0, 7, 7, 0};

  float uvs_last_row[] = {
      0, 0, first_u, 0, .5f, 0, .5f, 0, second_u, 0, 1, 0,
  };

  count_offset = kVerticesInRow * (kVerticesInRow - 1);
  AssertCorrectTranslations(nine_patch, nine_patch_vertices, positions_last_row,
                            uvs_last_row, kVerticesInRow, count_offset, stride,
                            x_offset, y_offset);

  // Check the first column
  float positions_first_col[] = {0, 0, 0, 0, 2.f + (1.f / 3.f), 0, 0, 3, 0,
                                 0, 4, 0, 0, 4.f + (2.f / 3.f), 0, 0, 7, 0};

  float uvs_first_col[] = {
      0, 1, 0, second_u, 0, .5f, 0, .5f, 0, first_u, 0, 0,
  };

  count_offset = 0;
  stride = kVerticesInRow;
  AssertCorrectTranslations(nine_patch, nine_patch_vertices,
                            positions_first_col, uvs_first_col, kVerticesInRow,
                            count_offset, stride, x_offset, y_offset);

  // Check the last column
  float positions_last_col[] = {7, 0, 0, 7, 2.f + (1.f / 3.f), 0, 7, 3, 0,
                                7, 4, 0, 7, 4.f + (2.f / 3.f), 0, 7, 7, 0};

  float uvs_last_col[] = {
      1, 1, 1, second_u, 1, .5f, 1, .5f, 1, first_u, 1, 0,
  };

  count_offset = kVerticesInRow - 1;
  stride = kVerticesInRow;
  AssertCorrectTranslations(nine_patch, nine_patch_vertices, positions_last_col,
                            uvs_last_col, kVerticesInRow, count_offset, stride,
                            x_offset, y_offset);
}

TEST(NinePatch, CheckThinSliceVertices) {
  NinePatch nine_patch;

  nine_patch.size = mathfu::vec2(.25f, .25f);
  nine_patch.original_size = mathfu::vec2(1, 1);
  nine_patch.left_slice = .25f;
  nine_patch.right_slice = .25f;
  nine_patch.bottom_slice = .25f;
  nine_patch.top_slice = .25f;

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);
  GenerateNinePatchMesh(nine_patch, &mesh);

  float expected_positions[] = {
      -.125f, -.125f, 0, 0, -.125f, 0, 0, -.125f, 0, .125f, -.125f, 0,
      -.125f, 0,      0, 0, 0,      0, 0, 0,      0, .125f, 0,      0,
      -.125f, 0,      0, 0, 0,      0, 0, 0,      0, .125f, 0,      0,
      -.125f, .125f,  0, 0, .125f,  0, 0, .125f,  0, .125f, .125f,  0};

  float expected_uvs[] = {0, 1,    .25f, 1,    .75f, 1,    1, 1,
                          0, .75f, .25f, .75f, .75f, .75f, 1, .75f,
                          0, .25f, .25f, .25f, .75f, .25f, 1, .25f,
                          0, 0,    .25f, 0,    .75f, 0,    1, 0};

  AssertCorrectTranslations(nine_patch, nine_patch_vertices, expected_positions,
                            expected_uvs);
}

TEST(NinePatch, CheckThinUnsymmetricalSliceVertices) {
  NinePatch nine_patch;

  nine_patch.size = mathfu::vec2(.5f, .5f);
  nine_patch.original_size = mathfu::vec2(10, 10);
  nine_patch.left_slice = .4f;
  nine_patch.right_slice = .1f;
  nine_patch.bottom_slice = .1f;
  nine_patch.top_slice = .4f;

  std::vector<lull::VertexPTT> nine_patch_vertices(nine_patch.GetVertexCount());
  std::vector<uint16_t> nine_patch_indices(nine_patch.GetIndexCount());

  MeshData mesh = BuildMeshFromNinePatchVerticesAndIndices(&nine_patch_vertices,
                                                           &nine_patch_indices);
  GenerateNinePatchMesh(nine_patch, &mesh);

  float expected_positions[] = {
      -.25f, -.25f, 0, .15f, -.25f, 0, .15f, -.25f, 0, .25f, -.25f, 0,
      -.25f, -.15f, 0, .15f, -.15f, 0, .15f, -.15f, 0, .25f, -.15f, 0,
      -.25f, -.15f, 0, .15f, -.15f, 0, .15f, -.15f, 0, .25f, -.15f, 0,
      -.25f, .25f,  0, .15f, .25f,  0, .15f, .25f,  0, .25f, .25f,  0};

  float expected_uvs[] = {0,   1,   .4f, 1, .9f, 1, 1,   1,   0,   .9f, .4f,
                          .9f, .9f, .9f, 1, .9f, 0, .4f, .4f, .4f, .9f, .4f,
                          1,   .4f, 0,   0, .4f, 0, .9f, 0,   1,   0};

  AssertCorrectTranslations(nine_patch, nine_patch_vertices, expected_positions,
                            expected_uvs);
}

}  // namespace
}  // namespace lull
