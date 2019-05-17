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

#ifndef LULLABY_MODULES_RENDER_TANGENT_GENERATION_H_
#define LULLABY_MODULES_RENDER_TANGENT_GENERATION_H_

#include <stddef.h>
#include <stdint.h>

namespace lull {

// The functions here are intended to compute tangents and bitangents given a
// set of triangles and vertex position, normal, and texture coordinate data for
// those triangles.  Tangents and bitangent storage must be preallocated before
// entering these functions.  All attributes are assumed to have a float basic
// type with the following vector types:
// vec3 position
// vec3 normal
// vec2 tex_coord
// vec4 tangent - The w component encodes handedness:  1 for right, -1 for left.
// vec3 bitangent

// Computes tangents and bitangents, storing them in |tangents| and |bitangents|
// using the specified positions, normals, texture coordinates, and triangle
// indices.
void ComputeTangentsWithIndexedTriangles(
    const uint8_t* positions, size_t position_stride, const uint8_t* normals,
    size_t normal_stride, const uint8_t* tex_coords, size_t tex_coord_stride,
    size_t vertex_count, const uint8_t* triangle_indices, size_t sizeof_index,
    size_t triangle_count, uint8_t* tangents, size_t tangent_stride,
    uint8_t* bitangents, size_t bitangent_stride);

// Computes tangents and bitangents, storing them in |tangents| and |bitangents|
// using the specified positions, normals, texture coordinates.  Vertex data is
// assumed to be ordered as the vertices of a set of triangles.
void ComputeTangentsWithTriangles(
    const uint8_t* positions, size_t position_stride, const uint8_t* normals,
    size_t normal_stride, const uint8_t* tex_coords, size_t tex_coord_stride,
    size_t vertex_count, size_t triangle_count, uint8_t* tangents,
    size_t tangent_stride, uint8_t* bitangents, size_t bitangent_stride);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_TANGENT_GENERATION_H_
