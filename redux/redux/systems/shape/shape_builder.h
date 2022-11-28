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

#ifndef REDUX_SYSTEMS_SHAPE_SHAPE_BUILDER_H_
#define REDUX_SYSTEMS_SHAPE_SHAPE_BUILDER_H_

#include <utility>

#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/logging.h"
#include "redux/systems/shape/box_shape_generator.h"
#include "redux/systems/shape/sphere_shape_generator.h"

namespace redux {

// Populates a buffer of data with Vertices and Indices using generic algorithms
// for various types of shapes.
//
// The format of the vertices and indices is defined by the template arguments.
template <typename Vertex, typename Index>
class ShapeBuilder {
 public:
  // Given a definition of a shape, generates vertex and index buffers for that
  // shape. Requires the following functions to be defined for the given def.
  //
  //   size_t CalculateVertexCount: Returns the number of vertices required
  //                                for the shape.
  //
  //   size_t CalculateIndexCount: Returns the number of indices required for
  //                               the shape.
  //
  //   void GenerateShape<Vertex, Index>: Populate the provided vertex and index
  //                                      buffers such that they create a shape.
  template <typename T>
  void Build(const T& def) {
    const size_t num_vertices = CalculateVertexCount(def);
    DataBuilder vertices(sizeof(Vertex) * num_vertices);
    auto* vertices_ptr = reinterpret_cast<Vertex*>(
        vertices.GetAppendPtr(vertices.GetCapacity()));

    const size_t num_indices = CalculateIndexCount(def);
    DataBuilder indices(sizeof(Index) * num_indices);
    auto* indices_ptr =
        reinterpret_cast<Index*>(indices.GetAppendPtr(indices.GetCapacity()));

    GenerateShape<Vertex, Index>({vertices_ptr, num_vertices},
                                 {indices_ptr, num_indices}, def);
    vertex_data_ = vertices.Release();
    index_data_ = indices.Release();
  }

  // Returns the vertex data of the generated shape.
  absl::Span<Vertex> Vertices() {
    auto bytes = vertex_data_.GetByteSpan();
    return {reinterpret_cast<Vertex*>(const_cast<std::byte*>(bytes.data())),
            bytes.size() / Vertex().GetVertexFormat().GetVertexSize()};
  }

  // Returns the index data of the generated shape.
  absl::Span<Index> Indices() {
    auto bytes = index_data_.GetByteSpan();
    return {reinterpret_cast<Index*>(const_cast<std::byte*>(bytes.data())),
            bytes.size() / sizeof(Index)};
  }

  // Releases the vertex buffer to the caller.
  DataContainer ReleaseVertices() { return std::move(vertex_data_); }

  // Releases the index buffer to the caller.
  DataContainer ReleaseIndices() { return std::move(index_data_); }

 private:
  DataContainer vertex_data_;
  DataContainer index_data_;
};

}  // namespace redux

#endif  // REDUX_SYSTEMS_SHAPE_SHAPE_BUILDER_H_
