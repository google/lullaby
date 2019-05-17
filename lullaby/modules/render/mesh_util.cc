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

#include <array>

#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

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
      DataContainer::CreateHeapDataContainer(num_vertices * sizeof(VertexPTN));
  DataContainer index_data =
      DataContainer::CreateHeapDataContainer(num_indices * sizeof(uint32_t));
  MeshData mesh =
      MeshData(MeshData::kTriangles, VertexPTN::kFormat, std::move(vertex_data),
               MeshData::kIndexU32, std::move(index_data));

  // Pole vertices.
  const uint32_t north_pole = *mesh.AddVertex<VertexPTN>(
      0.0f, radius, 0.0f, .5f, 0.0f, 0.0f, 1.0f, 0.0f);
  const uint32_t south_pole = *mesh.AddVertex<VertexPTN>(
      0.0f, -radius, 0.0f, .5f, 1.0f, 0.0f, -1.0f, 0.0f);

  // Vertices by latitude.
  std::vector<uint32_t> row_indices(num_parallels);
  float phi = kPhiStep;
  for (int lat = 0; lat < num_parallels; ++lat, phi += kPhiStep) {
    const float cos_phi = std::cos(phi);
    const float sin_phi = std::sin(phi);
    const float rad_sin_phi = radius * sin_phi;
    const float y = radius * cos_phi;
    const float ny = cos_phi;
    const float v = phi / kPi;

    row_indices[lat] = mesh.GetNumVertices();

    float theta = 0.0f;
    for (int lon = 0; lon < num_meridians; ++lon, theta += kThetaStep) {
      const float cos_theta = std::cos(theta);
      const float sin_theta = std::sin(theta);
      const float x = rad_sin_phi * cos_theta;
      const float z = rad_sin_phi * sin_theta;
      const float u = theta / (2.0f * kPi);
      const float nx = sin_phi * cos_theta;
      const float nz = sin_phi * sin_theta;
      mesh.AddVertex<VertexPTN>(x, y, z, u, v, nx, ny, nz);
    }

    // Add a u = 1.0 vertex, otherwise the final longitudinal strip will blend
    // from u=(num_meridians-1/num_meridians) to u=0.0... back across almost all
    // of the texture.
    mesh.AddVertex<VertexPTN>(rad_sin_phi, y, 0.0f, 1.0f, v, sin_phi, ny, 0.0f);
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

MeshData CreateArrowMesh(float start_angle, float delta_angle,
                         float line_length, float line_width, float line_offset,
                         float pointer_height, float pointer_length,
                         Color4ub color) {
  return CreateArrowMeshWithTint(start_angle, delta_angle, line_length,
                                 line_width, line_offset, pointer_height,
                                 pointer_length, color, color);
}

MeshData CreateArrowMeshWithTint(float start_angle, float delta_angle,
                                 float line_length, float line_width,
                                 float line_offset, float pointer_height,
                                 float pointer_length, Color4ub start_tint,
                                 Color4ub end_tint) {
  const float end_angle = mathfu::kPi * 2.f;
  const float num_iterations = end_angle / delta_angle;
  const size_t num_vertices = 8 + static_cast<size_t>(num_iterations);
  const size_t num_triangles = 2 * num_vertices - 8;
  lull::MeshData arrow(lull::MeshData::PrimitiveType::kTriangles,
                       lull::VertexPC::kFormat,
                       lull::DataContainer::CreateHeapDataContainer(
                           num_vertices * sizeof(lull::VertexPC)),
                       lull::MeshData::kIndexU16,
                       lull::DataContainer::CreateHeapDataContainer(
                           3 * num_triangles * sizeof(uint16_t)));
  // Line segment vertices
  arrow.AddVertex<lull::VertexPC>(-line_width, -line_width, line_offset,
                                  start_tint);
  arrow.AddVertex<lull::VertexPC>(line_width, -line_width, line_offset,
                                  start_tint);
  arrow.AddVertex<lull::VertexPC>(0.f, line_width, line_offset, start_tint);
  arrow.AddVertex<lull::VertexPC>(-line_width, -line_width,
                                  line_offset + line_length, end_tint);
  arrow.AddVertex<lull::VertexPC>(0.f, line_width, line_offset + line_length,
                                  end_tint);
  arrow.AddVertex<lull::VertexPC>(line_width, -line_width,
                                  line_offset + line_length, end_tint);
  // Add the indices for the shaft of the arrow.
  // clang-format off
  const std::array<uint16_t, 21> arrow_shaft_indices = {
    2, 1, 0,
    3, 4, 0,
    4, 2, 0,
    1, 3, 0,
    5, 3, 1,
    2, 5, 1,
    4, 5, 2
    };
  // clang-format on
  arrow.AddIndices(arrow_shaft_indices.data(), arrow_shaft_indices.size());

  // Pointer part of the arrow is drawn programmatically to allow for a higher
  // resolution.
  const float pointer_base_point = line_offset + line_length;
  const float pointer_end_point = pointer_base_point + pointer_length;
  const uint32_t pointer_base_index =
      arrow.AddVertex<lull::VertexPC>(0.f, 0.f, pointer_base_point, start_tint)
          .value();
  const uint32_t pointer_end_index =
      arrow.AddVertex<lull::VertexPC>(0.f, 0.f, pointer_end_point, start_tint)
          .value();
  uint32_t current_index =
      arrow
          .AddVertex<lull::VertexPC>(pointer_height * cosf(start_angle),
                                     pointer_height * sinf(start_angle),
                                     pointer_base_point, end_tint)
          .value();

  for (float angle = start_angle + delta_angle; angle < end_angle;
       angle += delta_angle) {
    arrow.AddVertex<lull::VertexPC>(pointer_height * cosf(angle),
                                    pointer_height * sinf(angle),
                                    pointer_base_point, end_tint);
    ++current_index;
    // Creating the front facing part of the pointer.
    arrow.AddIndex(pointer_end_index);
    arrow.AddIndex(current_index - 1);
    arrow.AddIndex(current_index);
    // Creating the back facing part of the pointer.
    arrow.AddIndex(pointer_base_index);
    arrow.AddIndex(current_index);
    arrow.AddIndex(current_index - 1);
  }
  // Create the last front facing part of the pointer.
  arrow.AddIndex(pointer_end_index);
  arrow.AddIndex(current_index);
  arrow.AddIndex(pointer_end_index + 1);
  // Create the last back facing part of the pointer.
  arrow.AddIndex(pointer_base_index);
  arrow.AddIndex(pointer_end_index + 1);
  arrow.AddIndex(current_index);

  return arrow;
}

}  // namespace lull
