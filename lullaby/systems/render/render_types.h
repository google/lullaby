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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_TYPES_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_TYPES_H_

#include "lullaby/modules/render/quad_util.h"
#include "lullaby/systems/render/detail/sort_order_types.h"
#include "lullaby/util/bits.h"
#include "lullaby/generated/render_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

enum class RenderStencilMode {
  kDisabled,
  kTest,
  kWrite,
};

enum class RenderCullMode {
  kNone,
  kVisibleInAnyView,
};

enum class RenderFrontFace {
  kClockwise,
  kCounterClockwise,
};

/// Structure containing the parameters used for clearing the render target.
struct RenderClearParams {
  enum ClearOptions { kColor = 1 << 0, kDepth = 1 << 1, kStencil = 1 << 2 };

  /// Bits representing the selected clear options. 0 = no clear operations.
  Bits clear_options = 0;
  /// Color value to clear the color buffer to (if selected).
  mathfu::vec4 color_value = mathfu::kZeros4f;
  /// Depth value the depth buffer will be set upon clear (if selected).
  float depth_value = 1.0f;
  /// Stencil index used to clear the stencil buffer (if selected).
  int stencil_value = 0;
};


struct RenderQuad {
  RenderQuad() {}
  HashValue id = 0;
  mathfu::vec2 size = {0, 0};
  mathfu::vec2i verts = {2, 2};
  float corner_radius = 0.f;
  int corner_verts = 0;
  bool has_uv = false;
  CornerMask corner_mask = CornerMask::kAll;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_TYPES_H_
