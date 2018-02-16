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

#include "lullaby/modules/render/mesh_util.h"

#include <array>

#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

size_t GetTesselatedQuadVertexCount(int num_verts_x, int num_verts_y,
                                    int corner_verts,
                                    size_t* num_interior_verts_x,
                                    size_t* num_interior_verts_y) {
  const size_t interior_verts_x =
      corner_verts > 0 ? num_verts_x - 2 : num_verts_x;
  const size_t interior_verts_y =
      corner_verts > 0 ? num_verts_y - 2 : num_verts_y;
  const size_t num_border_verts =
      corner_verts > 0
          ? (4 * corner_verts) + (2 * interior_verts_x) + (2 * interior_verts_y)
          : 0;

  if (num_interior_verts_x) {
    *num_interior_verts_x = interior_verts_x;
  }
  if (num_interior_verts_y) {
    *num_interior_verts_y = interior_verts_y;
  }

  // The radiused corners add |corner_verts| vertices for each of the four
  // corners as well as an additional line of interior verts on each side
  // of the quad for the tabs.
  return (interior_verts_x * interior_verts_y) + num_border_verts;
}

size_t GetTesselatedQuadIndexCount(int num_verts_x, int num_verts_y,
                                   int corner_verts) {
  const int quads_x = num_verts_x - 1;
  const int quads_y = num_verts_y - 1;
  // Addition of radiused corners remove the 4 outer quads for a total of 24
  // indices saved but add corner_verts + 1 triangles at each of the 4 corners.
  const size_t num_indices = corner_verts > 0 ? (quads_x * quads_y * 6) - 24 +
                                                    (12 * (corner_verts + 1))
                                              : quads_x * quads_y * 6;

  return num_indices;
}

