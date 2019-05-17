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

#include "lullaby/modules/render/tangent_generation.h"

#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

template <typename T>
class BufferWithStride {
 public:
  BufferWithStride(uint8_t* buffer, size_t stride = sizeof(T))
      : buffer_(buffer), stride_(stride) {}

  T& operator[](size_t index) {
    return *reinterpret_cast<T*>(buffer_ + index * stride_);
  }

 private:
  uint8_t* buffer_;
  size_t stride_;
};

struct TriangleIndices {
  size_t a = 0;
  size_t b = 0;
  size_t c = 0;
};

template <typename TriangleFn>
void ComputeTangents(const uint8_t* positions_ptr, size_t position_stride,
                     const uint8_t* normals_ptr, size_t normal_stride,
                     const uint8_t* tex_coords_ptr, size_t tex_coord_stride,
                     size_t vertex_count, const TriangleFn& triangle_fn,
                     size_t triangle_count, uint8_t* tangents_ptr,
                     size_t tangent_stride, uint8_t* bitangents_ptr,
                     size_t bitangent_stride) {
  BufferWithStride<mathfu::vec3_packed> positions(
      const_cast<uint8_t*>(positions_ptr), position_stride);
  BufferWithStride<mathfu::vec3_packed> normals(
      const_cast<uint8_t*>(normals_ptr), normal_stride);
  BufferWithStride<mathfu::vec2_packed> tex_coords(
      const_cast<uint8_t*>(tex_coords_ptr), tex_coord_stride);
  BufferWithStride<mathfu::vec4_packed> tangents(
      const_cast<uint8_t*>(tangents_ptr), tangent_stride);
  BufferWithStride<mathfu::vec3_packed> bitangents(
      const_cast<uint8_t*>(bitangents_ptr), bitangent_stride);

  // Zero out tangents to prepare for accumulation.
  for (int ii = 0; ii < vertex_count; ++ii) {
    tangents[ii] = mathfu::vec4_packed(mathfu::vec4(0, 0, 0, 0));
    bitangents[ii] = mathfu::vec3_packed(mathfu::vec3(0, 0, 0));
  }

  // Compute and accumulate tangent values.
  for (int i = 0; i < triangle_count; ++i) {
    TriangleIndices triangle = triangle_fn(i);

    const mathfu::vec3 pos_a(positions[triangle.a]);
    const mathfu::vec3 pos_b(positions[triangle.b]);
    const mathfu::vec3 pos_c(positions[triangle.c]);

    const mathfu::vec2 uv_a(tex_coords[triangle.a]);
    const mathfu::vec2 uv_b(tex_coords[triangle.b]);
    const mathfu::vec2 uv_c(tex_coords[triangle.c]);

    const mathfu::vec3 edge_ba = pos_b - pos_a;
    const mathfu::vec3 edge_ca = pos_c - pos_a;

    mathfu::vec2 uv_ba = uv_b - uv_a;
    mathfu::vec2 uv_ca = uv_c - uv_a;

    if (uv_ba == mathfu::kZeros2f && uv_ca == mathfu::kZeros2f) {
      uv_ba = {0.f, 1.f};
      uv_ca = {1.f, 0.f};
    }

    const float direction =
        (uv_ca.x * uv_ba.y - uv_ca.y * uv_ba.x) < 0.f ? -1.f : 1.f;

    const mathfu::vec3 tangent =
        ((edge_ca * uv_ba.y - edge_ba * uv_ca.y) * direction).Normalized();
    const mathfu::vec3 bitangent =
        ((edge_ca * uv_ba.x - edge_ba * uv_ca.x) * direction).Normalized();

    mathfu::vec4 tan_a(tangents[triangle.a]);
    mathfu::vec4 tan_b(tangents[triangle.b]);
    mathfu::vec4 tan_c(tangents[triangle.c]);
    mathfu::vec3 bitan_a(bitangents[triangle.a]);
    mathfu::vec3 bitan_b(bitangents[triangle.b]);
    mathfu::vec3 bitan_c(bitangents[triangle.c]);

    tan_a += mathfu::vec4(tangent, 0.f);
    tan_b += mathfu::vec4(tangent, 0.f);
    tan_c += mathfu::vec4(tangent, 0.f);
    bitan_a += mathfu::vec3(bitangent);
    bitan_b += mathfu::vec3(bitangent);
    bitan_c += mathfu::vec3(bitangent);

    tangents[triangle.a] = mathfu::vec4_packed(tan_a);
    tangents[triangle.b] = mathfu::vec4_packed(tan_b);
    tangents[triangle.c] = mathfu::vec4_packed(tan_c);
    bitangents[triangle.a] = mathfu::vec3_packed(bitan_a);
    bitangents[triangle.b] = mathfu::vec3_packed(bitan_b);
    bitangents[triangle.c] = mathfu::vec3_packed(bitan_c);
  }

  // Normalize tangents and bitangents and compute handedness.
  for (int ii = 0; ii < vertex_count; ++ii) {
    const mathfu::vec3 normal(normals[ii]);
    const mathfu::vec4 tangent_with_handedness(tangents[ii]);
    const mathfu::vec3 tangent = tangent_with_handedness.xyz().Normalized();
    const mathfu::vec3 bitangent = mathfu::vec3(bitangents[ii]).Normalized();
    const float hand =
        (mathfu::dot(mathfu::cross(normal, tangent), bitangent) < 0.f) ? -1.f
                                                                       : 1.f;
    tangents[ii] = mathfu::vec4_packed(mathfu::vec4(tangent, hand));
    bitangents[ii] = mathfu::vec3_packed(bitangent);
  }
}

