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

#ifndef LULLABY_UTIL_MESH_UTIL_H_
#define LULLABY_UTIL_MESH_UTIL_H_

#include <stdint.h>
#include <algorithm>
#include <functional>
#include <vector>

#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/quad_util.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

using PositionDeformation = std::function<mathfu::vec3(const mathfu::vec3&)>;

// Deforms |mesh| in-place by applying |deform| to each of its vertices. Fails
// with a DFATAL if |mesh| doesn't have read+write access.
void ApplyDeformation(MeshData* mesh, const PositionDeformation& deform);

// Creates a VertexPT sphere mesh using latitude-longitude tessellation. The
// sphere will be external-facing unless |radius| is negative. The mesh will
// always use 32-bit indices. The 'u' texture coordinate tracks longitude; the
// 'v' coordinate tracks latitude, with 0 and 1 at the north and south poles,
// respectively. |num_parallels| is the number of latitude divisions not incl.
// poles. [1,inf) |num_meridians| is the number of longitude divisions. [3,inf).
MeshData CreateLatLonSphere(float radius, int num_parallels, int num_meridians);

// Returns the 3D box that contains all of the points represented by |mesh|.
// Logs a DFATAL and returns an empty box if |mesh| doesn't have read access.
Aabb GetBoundingBox(const MeshData& mesh);

// Generates an arrow mesh with VertexPC vertices and uint16_t type indices.
// |start_angle| specifies the starting angle in radians from the xz-plane
// towards positive y-axis to start drawing the first vertex of the pointer.
// |delta_angle| specifies the change in angle before drawing a new vertex
// at where the cone and circle meet at the arrow pointer. |line_length|
// specifies the length of the arrow shaft. |line_width| is the width of the
// arrow_shaft (Note: the arrow shaft is a trangular prism). |line_offset|
// is the offset between the origin of the mesh and the start of the arrow
// shaft. |pointer_height| specifies the height of the arrow pointer relative
// to the base of the arrow pointer. |pointer_length| specifies the length of
// the arrow pointer. |color| specifies the color of the arrow.
MeshData CreateArrowMesh(float start_angle, float delta_angle,
                         float line_length, float line_width, float line_offset,
                         float pointer_height, float pointer_length,
                         Color4ub color);

// Same as above except you can have a gradient like arrow by specifying the
// |start_tint| which is the starting color, and |end_tint| being the ending
// color of the gradient. The start of the arrow is the base of the shaft and
// then transitions into the |end_tint| when it reaches the base of the arrow
// pointer, then does another transition into the |start_tint| and then back
// to the |end_tint| as it goes from the base of the pointer to the end of the
// arrow pointer.
MeshData CreateArrowMeshWithTint(float start_angle, float delta_angle,
                                 float line_length, float line_width,
                                 float line_offset, float pointer_height,
                                 float pointer_length, Color4ub start_tint,
                                 Color4ub end_tint);

}  // namespace lull

#endif  // LULLABY_UTIL_MESH_UTIL_H_
