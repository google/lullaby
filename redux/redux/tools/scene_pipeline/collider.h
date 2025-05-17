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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_COLLIDER_H_
#define REDUX_TOOLS_SCENE_PIPELINE_COLLIDER_H_

#include "redux/tools/scene_pipeline/buffer_view.h"
#include "redux/tools/scene_pipeline/index.h"

namespace redux::tool {

// A single object that can be used for rigid body collision detection.
struct Collider {
  enum class ColliderType {
    kUnspecified,
    kTriMesh,
  };

  // A triangle mesh.
  struct TriMesh {
    // A collection of float3 vertices.
    BufferView vertices;

    // A collection on int3 triangles. Each element in each triangle is an
    // index into the vertices buffer.
    BufferView triangles;
  };

  // The type of data stored in this collider.
  ColliderType collider_type = ColliderType::kUnspecified;

  // The collider is a triangle mesh.
  TriMesh tri_mesh;
};

using ColliderIndex = Index<Collider>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_COLLIDER_H_
