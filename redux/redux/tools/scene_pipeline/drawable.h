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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_DRAWABLE_H_
#define REDUX_TOOLS_SCENE_PIPELINE_DRAWABLE_H_

#include "redux/tools/scene_pipeline/index.h"
#include "redux/tools/scene_pipeline/index_buffer.h"
#include "redux/tools/scene_pipeline/material.h"
#include "redux/tools/scene_pipeline/vertex_buffer.h"

namespace redux::tool {

// A single object that can be rendered.
struct Drawable {
  // Different ways to interpret a set of vertices to represent a single polygon
  // face of the object.
  enum class PrimitiveType {
    kUnspecified,
    kTriangleList,
    kTriangleStrip,
    kTriangleFan,
    kLineList,
    kLineStrip,
    kPointList,
  };

  // The vertex data for this drawable.
  VertexBuffer vertex_buffer;

  // Indices into the vertex buffer.
  IndexBuffer index_buffer;

  // How to interpret the indices to represent a face.
  PrimitiveType primitive_type = PrimitiveType::kUnspecified;

  // The material (shader) used for rendering.
  MaterialIndex material_index;

  // The offset within the index buffer from which to start rendering.
  int offset = 0;

  // The number of indices to render.
  int count = 0;
};

using DrawableIndex = Index<Drawable>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_DRAWABLE_H_
