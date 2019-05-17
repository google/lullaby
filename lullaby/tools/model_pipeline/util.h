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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_UTIL_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_UTIL_H_

#include "lullaby/generated/common_generated.h"
#include "lullaby/tools/model_pipeline/model.h"

namespace lull {
namespace tool {

// Generates a unique name for a texture.
std::string GenerateUniqueName(const std::string& src);

// Converts the specified bone index into a smaller representation.
uint8_t CompactBoneIndex(int index);

// Returns the list of bone names from the set of bones.
std::vector<std::string> GatherBoneNames(const std::vector<Bone>& bones);

// Returns the list of bone indices from the set of bones.
std::vector<uint8_t> GatherParentBoneIndices(const std::vector<Bone>& bones);

// Returns the list of bone transforms from the set of bones.
std::vector<mathfu::AffineTransform> GatherBoneTransforms(
    const std::vector<Bone>& bones);

// Returns mappings between mesh bones and shader bones.
void GatherBoneIndexMaps(const std::vector<Bone>& bones,
                         const std::vector<Vertex>& vertices,
                         std::vector<uint8_t>* mesh_to_shader_bones,
                         std::vector<uint8_t>* shader_to_mesh_bones);

// Returns a set of influences that sum up to 1.0. The number of influences
// returned will be capped at |number_of_influences|.
std::vector<Vertex::Influence> NormalizeInfluences(
    std::vector<Vertex::Influence> influences, int number_of_influences = 4);

// Converts the influences into a smaller data representation.
void CompactInfluences(const std::vector<Vertex::Influence>& influences,
                       const std::vector<uint8_t>& mesh_to_shader_bones,
                       uint8_t* indices, uint8_t* weights,
                       int number_of_influences = 4);

// Computes a quaternion given a normal and a tangent.  The tangent's 4th
// component represents handedness.  The input vectors do not have to be unit
// length.
mathfu::vec4 CalculateOrientation(const mathfu::vec3& normal,
                                  const mathfu::vec4& tangent);

// Computes a quaternion given a normal and a tangent. The quaternion will not
// have w == 0 by introducing a bias to it.
mathfu::vec4 CalculateOrientationNonZeroW(const mathfu::vec3& normal,
                                          const mathfu::vec4& tangent);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_UTIL_H_
