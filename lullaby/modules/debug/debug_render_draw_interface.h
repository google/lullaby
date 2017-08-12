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

#ifndef LULLABY_MODULES_DEBUG_RENDER_DRAW_INTERFACE_H_
#define LULLABY_MODULES_DEBUG_RENDER_DRAW_INTERFACE_H_

#include "lullaby/util/color.h"
#include "lullaby/util/math.h"

namespace lull {
namespace debug {

class DebugRenderDrawInterface {
 public:
  virtual ~DebugRenderDrawInterface() {}

  virtual void DrawLine(const mathfu::vec3& start_point,
                        const mathfu::vec3& end_point,
                        const Color4ub color) = 0;

  virtual void DrawText3D(const mathfu::vec3& pos, const Color4ub color,
                          const char* text) = 0;

  virtual void DrawText2D( const Color4ub color, const char* text) = 0;

  virtual void DrawBox3D(const mathfu::mat4& world_from_object_matrix,
                         const Aabb& box, Color4ub color) = 0;
};

}  // namespace debug
}  // namespace lull

#endif  // LULLABY_MODULES_DEBUG_RENDER_DRAW_INTERFACE_H_
