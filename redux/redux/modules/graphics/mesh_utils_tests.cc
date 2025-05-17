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

#include <cstddef>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/mesh_utils.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/graphics/vertex_utils.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/vector.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(MeshUtilsTests, ComputeBounds) {
  const vec3 points[] = {
    vec3(-1, -2, -3),
    vec3(1, 2, 3),
    vec3(-3, 3, 2),
    vec3(0, 0, -3),
    vec3(3, -3, 1),
  };
  const std::size_t num_points = sizeof(points) / sizeof(points[0]);

  VertexFormat format;
  format.AppendAttribute({VertexUsage::Position, VertexType::Vec3f}, 0,
                         sizeof(vec3));

  MeshData mesh;
  mesh.SetVertexData(format, DataContainer::WrapData(points, num_points),
                     num_points, Box::Empty());

  Box bounds = ComputeBounds(mesh);
  EXPECT_THAT(bounds.min, Eq(vec3(-3, -3, -3)));
  EXPECT_THAT(bounds.max, Eq(vec3(3, 3, 3)));
}

TEST(MeshUtilsTests, ComputeOrientationsNormalsAndTangents) {
  const vec4 data[] = {
    // Normals. Note, the w-component will be ignored.
    vec4(-1, -2, -3, 0).Normalized(),
    vec4(1, 2, 3, 0).Normalized(),
    vec4(-3, 3, 2, 0).Normalized(),
    vec4(0, 0, -3, 0).Normalized(),
    vec4(3, -3, 1, 0).Normalized(),
    // Tangents.
    vec4(0, -3, 2, 1).Normalized(),
    vec4(0, 3, -2, 1).Normalized(),
    vec4(0, 2, -3, 0).Normalized(),
    vec4(3, 0, 0, 0).Normalized(),
    vec4(0, 1, 3, 0).Normalized(),
  };
  const std::size_t num_normals = 5;
  const std::size_t num_tangents = 5;
  EXPECT_THAT(num_tangents, Eq(num_normals));

  VertexFormat format;
  format.AppendAttribute({VertexUsage::Normal, VertexType::Vec3f}, 0,
                         sizeof(vec4));
  format.AppendAttribute({VertexUsage::Tangent, VertexType::Vec4f},
                         sizeof(vec4) * num_normals, sizeof(vec4));

  MeshData mesh;
  mesh.SetVertexData(format,
                     DataContainer::WrapData(data, num_normals + num_tangents),
                     num_normals, Box::Empty());

  MeshData orientations = ComputeOrientations(mesh);
  EXPECT_THAT(orientations.GetNumVertices(), Eq(num_normals));

  const VertexFormat& out_format = orientations.GetVertexFormat();
  EXPECT_THAT(out_format.GetNumAttributes(), Eq(1));

  const VertexAttribute* attrib = out_format.GetAttributeAt(0);
  EXPECT_THAT(attrib->usage, Eq(VertexUsage::Orientation));
  EXPECT_THAT(attrib->type, Eq(VertexType::Vec4f));

  const std::size_t offset = out_format.GetOffsetOfAttributeAt(0);
  const std::size_t stride = out_format.GetStrideOfAttributeAt(0);
  EXPECT_THAT(offset, Eq(0));
  EXPECT_THAT(stride, Eq(sizeof(float) * 4));

  auto ptr = orientations.GetVertexData();
  for (std::size_t i = 0; i < num_normals; ++i) {
    const vec4& orientation = *reinterpret_cast<const vec4*>(&ptr[i * stride]);
    const vec4 expected =
        CalculateOrientation(data[i].xyz(), data[i + num_normals]);
    EXPECT_THAT(orientation, Eq(expected));
  }
}

TEST(MeshUtilsTests, ComputeOrientationsNormalsOnly) {
  const vec3 normals[] = {
    vec3(-1, -2, -3).Normalized(),
    vec3(1, 2, 3).Normalized(),
    vec3(-3, 3, 2).Normalized(),
    vec3(0, 0, -3).Normalized(),
    vec3(3, -3, 1).Normalized(),
  };
  const std::size_t num_normals = sizeof(normals) / sizeof(normals[0]);

  VertexFormat format;
  format.AppendAttribute({VertexUsage::Normal, VertexType::Vec3f}, 0,
                         sizeof(vec3));

  MeshData mesh;
  mesh.SetVertexData(format, DataContainer::WrapData(normals, num_normals),
                     num_normals, Box::Empty());

  MeshData orientations = ComputeOrientations(mesh);
  EXPECT_THAT(orientations.GetNumVertices(), Eq(num_normals));

  const VertexFormat& out_format = orientations.GetVertexFormat();
  EXPECT_THAT(out_format.GetNumAttributes(), Eq(1));

  const VertexAttribute* attrib = out_format.GetAttributeAt(0);
  EXPECT_THAT(attrib->usage, Eq(VertexUsage::Orientation));
  EXPECT_THAT(attrib->type, Eq(VertexType::Vec4f));

  const std::size_t offset = out_format.GetOffsetOfAttributeAt(0);
  const std::size_t stride = out_format.GetStrideOfAttributeAt(0);
  EXPECT_THAT(offset, Eq(0));
  EXPECT_THAT(stride, Eq(sizeof(float) * 4));

  auto ptr = orientations.GetVertexData();
  for (std::size_t i = 0; i < num_normals; ++i) {
    const vec4& orientation = *reinterpret_cast<const vec4*>(&ptr[i * stride]);
    const vec4 expected = CalculateOrientation(normals[i]);
    EXPECT_THAT(orientation, Eq(expected));
  }
}

}  // namespace
}  // namespace redux
