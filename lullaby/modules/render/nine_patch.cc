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

#include "lullaby/modules/render/nine_patch.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {
inline float SafeDiv(float num, float den, float otherwise = 0.f) {
  return den != 0.f ? num / den : otherwise;
}
}  // namespace

void NinePatchFromDef(const NinePatchDef* def, NinePatch* nine_patch) {
  MathfuVec2FromFbVec2(def->size(), &nine_patch->size);
  nine_patch->left_slice = def->left_slice();
  nine_patch->right_slice = def->right_slice();
  nine_patch->bottom_slice = def->bottom_slice();
  nine_patch->top_slice = def->top_slice();
  MathfuVec2FromFbVec2(def->original_size(), &nine_patch->original_size);
  if (def->subdivisions()) {
    MathfuVec2iFromFbVec2i(def->subdivisions(), &nine_patch->subdivisions);
    DCHECK_GE(nine_patch->subdivisions.x, 1);
    DCHECK_GE(nine_patch->subdivisions.y, 1);
  }
  MathfuVec2FromFbVec2(def->texture_alt_min(), &nine_patch->texture_alt_min);
  MathfuVec2FromFbVec2(def->texture_alt_max(), &nine_patch->texture_alt_max);
}


// Generates vertex data for a nine patch.
// This mesh is a tessellated quad mesh with specific vertices placed on the
// nine patch slices to achieve standard nine patch resizing behavior.  If
// desired, extra vertices will be added to the mesh at regular subdivisions to
// allow the mesh to be deformed.  As the name suggests, there are nine distinct
// regions in the mesh.  The four corner patches do not stretch as the nine
// patch changes size, the top, bottom, left, and right patches stretch in only
// one dimension, and the middle stretches in both dimensions.

// Computes the position and texture coordinate for one dimension (the math
// works for both x and y).  The resulting spatial position and texture
// coordinate are written into p and u respectively.  For a single dimension of
// the nine patch, there are five cases a vertex can fall into, hence the five
// if-cases.
void ComputeVertexValues(float size, float original_size, float low_slice,
                         int low_slice_index, float low_patch_width,
                         float high_slice, int high_slice_index,
                         float high_patch_width, float middle_patch_size,
                         float middle_patch_uv_size, int vertex_index,
                         float vertex_interval, float* p, float* u) {
  // Set vertex x and u0.
  if (vertex_index < low_slice_index) {
    // Low patch.
    // Position is unchanged from the interval position.
    // Texture coordinate is computed by figuring out the fraction of the patch
    // this interval is at, and multiplying that by the original slice UV width.
    *p = vertex_interval;
    *u = SafeDiv(low_slice * vertex_interval, low_patch_width);
  } else if (vertex_index == low_slice_index) {
    // Low slice column.
    // Position is the original size scaled by the low slice UV width.
    // Texture coordinate is simply the slice UV width.
    *p = low_patch_width;
    *u = low_slice;
  } else if (vertex_index < high_slice_index) {
    // Middle patch.
    // Position is unchanged from the interval position.
    // Texture coordinate is the middle patch's UV size scaled by the fraction
    // of the middle patch this interval is at, plus the UV width of the low
    // slice.
    *p = vertex_interval;
    const float distance_in_middle_patch = vertex_interval - low_patch_width;
    *u = SafeDiv(middle_patch_uv_size * distance_in_middle_patch,
                  middle_patch_size) +
         low_slice;
  } else if (vertex_index == high_slice_index) {
    // High slice column.
    // Position is the full size minus the size of the slice.
    // Texture coordinate is the whole texture (1.0f) minus this slice's
    // UV width.
    *p = size - high_patch_width;
    *u = 1.0f - high_slice;
  } else {
    // High patch.
    // Position is unchanged from the interval position.
    // Texture coordinate is the end of this patch's UV minus the UV fraction
    // from the edge.
    // This counts backwards so that it is symmetrical to low_slice and matches
    // the way high_slice is measured, i.e. distance from edge.
    *p = vertex_interval;
    const float distance_in_high_patch_from_edge = size - vertex_interval;
    *u = 1.0f - SafeDiv(high_slice * distance_in_high_patch_from_edge,
                         high_patch_width);
  }
}

