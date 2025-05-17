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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_VERTEX_BUFFER_H_
#define REDUX_TOOLS_SCENE_PIPELINE_VERTEX_BUFFER_H_

#include <string>
#include <vector>

#include "redux/tools/scene_pipeline/buffer_view.h"
#include "redux/tools/scene_pipeline/type_id.h"
#include "redux/tools/scene_pipeline/types.h"

namespace redux::tool {

// Vertex data for a set of vertices.
struct VertexBuffer {
  struct Attribute {
    // The name of the attribute. (See names below for examples.)
    std::string name;

    // Index of attribute, for attributes with the name (e.g. "color").
    int index = 0;

    // The type of data stored in this attribute (e.g. float3).
    TypeId type = 0;

    // The data storing this attribute.
    BufferView buffer_view;

    // The number of bytes between each instance of this attribute within the
    // buffer. For non-interleaved data, this will be the same as the sizeof the
    // type.
    int stride = 0;
  };

  // The different attributes of vertex data represented by this buffer.
  std::vector<Attribute> attributes;

  // The total number of vertices represented by this buffer.
  int num_vertices = 0;

  // Common names for known vertex attributes. We do our best to map data from
  // scene formats to these names. Any name not prefixed with "$" is
  // considered format-specific.
  SCENE_NAME(kPosition, "$position");
  SCENE_NAME(kNormal, "$normal");
  SCENE_NAME(kBinormal, "$binormal");
  SCENE_NAME(kTangent, "$tangent");
  SCENE_NAME(kOrientation, "$orientation");
  SCENE_NAME(kColor, "$color");
  SCENE_NAME(kTexCoord, "$texcoord");
  SCENE_NAME(kBoneIndex, "$bone_index");
  SCENE_NAME(kBoneWeight, "$bone_weight");
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_VERTEX_BUFFER_H_
