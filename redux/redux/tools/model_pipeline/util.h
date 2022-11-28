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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_UTIL_H_
#define REDUX_TOOLS_MODEL_PIPELINE_UTIL_H_

#include "absl/types/span.h"
#include "redux/tools/model_pipeline/config_generated.h"
#include "redux/tools/model_pipeline/model.h"

namespace redux::tool {

// Returns true if the mesh with the given name is listed in the import options.
bool IsValidMesh(const ModelConfig& config, std::string_view name);

// Generates a unique name for a texture.
std::string GenerateUniqueName(const std::string& src);

// Converts the specified bone index into a smaller representation.
uint16_t CompactBoneIndex(int index);

// Returns mappings between mesh bones and shader bones.
void GatherBoneIndexMaps(const std::vector<Bone>& bones,
                         const std::vector<Vertex>& vertices,
                         std::vector<uint16_t>* mesh_to_shader_bones,
                         std::vector<uint16_t>* shader_to_mesh_bones);

// Returns a set of influences that sum up to 1.0. The number of influences
// returned will be capped at |number_of_influences|.
std::vector<Vertex::Influence> NormalizeInfluences(
    std::vector<Vertex::Influence> influences, int number_of_influences = 4);

// Converts the influences into a smaller data representation.
void CompactInfluences(const std::vector<Vertex::Influence>& influences,
                       const std::vector<uint16_t>& mesh_to_shader_bones,
                       uint16_t* indices, float* weights,
                       int number_of_influences = 4);

// Computes a quaternion given a normal and a tangent.  The tangent's 4th
// component represents handedness.  The input vectors do not have to be unit
// length.
vec4 CalculateOrientation(const vec3& normal, const vec4& tangent);

// Computes a quaternion given a normal and a tangent. The quaternion will not
// have w == 0 by introducing a bias to it.
vec4 CalculateOrientationNonZeroW(const vec3& normal, const vec4& tangent);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_UTIL_H_