void GenerateNinePatchMesh(const NinePatch& nine_patch, MeshData* mesh) {
  const mathfu::vec2 half_size = nine_patch.size * .5f;

  // The + 2 + 1 here is to add 2 extra rows/columns for the slices and one more
  // to complete the edges of the quad mesh.
  const int col_vert_count = nine_patch.subdivisions.x + 2 + 1;
  const int row_vert_count = nine_patch.subdivisions.y + 2 + 1;

  const float x_step =
      nine_patch.size.x / static_cast<float>(nine_patch.subdivisions.x);
  const float y_step =
      nine_patch.size.y / static_cast<float>(nine_patch.subdivisions.y);

  const mathfu::vec2 middle_patch_uv_size(
      1.0f - nine_patch.right_slice - nine_patch.left_slice,
      1.0f - nine_patch.top_slice - nine_patch.bottom_slice);

  // Ratio of slice sizes to determine maximum slice size.
  const float slice_width_sum = nine_patch.left_slice + nine_patch.right_slice;
  const float slice_height_sum = nine_patch.bottom_slice + nine_patch.top_slice;
  const float slice_width_center =
      SafeDiv(nine_patch.left_slice, slice_width_sum, 0.5f);
  const float slice_height_center =
      SafeDiv(nine_patch.bottom_slice, slice_height_sum, 0.5f);

  // Make sure that the final positions are always within the final size,
  // even if the original_size was larger.
  const float left_patch_width = std::min(
      nine_patch.size.x * slice_width_center,
      nine_patch.original_size.x * nine_patch.left_slice);
  const float right_patch_width = std::min(
      nine_patch.size.x * (1.0f - slice_width_center),
      nine_patch.original_size.x * nine_patch.right_slice);
  const float bottom_patch_width = std::min(
      nine_patch.size.y * slice_height_center,
      nine_patch.original_size.y * nine_patch.bottom_slice);
  const float top_patch_width = std::min(
      nine_patch.size.y * (1.0f - slice_height_center),
      nine_patch.original_size.y * nine_patch.top_slice);
  const mathfu::vec2 middle_patch_size =
      nine_patch.size - mathfu::vec2(left_patch_width + right_patch_width,
                                     bottom_patch_width + top_patch_width);

  const float indices_per_width = SafeDiv(
      static_cast<float>(nine_patch.subdivisions.x), nine_patch.size.x);
  const float indices_per_height = SafeDiv(
      static_cast<float>(nine_patch.subdivisions.y), nine_patch.size.y);
  // The 1 + and 2 + here account for the extra vertices added for the slices.
  // The min accounts for the extra vertices counting from the back.
  const int left_slice_index =
      std::min(1 + static_cast<int>(indices_per_width * left_patch_width),
               row_vert_count - 3);
  const int right_slice_index =
      std::min(2 + static_cast<int>(indices_per_width *
                                    (left_patch_width + middle_patch_size.x)),
               row_vert_count - 2);
  const int bottom_slice_index =
      std::min(1 + static_cast<int>(indices_per_height * bottom_patch_width),
               col_vert_count - 3);
  const int top_slice_index =
      std::min(2 + static_cast<int>(indices_per_height *
                                    (bottom_patch_width + middle_patch_size.y)),
               col_vert_count - 2);

  // Save the current number of vertices to use later as a base index during
  // index generation.  This allows a nine patch mesh to be tacked on to the end
  // of an existing mesh.
  const uint32_t num_verts = static_cast<uint32_t>(mesh->GetNumVertices());

  // Now generate the mesh.  It is nothing more than a tessellated quad with
  // some fancy positioning of vertices and UVs.
  float interval_y = 0.0f;
  for (int y_index = 0; y_index < row_vert_count; ++y_index) {
    float interval_x = 0.0f;

    float y, v0, v1;

    ComputeVertexValues(nine_patch.size.y, nine_patch.original_size.y,
                        nine_patch.bottom_slice, bottom_slice_index,
                        bottom_patch_width, nine_patch.top_slice,
                        top_slice_index, top_patch_width, middle_patch_size.y,
                        middle_patch_uv_size.y, y_index, interval_y, &y, &v0);
    v1 = mathfu::Lerp(1.f - nine_patch.texture_alt_min.y,
                      1.f - nine_patch.texture_alt_max.y,
                      SafeDiv(y, nine_patch.size.y));

    if (y_index != bottom_slice_index && y_index != top_slice_index) {
      interval_y += y_step;
    }

    for (int x_index = 0; x_index < col_vert_count; ++x_index) {
      float x, u0, u1;

      ComputeVertexValues(
          nine_patch.size.x, nine_patch.original_size.x, nine_patch.left_slice,
          left_slice_index, left_patch_width, nine_patch.right_slice,
          right_slice_index, right_patch_width, middle_patch_size.x,
          middle_patch_uv_size.x, x_index, interval_x, &x, &u0);
      u1 = mathfu::Lerp(nine_patch.texture_alt_min.x,
                        nine_patch.texture_alt_max.x,
                        SafeDiv(x, nine_patch.size.x));

      if (x_index != left_slice_index && x_index != right_slice_index) {
        interval_x += x_step;
      }

      mesh->AddVertex<VertexPTT>(x - half_size.x, y - half_size.y, 0.0f, u0,
                                 1.0f - v0, u1, v1);
    }
  }

  // Generate indices for the nine-patch mesh.
  for (int y_index = 1; y_index < row_vert_count; ++y_index) {
    const int row = col_vert_count * y_index;
    const int last_row = row - col_vert_count;

    for (int x_index = 1; x_index < col_vert_count; ++x_index) {
      mesh->AddIndex(num_verts + last_row + x_index - 1);
      mesh->AddIndex(num_verts + last_row + x_index);
      mesh->AddIndex(num_verts + row + x_index - 1);
      mesh->AddIndex(num_verts + last_row + x_index);
      mesh->AddIndex(num_verts + row + x_index);
      mesh->AddIndex(num_verts + row + x_index - 1);
    }
  }
}

}  // namespace lull
