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

#ifndef REDUX_SYSTEMS_SHAPE_BOX_SHAPE_GENERATOR_H_
#define REDUX_SYSTEMS_SHAPE_BOX_SHAPE_GENERATOR_H_

#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/graphics/vertex.h"
#include "redux/systems/shape/shape_def_generated.h"

namespace redux {

inline size_t CalculateVertexCount(const BoxShapeDef& def) {
  static constexpr size_t kNumFaces = 6;
  static constexpr size_t kNumCornersPerFace = 4;
  return kNumFaces * kNumCornersPerFace;
}

inline size_t CalculateIndexCount(const BoxShapeDef& def) {
  static constexpr size_t kNumFaces = 6;
  static constexpr size_t kNumTrianglesPerFace = 2;
  static constexpr size_t kNumVerticesPerTriangle = 3;
  return kNumFaces * kNumTrianglesPerFace * kNumVerticesPerTriangle;
}

template <typename Vertex, typename Index>
void GenerateShape(absl::Span<Vertex> vertices, absl::Span<Index> indices,
                   const BoxShapeDef& def) {
  CHECK(vertices.size() == CalculateVertexCount(def));
  CHECK(indices.size() == CalculateIndexCount(def));

  // We'll refer to this diagram for the purposes of this code. Note that the
  // "back" face appears upside-down, so we do a little processing at the end to
  // fix this up.
  //
  //               A+--------+B
  //                |        |
  //                |  TOP   |
  //               C|        |D
  //      A+--------+--------+--------+B
  //       |        |        |        |
  //       |  LEFT  | FRONT  | RIGHT  |
  //       |        |        |        |
  //      E+--------+--------+--------+F
  //               G|        |H
  //                | BOTTOM |
  //                |        |
  //               E+--------+F
  //                |        |
  //                |  BACK  |
  //                |        |
  //               A+--------+B
  //
  enum Faces {
    kFront,
    kBack,
    kLeft,
    kRight,
    kTop,
    kBottom,
    kNumFaces,
  };

  enum QuadCorners {
    kTopLeft,
    kTopRight,
    kBottomLeft,
    kBottomRight,
    kNumQuadCorners,
  };

  // Generate the indices for each vertex of the box.
  auto box_index = [](int face, int corner) constexpr {
    return (kNumQuadCorners * face) + corner;
  };
  static constexpr auto kFrontTopLeft = box_index(kFront, kTopLeft);
  static constexpr auto kFrontTopRight = box_index(kFront, kTopRight);
  static constexpr auto kFrontBottomLeft = box_index(kFront, kBottomLeft);
  static constexpr auto kFrontBottomRight = box_index(kFront, kBottomRight);
  static constexpr auto kBackTopLeft = box_index(kBack, kTopLeft);
  static constexpr auto kBackTopRight = box_index(kBack, kTopRight);
  static constexpr auto kBackBottomLeft = box_index(kBack, kBottomLeft);
  static constexpr auto kBackBottomRight = box_index(kBack, kBottomRight);
  static constexpr auto kLeftTopLeft = box_index(kLeft, kTopLeft);
  static constexpr auto kLeftTopRight = box_index(kLeft, kTopRight);
  static constexpr auto kLeftBottomLeft = box_index(kLeft, kBottomLeft);
  static constexpr auto kLeftBottomRight = box_index(kLeft, kBottomRight);
  static constexpr auto kRightTopLeft = box_index(kRight, kTopLeft);
  static constexpr auto kRightTopRight = box_index(kRight, kTopRight);
  static constexpr auto kRightBottomLeft = box_index(kRight, kBottomLeft);
  static constexpr auto kRightBottomRight = box_index(kRight, kBottomRight);
  static constexpr auto kTopTopLeft = box_index(kTop, kTopLeft);
  static constexpr auto kTopTopRight = box_index(kTop, kTopRight);
  static constexpr auto kTopBottomLeft = box_index(kTop, kBottomLeft);
  static constexpr auto kTopBottomRight = box_index(kTop, kBottomRight);
  static constexpr auto kBottomTopLeft = box_index(kBottom, kTopLeft);
  static constexpr auto kBottomTopRight = box_index(kBottom, kTopRight);
  static constexpr auto kBottomBottomLeft = box_index(kBottom, kBottomLeft);
  static constexpr auto kBottomBottomRight = box_index(kBottom, kBottomRight);

  enum BoxCorners {
    kCornerA,
    kCornerB,
    kCornerC,
    kCornerD,
    kCornerE,
    kCornerF,
    kCornerG,
    kCornerH,
    kNumBoxCorners,
  };

  const float hx = def.half_extents.x;
  const float hy = def.half_extents.y;
  const float hz = def.half_extents.z;

  vec3 points[kNumBoxCorners];
  points[kCornerA] = vec3(-hx, hy, -hz);
  points[kCornerB] = vec3(hx, hy, -hz);
  points[kCornerC] = vec3(-hx, hy, hz);
  points[kCornerD] = vec3(hx, hy, hz);
  points[kCornerE] = vec3(-hx, -hy, -hz);
  points[kCornerF] = vec3(hx, -hy, -hz);
  points[kCornerG] = vec3(-hx, -hy, hz);
  points[kCornerH] = vec3(hx, -hy, hz);

  vec3 normals[kNumFaces];
  normals[kFront] = vec3(0, 0, 1);
  normals[kBack] = vec3(0, 0, -1);
  normals[kLeft] = vec3(-1, 0, 0);
  normals[kRight] = vec3(1, 0, 0);
  normals[kTop] = vec3(0, 1, 0);
  normals[kBottom] = vec3(0, -1, 0);

  //
  vertices[kFrontTopLeft].Position().SetVector(points[kCornerC]);
  vertices[kFrontTopRight].Position().SetVector(points[kCornerD]);
  vertices[kFrontBottomLeft].Position().SetVector(points[kCornerG]);
  vertices[kFrontBottomRight].Position().SetVector(points[kCornerH]);
  vertices[kBackTopLeft].Position().SetVector(points[kCornerE]);
  vertices[kBackTopRight].Position().SetVector(points[kCornerF]);
  vertices[kBackBottomLeft].Position().SetVector(points[kCornerA]);
  vertices[kBackBottomRight].Position().SetVector(points[kCornerB]);
  vertices[kLeftTopLeft].Position().SetVector(points[kCornerA]);
  vertices[kLeftTopRight].Position().SetVector(points[kCornerC]);
  vertices[kLeftBottomLeft].Position().SetVector(points[kCornerE]);
  vertices[kLeftBottomRight].Position().SetVector(points[kCornerG]);
  vertices[kRightTopLeft].Position().SetVector(points[kCornerD]);
  vertices[kRightTopRight].Position().SetVector(points[kCornerB]);
  vertices[kRightBottomLeft].Position().SetVector(points[kCornerH]);
  vertices[kRightBottomRight].Position().SetVector(points[kCornerF]);
  vertices[kTopTopLeft].Position().SetVector(points[kCornerA]);
  vertices[kTopTopRight].Position().SetVector(points[kCornerB]);
  vertices[kTopBottomLeft].Position().SetVector(points[kCornerC]);
  vertices[kTopBottomRight].Position().SetVector(points[kCornerD]);
  vertices[kBottomTopLeft].Position().SetVector(points[kCornerG]);
  vertices[kBottomTopRight].Position().SetVector(points[kCornerH]);
  vertices[kBottomBottomLeft].Position().SetVector(points[kCornerE]);
  vertices[kBottomBottomRight].Position().SetVector(points[kCornerF]);

  size_t idx = 0;
  for (int face = 0; face < kNumFaces; ++face) {
    for (int corner = 0; corner < kNumQuadCorners; ++corner) {
      vertices[box_index(face, corner)].Normal().SetVector(normals[face]);
      vertices[box_index(face, corner)].TangentFromNormal(normals[face]);
      vertices[box_index(face, corner)].OrientationFromNormal(normals[face]);
    }
    if (face == kBack) {
      vertices[box_index(face, kTopLeft)].TexCoord0().Set(0, 1);
      vertices[box_index(face, kTopRight)].TexCoord0().Set(1, 1);
      vertices[box_index(face, kBottomLeft)].TexCoord0().Set(0, 0);
      vertices[box_index(face, kBottomRight)].TexCoord0().Set(1, 0);
    } else {
      vertices[box_index(face, kTopLeft)].TexCoord0().Set(0, 0);
      vertices[box_index(face, kTopRight)].TexCoord0().Set(1, 0);
      vertices[box_index(face, kBottomLeft)].TexCoord0().Set(0, 1);
      vertices[box_index(face, kBottomRight)].TexCoord0().Set(1, 1);
    }

    indices[idx++] = box_index(face, kTopLeft);
    indices[idx++] = box_index(face, kTopRight);
    indices[idx++] = box_index(face, kBottomLeft);
    indices[idx++] = box_index(face, kTopRight);
    indices[idx++] = box_index(face, kBottomRight);
    indices[idx++] = box_index(face, kBottomLeft);
  }

  CHECK_EQ(vertices.size(), kBottomBottomRight + 1);
  CHECK_EQ(indices.size(), idx);
}

}  // namespace redux

#endif  // REDUX_SYSTEMS_SHAPE_BOX_SHAPE_GENERATOR_H_