std::vector<uint16_t> CalculateTesselatedQuadIndices(int num_verts_x,
                                                     int num_verts_y,
                                                     int corner_verts) {
  if (corner_verts > 0) {
    // We reserve 2 additional verts in each dimension to generate the "tabs"
    // that overhang the central quad on the sides for the triangle fan to
    // connect to.
    if (num_verts_x < 4 || num_verts_y < 4) {
      LOG(DFATAL) << "Failed to reserve 4 additional vertices.";
      return std::vector<uint16_t>();
    }
  } else if (corner_verts == 0) {
    if (num_verts_x < 2 || num_verts_y < 2) {
      LOG(DFATAL) << "Failed to reserve 2 additional vertices.";
      return std::vector<uint16_t>();
    }
  } else {
    LOG(DFATAL) << "Must have >= 0 corner vertices.";
    return std::vector<uint16_t>();
  }

  // Define each quad as 2 triangles, each made from 4 vertices:
  //
  //  2---5---8
  //  | B | D |
  //  1---4---7
  //  | A | C |
  //  0---3---6
  //
  // Quad A is defined as two triangles: (1, 0, 3) and (4, 1, 3).
  //
  //  1-4
  //  |\|
  //  0-3

  const int quads_x = num_verts_x - 1;
  const int quads_y = num_verts_y - 1;
  // Addition of radiused corners remove the 4 outer quads for a total of 24
  // indices saved but add corner_verts + 1 triangles at each of the 4 corners.
  const size_t num_indices = corner_verts > 0 ? (quads_x * quads_y * 6) - 24 +
                                                    (12 * (corner_verts + 1))
                                              : quads_x * quads_y * 6;
  std::vector<uint16_t> indices(num_indices);
  size_t index = 0;
  uint16_t anchor_vert_index = 0;

  for (int x = 0; x < quads_x; ++x) {
    int x_increment = num_verts_y;
    int y_start = 0;
    int y_end = quads_y;
    if (corner_verts > 0 && (x == 0 || x == quads_x - 1)) {
      x_increment = quads_y;
      y_start = 1;
      y_end = quads_y - 1;
      if (x == quads_x - 1) {
        // We need to skip generating the quad in the lower right-hand corner.
        ++anchor_vert_index;
      }
    }
    for (int y = y_start; y < y_end; ++y) {
      const int bottom_left = anchor_vert_index;
      const int bottom_right = anchor_vert_index + x_increment;
      const int top_left = bottom_left + 1;
      const int top_right = bottom_right + 1;

      // triangle 1:
      indices[index++] = static_cast<uint16_t>(top_left);
      indices[index++] = static_cast<uint16_t>(bottom_left);
      indices[index++] = static_cast<uint16_t>(bottom_right);
      // triangle 2:
      indices[index++] = static_cast<uint16_t>(top_right);
      indices[index++] = static_cast<uint16_t>(top_left);
      indices[index++] = static_cast<uint16_t>(bottom_right);

      ++anchor_vert_index;
    }
    // We never generate quads using the top row of verts as an anchor, so skip
    // to the next vertex before starting another row of y vertices.
    ++anchor_vert_index;
  }

  if (corner_verts > 0) {
    // Pick out some key indices representing corners of the central quad.
    const int quad_lower_left_index = num_verts_y - 1;
    const int quad_upper_left_index = quad_lower_left_index + num_verts_y - 3;
    const int quad_lower_right_index =
        quad_lower_left_index + (num_verts_y * (num_verts_x - 3));
    const int quad_upper_right_index = quad_lower_right_index + num_verts_y - 3;
    const int first_fan_index = (num_verts_x * num_verts_y) - 4;
    int lower_left_fan_index = quad_lower_left_index - 1;
    int upper_left_fan_index = quad_upper_left_index - quads_y;
    int lower_right_fan_index = quad_lower_right_index + quads_y;
    int upper_right_fan_index = quad_upper_right_index + 1;
    for (int i = 0; i < corner_verts; ++i) {
      const int fan_set_index = first_fan_index + (i * 4);
      // Lower left fan.
      indices[index++] = static_cast<uint16_t>(fan_set_index);
      indices[index++] = static_cast<uint16_t>(lower_left_fan_index);
      indices[index++] = static_cast<uint16_t>(quad_lower_left_index);
      // Upper left fan.
      indices[index++] = static_cast<uint16_t>(fan_set_index + 1);
      indices[index++] = static_cast<uint16_t>(upper_left_fan_index);
      indices[index++] = static_cast<uint16_t>(quad_upper_left_index);
      // Lower right fan.
      indices[index++] = static_cast<uint16_t>(fan_set_index + 2);
      indices[index++] = static_cast<uint16_t>(lower_right_fan_index);
      indices[index++] = static_cast<uint16_t>(quad_lower_right_index);
      // Upper right fan.
      indices[index++] = static_cast<uint16_t>(fan_set_index + 3);
      indices[index++] = static_cast<uint16_t>(upper_right_fan_index);
      indices[index++] = static_cast<uint16_t>(quad_upper_right_index);

      lower_left_fan_index = fan_set_index;
      upper_left_fan_index = fan_set_index + 1;
      lower_right_fan_index = fan_set_index + 2;
      upper_right_fan_index = fan_set_index + 3;
    }
    // Append final 4 fan triangles, starting with lower left.
    indices[index++] = 0;
    indices[index++] = static_cast<uint16_t>(lower_left_fan_index);
    indices[index++] = static_cast<uint16_t>(quad_lower_left_index);
    // Final upper left fan triangle.
    indices[index++] = static_cast<uint16_t>(quad_upper_left_index + 1);
    indices[index++] = static_cast<uint16_t>(upper_left_fan_index);
    indices[index++] = static_cast<uint16_t>(quad_upper_left_index);
    // Final lower right fan triangle.
    indices[index++] = static_cast<uint16_t>(quad_lower_right_index - 1);
    indices[index++] = static_cast<uint16_t>(lower_right_fan_index);
    indices[index++] = static_cast<uint16_t>(quad_lower_right_index);
    // Final upper right fan triangle.
    indices[index++] = static_cast<uint16_t>(quad_upper_right_index + quads_y);
    indices[index++] = static_cast<uint16_t>(upper_right_fan_index);
    indices[index++] = static_cast<uint16_t>(quad_upper_right_index);
  }

  DCHECK_EQ(num_indices, index) << "Failed to fill indices array!";
  return indices;
}

