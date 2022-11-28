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

#ifndef REDUX_SYSTEMS_SHAPE_SPHERE_SHAPE_GENERATOR_H_
#define REDUX_SYSTEMS_SHAPE_SPHERE_SHAPE_GENERATOR_H_

#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/graphics/vertex.h"
#include "redux/systems/shape/shape_def_generated.h"

namespace redux {

inline size_t CalculateVertexCount(const SphereShapeDef& def) {
  const size_t num_vertices = def.num_parallels * (def.num_meridians + 1) + 2;
  return num_vertices;
}

inline size_t CalculateIndexCount(const SphereShapeDef& def) {
  static constexpr size_t kNumVerticesPerTriangle = 3;
  const size_t num_triangles =
      (2 * def.num_meridians) +
      (2 * def.num_meridians) * (def.num_parallels - 1);
  const size_t num_indices = num_triangles * kNumVerticesPerTriangle;
  return num_indices;
}

template <typename Vertex, typename Index>
void GenerateShape(absl::Span<Vertex> vertices, absl::Span<Index> indices,
                   const SphereShapeDef& def) {
  CHECK_GT(def.radius, 0.f);
  CHECK_GE(def.num_parallels, 1);
  CHECK_GE(def.num_meridians, 3);
  CHECK_EQ(vertices.size(), CalculateVertexCount(def));
  CHECK_EQ(indices.size(), CalculateIndexCount(def));

  if (vertices.size() > std::numeric_limits<Index>::max()) {
    LOG(FATAL) << "Exceeded vertex limit";
    return;
  }

  const float kLatAngleStep = kPi / static_cast<float>(def.num_parallels + 1);
  const float kLonAngleStep = kTwoPi / static_cast<float>(def.num_meridians);

  static constexpr Index kNorthPoleIndex = 0;
  static constexpr Index kSouthPoleIndex = 1;

  Vertex& north_pole = vertices[kNorthPoleIndex];
  north_pole.Position().Set(0.f, def.radius, 0.f);
  north_pole.Normal().Set(0.f, 1.f, 0.f);
  north_pole.TangentFromNormal({0.f, 1.f, 0.f});
  north_pole.OrientationFromNormal({0.f, 1.f, 0.f});
  north_pole.TexCoord0().Set(0.5f, 0.f);

  Vertex& south_pole = vertices[kSouthPoleIndex];
  south_pole.Position().Set(0.f, -def.radius, 0.f);
  south_pole.Normal().Set(0.f, -1.f, 0.f);
  south_pole.TangentFromNormal({0.f, -1.f, 0.f});
  south_pole.OrientationFromNormal({0.f, -1.f, 0.f});
  south_pole.TexCoord0().Set(0.5f, 1.f);

  size_t vertex_idx = kSouthPoleIndex + 1;
  std::vector<Index> row_indices(def.num_parallels);

  // Vertices by latitude.
  for (int lat = 0; lat < def.num_parallels; ++lat) {
    row_indices[lat] = vertex_idx;

    // +1 because we handle the north pole (which would be at a lat angle of
    // 0-degreed) explicitly.
    const float lat_angle = static_cast<float>(lat + 1) * kLatAngleStep;
    const float cos_lat_angle = std::cos(lat_angle);
    const float sin_lat_angle = std::sin(lat_angle);
    const float y = def.radius * cos_lat_angle;
    const float ny = cos_lat_angle;
    const float v =
        static_cast<float>(lat + 1) / static_cast<float>(def.num_parallels + 1);

    // We do one extra iteration (i.e.lon == num_meridians) in order to add an
    // extra set of duplicate vertices down the back seem to stitch the UVs
    // together correctly.
    for (int lon = 0; lon <= def.num_meridians; ++lon) {
      // In theory, def.num_meridians * kLonAngleStep should equal 0.0, but we
      // set it explicitly to avoid any floating point errors.
      const float lon_angle = lon < def.num_meridians
                                  ? static_cast<float>(lon) * kLonAngleStep
                                  : 0.f;

      const float cos_lon_angle = std::cos(lon_angle);
      const float sin_lon_angle = std::sin(lon_angle);

      const float x = def.radius * sin_lat_angle * cos_lon_angle;
      const float z = def.radius * sin_lat_angle * sin_lon_angle;
      const float nx = sin_lat_angle * cos_lon_angle;
      const float nz = sin_lat_angle * sin_lon_angle;
      const float u =
          static_cast<float>(lon) / static_cast<float>(def.num_meridians);

      Vertex& vertex = vertices[vertex_idx++];
      vertex.Position().Set(x, y, z);
      vertex.Normal().Set(nx, ny, nz);
      vertex.TangentFromNormal({nx, ny, nz});
      vertex.OrientationFromNormal({nx, ny, nz});
      vertex.TexCoord0().Set(1.0f - u, v);
    }
  }

  // North polar cap.
  size_t index_idx = 0;
  for (int lon = 0; lon < def.num_meridians; ++lon) {
    const Index row_start = row_indices[0];
    indices[index_idx++] = kNorthPoleIndex;
    indices[index_idx++] = static_cast<Index>(row_start + lon + 1);
    indices[index_idx++] = static_cast<Index>(row_start + lon);
  }

  // Latitudinal triangle strips.
  for (int lat = 0; lat < def.num_parallels - 1; lat++) {
    const Index north_start = row_indices[lat];
    const Index south_start = row_indices[lat + 1];

    for (int lon = 0; lon < def.num_meridians; ++lon) {
      const Index next_lon = static_cast<Index>(lon + 1);
      const Index north_v0 = static_cast<Index>(north_start + lon);
      const Index north_v1 = static_cast<Index>(north_start + next_lon);
      const Index south_v0 = static_cast<Index>(south_start + lon);
      const Index south_v1 = static_cast<Index>(south_start + next_lon);
      indices[index_idx++] = north_v0;
      indices[index_idx++] = north_v1;
      indices[index_idx++] = south_v0;
      indices[index_idx++] = north_v1;
      indices[index_idx++] = south_v1;
      indices[index_idx++] = south_v0;
    }
  }

  // South polar cap.
  for (int lon = 0; lon < def.num_meridians; ++lon) {
    const Index row_start = row_indices[row_indices.size() - 1];
    indices[index_idx++] = kSouthPoleIndex;
    indices[index_idx++] = static_cast<Index>(row_start + lon);
    indices[index_idx++] = static_cast<Index>(row_start + lon + 1);
  }

  CHECK(vertex_idx == vertices.size());
  CHECK(index_idx == indices.size());
}

}  // namespace redux

#endif  // REDUX_SYSTEMS_SHAPE_SPHERE_SHAPE_GENERATOR_H_
