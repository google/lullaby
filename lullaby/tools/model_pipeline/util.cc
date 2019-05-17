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

#include "lullaby/tools/model_pipeline/util.h"

#include <fstream>
#include <limits.h>
#include "lullaby/util/filename.h"
#include "lullaby/util/math.h"

namespace lull {
namespace tool {

std::string GenerateUniqueName(const std::string& src) {
  // TODO: Actually generate a unique name.
  return RemoveDirectoryAndExtensionFromFilename(src);
}

uint8_t CompactBoneIndex(int index) {
  if (index != Bone::kInvalidBoneIndex) {
    CHECK_LT(index, kInvalidBoneIdx) << "Bone index out of range.";
    return static_cast<uint8_t>(index);
  } else {
    return kInvalidBoneIdx;
  }
}

std::vector<Vertex::Influence> NormalizeInfluences(
    std::vector<Vertex::Influence> influences, int number_of_influences) {
  std::sort(influences.rbegin(), influences.rend());
  influences.resize(number_of_influences);
  float total = 0.f;
  for (const Vertex::Influence& influences : influences) {
    total += influences.weight;
  }
  if (total != 0.f) {
    const float scale = 1.f / total;
    for (Vertex::Influence& influences : influences) {
      influences.weight *= scale;
    }
  }
  return influences;
}

std::vector<std::string> GatherBoneNames(const std::vector<Bone>& bones) {
  std::vector<std::string> names;
  names.reserve(bones.size());
  for (const Bone& bone : bones) {
    names.push_back(bone.name);
  }
  return names;
}

std::vector<uint8_t> GatherParentBoneIndices(const std::vector<Bone>& bones) {
  std::vector<uint8_t> parents;
  parents.reserve(bones.size());
  for (const Bone& bone : bones) {
    parents.push_back(CompactBoneIndex(bone.parent_bone_index));
  }
  return parents;
}

std::vector<mathfu::AffineTransform> GatherBoneTransforms(
    const std::vector<Bone>& bones) {
  std::vector<mathfu::AffineTransform> matrices;
  for (const Bone& bone : bones) {
    const mathfu::mat4& m = bone.inverse_bind_transform;
    matrices.push_back(mathfu::mat4::ToAffineTransform(m));
  }
  return matrices;
}

void GatherBoneIndexMaps(const std::vector<Bone>& bones,
                         const std::vector<Vertex>& vertices,
                         std::vector<uint8_t>* mesh_to_shader_bones,
                         std::vector<uint8_t>* shader_to_mesh_bones) {
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
                       const std::vector<uint8_t>& mesh_to_shader_bones,
                       uint8_t* indices, uint8_t* weights,
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
      weights[i] = static_cast<uint8_t>(dst_weight_rounded);
    } else {
      indices[i] = 0;
      weights[i] = 0;
    }
  }
}

mathfu::vec4 CalculateOrientation(const mathfu::vec3& normal,
                                  const mathfu::vec4& tangent) {
  const mathfu::vec3 n = normal.Normalized();
  const mathfu::vec3 t = tangent.xyz().Normalized();
  const mathfu::vec3 b = mathfu::vec3::CrossProduct(n, t).Normalized();
  const mathfu::mat3 m(t.x, t.y, t.z, b.x, b.y, b.z, n.x, n.y, n.z);
  mathfu::quat q = mathfu::quat::FromMatrix(m).Normalized();
  // Align the sign bit of the orientation scalar to our handedness.
  if (signbit(tangent.w) != signbit(q.scalar())) {
    q = mathfu::quat(-q.scalar(), -q.vector());
  }
  return mathfu::vec4(q.vector(), q.scalar());
}

mathfu::vec4 CalculateOrientationNonZeroW(const mathfu::vec3& normal,
                                          const mathfu::vec4& tangent) {
  mathfu::vec3 bitangent = mathfu::vec3::CrossProduct(normal, tangent.xyz());
  mathfu::mat3 orientation_matrix(tangent.x, tangent.y, tangent.z, bitangent.x,
                                  bitangent.y, bitangent.z, normal.x, normal.y,
                                  normal.z);
  mathfu::quat orientation_quaternion =
      mathfu::quat::FromMatrix(orientation_matrix).Normalized();

  if (orientation_quaternion.scalar() < 0) {
    orientation_quaternion = mathfu::quat(-orientation_quaternion.scalar(),
                                          -orientation_quaternion.vector());
  }

  // Ensures w is never 0. sizeof(int16_t) ensures that the bias can be
  // represented with a normalized short.
  static constexpr float kBias =
      1.0f / static_cast<float>((1u << (sizeof(int16_t) * CHAR_BIT - 1u)) - 1u);
  if (orientation_quaternion.scalar() < kBias) {
    orientation_quaternion.set_scalar(kBias);

    // Renormalizes the orientation_quaternion.
    auto factor = (float)std::sqrt(1.0 - (double)kBias * (double)kBias);
    orientation_quaternion.set_vector(orientation_quaternion.vector() * factor);
  }

  // Makes w negative if there's a reflection.
  if (std::signbit(tangent.w)) {
    orientation_quaternion = mathfu::quat(-orientation_quaternion.scalar(),
                                          -orientation_quaternion.vector());
  }

  return mathfu::vec4(orientation_quaternion.vector(),
                      orientation_quaternion.scalar());
}

}  // namespace tool
}  // namespace lull