void ApplyDeformation(MeshData* mesh, const PositionDeformation& deform) {
  const VertexFormat& format = mesh->GetVertexFormat();
  const VertexAttribute* position =
      format.GetAttributeWithUsage(VertexAttributeUsage_Position);
  if (!position || position->type() != VertexAttributeType_Vec3f) {
    LOG(DFATAL) << "Vertex format doesn't have pos3f";
    return;
  }

  float* vertex_data = reinterpret_cast<float*>(mesh->GetMutableVertexBytes());
  if (!vertex_data) {
    LOG(DFATAL) << "Can't deform mesh without read+write";
    return;
  }

  // Formats are always padded out to 4 bytes, so this is safe.
  DCHECK_EQ(format.GetVertexSize() % sizeof(float), 0);
  const size_t stride_in_floats = format.GetVertexSize() / sizeof(float);
  const size_t length_in_floats = mesh->GetNumVertices() * stride_in_floats;

  DCHECK_EQ(format.GetAttributeOffset(position) % sizeof(float), 0);
  vertex_data += format.GetAttributeOffset(position) / sizeof(float);

  for (size_t i = 0; i < length_in_floats; i += stride_in_floats) {
    const mathfu::vec3 original_position(vertex_data[i], vertex_data[i + 1],
                                         vertex_data[i + 2]);
    const mathfu::vec3 deformed_position = deform(original_position);
    vertex_data[i] = deformed_position.x;
    vertex_data[i + 1] = deformed_position.y;
    vertex_data[i + 2] = deformed_position.z;
  }
}

MeshData CreateLatLonSphere(float radius, int num_parallels,
                            int num_meridians) {
  CHECK_GE(num_parallels, 1);
  CHECK_GE(num_meridians, 3);

  const float kPhiStep = kPi / static_cast<float>(num_parallels + 1);
  const float kThetaStep = 2.0f * kPi / static_cast<float>(num_meridians);
  const size_t num_vertices = num_parallels * (num_meridians + 1) + 2;
  const size_t num_triangles =
      2 * num_meridians + 2 * num_meridians * (num_parallels - 1);
  const size_t num_indices = 3 * num_triangles;

  if (num_vertices > MeshData::kMaxValidIndexU32) {
    LOG(DFATAL) << "Exceeded vertex limit";
    return MeshData();
  }

  const bool flip_winding = radius < 0.0f;
  radius = std::abs(radius);

  DataContainer vertex_data =
      DataContainer::CreateHeapDataContainer(num_vertices * sizeof(VertexPT));
  DataContainer index_data =
      DataContainer::CreateHeapDataContainer(num_indices * sizeof(uint32_t));
  MeshData mesh =
      MeshData(MeshData::kTriangles, VertexPT::kFormat, std::move(vertex_data),
               MeshData::kIndexU32, std::move(index_data));

  // Pole vertices.
  const uint32_t north_pole =
      *mesh.AddVertex<VertexPT>(0.0f, radius, 0.0f, .5f, 0.0f);
  const uint32_t south_pole =
      *mesh.AddVertex<VertexPT>(0.0f, -radius, 0.0f, .5f, 1.0f);

  // Vertices by latitude.
  std::vector<uint32_t> row_indices(num_parallels);
  float phi = kPhiStep;
  for (int lat = 0; lat < num_parallels; ++lat, phi += kPhiStep) {
    const float cos_phi = std::cos(phi);
    const float rad_sin_phi = radius * std::sin(phi);
    const float y = radius * cos_phi;
    const float v = phi / kPi;

    row_indices[lat] = mesh.GetNumVertices();

    float theta = 0.0f;
    for (int lon = 0; lon < num_meridians; ++lon, theta += kThetaStep) {
      const float cos_theta = std::cos(theta);
      const float sin_theta = std::sin(theta);
      const float x = rad_sin_phi * cos_theta;
      const float z = rad_sin_phi * sin_theta;
      const float u = theta / (2.0f * kPi);
      mesh.AddVertex<VertexPT>(x, y, z, u, v);
    }

    // Add a u = 1.0 vertex, otherwise the final longitudinal strip will blend
    // from u=(num_meridians-1/num_meridians) to u=0.0... back across almost all
    // of the texture.
    mesh.AddVertex<VertexPT>(rad_sin_phi, y, 0.0f, 1.0f, v);
  }

  // North polar cap.
  for (int lon = 0; lon < num_meridians; ++lon) {
    const uint32_t row_start = row_indices[0];
    const uint32_t v1 = static_cast<uint32_t>(row_start + lon);
    const uint32_t v2 = static_cast<uint32_t>(row_start + lon + 1);
    std::array<uint32_t, 3> triangle = {{north_pole, v2, v1}};
    if (flip_winding) {
      std::swap(triangle[1], triangle[2]);
    }
    mesh.AddIndices(triangle.data(), triangle.size());
  }

  // Latitudinal triangle strips.
  for (int lat = 0; lat < num_parallels - 1; lat++) {
    const uint32_t north_start = row_indices[lat];
    const uint32_t south_start = row_indices[lat + 1];

    for (int lon = 0; lon < num_meridians; ++lon) {
      const uint32_t next_lon = static_cast<uint32_t>(lon + 1);
      const uint32_t north_v0 = static_cast<uint32_t>(north_start + lon);
      const uint32_t north_v1 = static_cast<uint32_t>(north_start + next_lon);
      const uint32_t south_v0 = static_cast<uint32_t>(south_start + lon);
      const uint32_t south_v1 = static_cast<uint32_t>(south_start + next_lon);

      std::array<uint32_t, 6> tris = {{north_v0, north_v1, south_v0,
                                       north_v1, south_v1, south_v0}};

      if (flip_winding) {
        std::swap(tris[1], tris[2]);
        std::swap(tris[4], tris[5]);
      }
      mesh.AddIndices(tris.data(), tris.size());
    }
  }

  // South polar cap.
  for (int lon = 0; lon < num_meridians; ++lon) {
    const uint32_t row_start = row_indices[row_indices.size() - 1];
    const uint32_t v1 = static_cast<uint32_t>(row_start + lon);
    const uint32_t v2 = static_cast<uint32_t>(row_start + lon + 1);
    std::array<uint32_t, 3> triangle = {{south_pole, v1, v2}};
    if (flip_winding) {
      std::swap(triangle[1], triangle[2]);
    }
    mesh.AddIndices(triangle.data(), triangle.size());
  }

  DCHECK_EQ(mesh.GetNumVertices(), num_vertices);
  DCHECK_EQ(mesh.GetNumIndices(), num_indices);

  return mesh;
}