void ComputeTangentsWithIndexedTriangles(
    const uint8_t* positions_ptr, size_t position_stride,
    const uint8_t* normals_ptr, size_t normal_stride,
    const uint8_t* tex_coords_ptr, size_t tex_coord_stride, size_t vertex_count,
    const uint8_t* triangle_indices_ptr, size_t sizeof_index,
    size_t triangle_count, uint8_t* tangents_ptr, size_t tangent_stride,
    uint8_t* bitangents_ptr, size_t bitangent_stride) {
  auto triangle_fn = [triangle_indices_ptr, sizeof_index](int index) {
    TriangleIndices triangle;

    switch (sizeof_index) {
      case 2: {
        const uint16_t* indices =
            reinterpret_cast<const uint16_t*>(triangle_indices_ptr);
        triangle.a = static_cast<size_t>(indices[index * 3 + 0]);
        triangle.b = static_cast<size_t>(indices[index * 3 + 1]);
        triangle.c = static_cast<size_t>(indices[index * 3 + 2]);
        break;
      }
      case 4: {
        const uint32_t* indices(
            reinterpret_cast<const uint32_t*>(triangle_indices_ptr));
        triangle.a = static_cast<size_t>(indices[index * 3 + 0]);
        triangle.b = static_cast<size_t>(indices[index * 3 + 1]);
        triangle.c = static_cast<size_t>(indices[index * 3 + 2]);
        break;
      }
      case 8: {
        const uint64_t* indices(
            reinterpret_cast<const uint64_t*>(triangle_indices_ptr));
        triangle.a = static_cast<size_t>(indices[index * 3 + 0]);
        triangle.b = static_cast<size_t>(indices[index * 3 + 1]);
        triangle.c = static_cast<size_t>(indices[index * 3 + 2]);
        break;
      }
      default: {
        LOG(DFATAL) << "Unsupported vertex index type with size "
                    << sizeof_index << ".";
        return triangle;
      }
    }

    return triangle;
  };

  ComputeTangents(positions_ptr, position_stride, normals_ptr, normal_stride,
                  tex_coords_ptr, tex_coord_stride, vertex_count, triangle_fn,
                  triangle_count, tangents_ptr, tangent_stride, bitangents_ptr,
                  bitangent_stride);
}

void ComputeTangentsWithTriangles(
    const uint8_t* positions_ptr, size_t position_stride,
    const uint8_t* normals_ptr, size_t normal_stride,
    const uint8_t* tex_coords_ptr, size_t tex_coord_stride, size_t vertex_count,
    size_t triangle_count, uint8_t* tangents_ptr, size_t tangent_stride,
    uint8_t* bitangents_ptr, size_t bitangent_stride) {
  auto triangle_fn = [](int index) {
    TriangleIndices triangle;

    triangle.a = index * 3 + 0;
    triangle.b = index * 3 + 1;
    triangle.c = index * 3 + 2;

    return triangle;
  };

  ComputeTangents(positions_ptr, position_stride, normals_ptr, normal_stride,
                  tex_coords_ptr, tex_coord_stride, vertex_count, triangle_fn,
                  triangle_count, tangents_ptr, tangent_stride, bitangents_ptr,
                  bitangent_stride);
}
}  // namespace lull
