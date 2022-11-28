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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_BONE_H_
#define REDUX_TOOLS_MODEL_PIPELINE_BONE_H_

#include <string>

#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"

namespace redux::tool {

// Information about a single bone in a skeletal hierarchy.
struct Bone {
  static constexpr int kInvalidBoneIndex = -1;

  Bone(std::string name, int parent_bone_index,
       const mat4& inverse_bind_transform)
      : name(std::move(name)),
        parent_bone_index(parent_bone_index),
        inverse_bind_transform(inverse_bind_transform) {}

  std::string name;
  int parent_bone_index;
  mat4 inverse_bind_transform;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_BONE_H_
