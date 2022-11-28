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

#include "redux/tools/model_pipeline/util.h"

#include <limits.h>

#include <fstream>

#include "absl/types/span.h"
#include "redux/modules/base/filepath.h"
#include "redux/modules/math/quaternion.h"

namespace redux::tool {

bool IsValidMesh(const ModelConfig& config, std::string_view name) {
  const auto targets = config.target_meshes();
  if (targets == nullptr || targets->size() == 0) {
    // No filtered targets specified, so all meshes are valid.
    return true;
  }

  for (const auto target : *targets) {
    if (name.compare(target->c_str())) {
      return true;
    }
  }
  return false;
}

std::string GenerateUniqueName(const std::string& src) {
  // TODO(b/69116339): Actually generate a unique name.
  return std::string(RemoveDirectoryAndExtension(src));
}

uint16_t CompactBoneIndex(int index) {
  if (index != Bone::kInvalidBoneIndex) {
    CHECK_LT(index, std::numeric_limits<uint16_t>::max())
        << "Bone index out of range.";
    return static_cast<uint16_t>(index);
  } else {
    return std::numeric_limits<uint16_t>::max();
  }
}

std::vector<Vertex::Influence> NormalizeInfluences(
    std::vector<Vertex::Influence> influences, int number_of_influences) {
  std::sort(influences.rbegin(), influences.rend());
  influences.resize(number_of_influences);
  float total = 0.0f;
  for (const Vertex::Influence& influences : influences) {
    total += influences.weight;
  }
  if (total != 0.0f) {
    const float scale = 1.0f / total;
    for (Vertex::Influence& influences : influences) {
      influences.weight *= scale;
    }
  }
  return influences;
}

void GatherBoneIndexMaps(const std::vector<Bone>& bones,
                         const std::vector<Vertex>& vertices,
                         std::vector<uint16_t>* mesh_to_shader_bones,
                         std::vector<uint16_t>* shader_to_mesh_bones) {
  const size_t num_bones = bones.size();

  std::vector<bool> used_bone_flags(num_bones);
  for (const Vertex& vertex : vertices) {
    auto influences = NormalizeInfluences(vertex.influences);
    for (const Vertex::Influence& influence : influences) {
      if (influence.bone_index != Bone::kInvalidBoneIndex) {
        used_bone_flags[influence.bone_index] = true;
      }
    }
  }

  // Only bones that have vertices weighted to them are uploaded to the shader.
  int shader_bone_index = 0;
  mesh_to_shader_bones->reserve(num_bones);
  shader_to_mesh_bones->reserve(num_bones);
  for (size_t bone_index = 0; bone_index < num_bones; ++bone_index) {
    if (used_bone_flags[bone_index]) {
      mesh_to_shader_bones->push_back(CompactBoneIndex(shader_bone_index));
      shader_to_mesh_bones->push_back(CompactBoneIndex(bone_index));
      shader_bone_index++;
    } else {
      mesh_to_shader_bones->push_back(
          CompactBoneIndex(Bone::kInvalidBoneIndex));
    }
  }
}

void CompactInfluences(const std::vector<Vertex::Influence>& influences,
                       const std::vector<uint16_t>& mesh_to_shader_bones,
                       uint16_t* indices, float* weights,
                       int number_of_influences) {
  unsigned int dst_weight_remain = 0xff;
  const float src_to_dst_scale = static_cast<float>(dst_weight_remain);
  auto normalized_influences =
      NormalizeInfluences(influences, number_of_influences);
  for (int i = 0; i < number_of_influences; ++i) {
    const int mesh_index = normalized_influences[i].bone_index;
    if (mesh_index != Bone::kInvalidBoneIndex) {
      const int shader_index = mesh_to_shader_bones[mesh_index];
      const float src_weight = normalized_influences[i].weight;
      const float dst_weight = src_weight * src_to_dst_scale;
      const unsigned int dst_weight_rounded = std::min(
          static_cast<unsigned int>(dst_weight + 0.5f), dst_weight_remain);
      dst_weight_remain -= dst_weight_rounded;

      indices[i] = CompactBoneIndex(shader_index);
      weights[i] = static_cast<uint8_t>(dst_weight_rounded) / 255.0f;
    } else {
      indices[i] = 0;
      weights[i] = 0;
    }
  }
}

vec4 CalculateOrientation(const vec3& normal, const vec4& tangent) {
  const vec3 n = normal.Normalized();
  const vec3 t = tangent.xyz().Normalized();
  const vec3 b = Cross(n, t).Normalized();
  const mat3 m(t.x, b.x, n.x, t.y, b.y, n.y, t.z, b.z, n.z);
  quat q = QuaternionFromRotationMatrix(m).Normalized();
  // Align the sign bit of the orientation scalar to our handedness.
  if (std::signbit(tangent.w) != std::signbit(q.w)) {
    q = quat(-q.xyz(), -q.w);
  }
  return vec4(q.xyz(), q.w);
}

vec4 CalculateOrientationNonZeroW(const vec3& normal, const vec4& tangent) {
  vec3 bitangent = Cross(normal, tangent.xyz());
  mat3 orientation_matrix(tangent.x, bitangent.x, normal.x, tangent.y,
                          bitangent.y, normal.y, tangent.z, bitangent.z,
                          normal.z);
  quat orientation_quaternion =
      QuaternionFromRotationMatrix(orientation_matrix).Normalized();

  if (orientation_quaternion.w < 0) {
    orientation_quaternion =
        quat(-orientation_quaternion.xyz(), -orientation_quaternion.w);
  }

  // Ensures w is never 0. sizeof(int16_t) ensures that the bias can be
  // represented with a normalized short.
  static constexpr float kBias =
      1.0f / static_cast<float>((1u << (sizeof(int16_t) * CHAR_BIT - 1u)) - 1u);
  if (orientation_quaternion.w < kBias) {
    orientation_quaternion.w = kBias;

    // Renormalizes the orientation_quaternion.
    auto factor = (float)std::sqrt(1.0 - (double)kBias * (double)kBias);
    orientation_quaternion.x *= factor;
    orientation_quaternion.y *= factor;
    orientation_quaternion.z *= factor;
  }

  // Makes w negative if there's a reflection.
  if (std::signbit(tangent.w)) {
    orientation_quaternion =
        quat(-orientation_quaternion.xyz(), -orientation_quaternion.w);
  }

  return vec4(orientation_quaternion.xyz(), orientation_quaternion.w);
}

}  // namespace redux::tool
