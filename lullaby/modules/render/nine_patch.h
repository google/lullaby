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

#ifndef LULLABY_MODULES_RENDER_NINE_PATCH_H_
#define LULLABY_MODULES_RENDER_NINE_PATCH_H_

#include "lullaby/modules/render/mesh_data.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

struct NinePatch {
  mathfu::vec2 size = mathfu::kZeros2f;
  float left_slice = 0;
  float right_slice = 0;
  float bottom_slice = 0;
  float top_slice = 0;
  mathfu::vec2 original_size = mathfu::kZeros2f;
  mathfu::vec2i subdivisions = mathfu::vec2i(1);

  /// Returns the number of vertices that will be generated for this NinePatch.
  int GetVertexCount() const {
    // The + 1 is to add another row or column to complete the mesh, and the
    // + 2 is to add the extra subdivisions for the nine patch slices.
    return (subdivisions.x + 1 + 2) * (subdivisions.y + 1 + 2);
  }

  /// Returns the number of indices that will be generated for this NinePatch.
  int GetIndexCount() const {
    // + 2 for the slice rows and columns, * 3 for 3 vertices per triangle,
    // * 2 for 2 triangles per quad.
    return (subdivisions.x + 2) * (subdivisions.y + 2) * 3 * 2;
  }
};

/// Computes the |mesh| given the data in |nine_patch|.
void GenerateNinePatchMesh(const NinePatch& nine_patch, MeshData* mesh);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_NINE_PATCH_H_
