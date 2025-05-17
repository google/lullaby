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

#ifndef REDUX_MODULES_GRAPHICS_MESH_UTILS_H_
#define REDUX_MODULES_GRAPHICS_MESH_UTILS_H_

#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Returns the bounding box of a mesh using the vertices with Position
// attributes.
Box ComputeBounds(const MeshData& mesh);

// Generates a mesh that contains Orientation vertices using the data from the
// provided mesh.
MeshData ComputeOrientations(const MeshData& mesh);

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_MESH_UTILS_H_