Aabb GetBoundingBox(const MeshData& mesh) {
  if (mesh.GetNumVertices() == 0) {
    return {};
  }

  const VertexFormat& format = mesh.GetVertexFormat();
  const VertexAttribute* position =
      format.GetAttributeWithUsage(VertexAttributeUsage_Position);
  if (!position || position->type() != VertexAttributeType_Vec3f) {
    LOG(DFATAL) << "Vertex format doesn't have pos3f";
    return {};
  }

  const auto* vertex_data =
      reinterpret_cast<const float*>(mesh.GetVertexBytes());
  if (!vertex_data) {
    LOG(DFATAL) << "Can't get bounding box without read access.";
    return {};
  }

  // Formats are always padded out to 4 bytes, so this is safe.
  DCHECK_EQ(format.GetVertexSize() % sizeof(float), 0);
  const size_t stride_in_floats =
      mesh.GetVertexFormat().GetVertexSize() / sizeof(float);
  const size_t length_in_floats = mesh.GetNumVertices() * stride_in_floats;

  DCHECK_EQ(format.GetAttributeOffset(position) % sizeof(float), 0);
  vertex_data += format.GetAttributeOffset(position) / sizeof(float);

  Aabb box;
  // Use the first vertex as the min and max.
  box.min = mathfu::vec3(vertex_data[0], vertex_data[1], vertex_data[2]);
  box.max = box.min;

  // Skip the first vertex, then advance by stride.
  for (size_t i = stride_in_floats; i < length_in_floats;
       i += stride_in_floats) {
    const mathfu::vec3 p(vertex_data[i], vertex_data[i + 1],
                         vertex_data[i + 2]);
    box.min = mathfu::vec3::Min(box.min, p);
    box.max = mathfu::vec3::Max(box.max, p);
  }

  return box;
}

}  // namespace lull
