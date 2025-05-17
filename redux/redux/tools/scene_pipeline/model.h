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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_MODEL_H_
#define REDUX_TOOLS_SCENE_PIPELINE_MODEL_H_

#include <string>
#include <vector>

#include "redux/tools/scene_pipeline/collider.h"
#include "redux/tools/scene_pipeline/drawable.h"
#include "redux/tools/scene_pipeline/index.h"
#include "redux/tools/scene_pipeline/types.h"

namespace redux::tool {

// Represents a single object in the scene.
struct Model {
  // Models are stored as a tree of objects, where each node may contain objects
  // such as drawables or colliders.
  struct Node {
    // The name of the node.
    std::string name;

    // The local-space transform of the node (relative to its parent).
    float4x4 transform;

    // The list of drawables that are attached to this node.
    std::vector<DrawableIndex> drawable_indexes;

    // The list of colliders that are attached to this node.
    std::vector<ColliderIndex> collider_indexes;

    // The children nodes.
    std::vector<Node> children;
  };

  // The scene-space transform of the model.
  float4x4 transform;

  // The root node of the model.
  Node root_node;
};

using ModelIndex = Index<Model>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_MODEL_H_
