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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_BONE_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_BONE_H_

#include <string>
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace tool {

// Information about a single bone in a skeletal hierarchy.
struct Bone {
  static const int kInvalidBoneIndex = -1;

  Bone(std::string name, int parent_bone_index,
       const mathfu::mat4& inverse_bind_transform)
      : name(std::move(name)),
        parent_bone_index(parent_bone_index),
        inverse_bind_transform(inverse_bind_transform) {}

  std::string name;
  int parent_bone_index;
  mathfu::mat4 inverse_bind_transform;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_BONE_H_
