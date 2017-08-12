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

#ifndef LULLABY_MODULES_DEBUG_DEBUG_RENDER_H_
#define LULLABY_MODULES_DEBUG_DEBUG_RENDER_H_

#include <vector>

#include "lullaby/util/color.h"
#include "lullaby/util/math.h"

// DebugRender provides basic geometry drawing functionality for
// debugging purposes.

namespace lull {
namespace debug {

class DebugRenderDrawInterface;

// Initializes the debug render system to allow debug drawing.
void Initialize(DebugRenderDrawInterface* interface);

// Resets DebugRender interface to nullptr.
void Shutdown();

// Adds line between two given points to the queue.
void DrawLine(const char* tag_name, const mathfu::vec3& start_point,
              const mathfu::vec3& end_point, Color4ub color);

// Adds line connecting given points in sequence to the queue.
// Calls DrawLine for the number of given points.
void DrawLineStrip(const char* tag_name,
                   const std::vector<mathfu::vec3>& points, Color4ub color);

// Adds an RGB transform frame using the given matrix.
void DrawTransformAxes(const char* tag_name,
                       const mathfu::mat4& world_from_object_matrix);

// Adds billboard text and its position to the render queue.
void DrawText3D(const char* tag_name, const mathfu::vec3& pos, Color4ub color,
                const char* text);

// Adds 2D text to render queue. Will be drawn in fixed screen space.
void DrawText2D(const char* tag_name, Color4ub color, const char* text);

// Adds a 3D box to the debug render queue.
void DrawBox3D(const char* tag_name,
               const mathfu::mat4& world_from_object_matrix, const Aabb& box,
               Color4ub color);

// Calls drawing for all enabled elements in element buffer. Must be callled in
// between Begin() and End() after main render_system->Render().
void Submit();

}  // namespace debug
}  // namespace lull

#endif  // LULLABY_MODULES_DEBUG_DEBUG_RENDER_H_
