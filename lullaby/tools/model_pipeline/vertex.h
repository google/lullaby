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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_VERTEX_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_VERTEX_H_

#include "lullaby/tools/model_pipeline/bone.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/util/bits.h"

namespace lull {
namespace tool {

// Information about a single vertex in a mesh.
struct Vertex {
  // Arbitrary limits on vertex attributes.  Increasing these numbers means
  // increasing the matching member variables and the matching list of vertex
  // attributes in this class.
  static const int kMaxUvs = 8;
  static const int kMaxColors = 4;

  using Attrib = Bits;
  static const Attrib kAttribBit_Position = 1 << 0;
  static const Attrib kAttribBit_Normal = 1 << 1;
  static const Attrib kAttribBit_Tangent = 1 << 2;
  static const Attrib kAttribBit_Bitangent = 1 << 3;
  static const Attrib kAttribBit_Orientation = 1 << 4;
  static const Attrib kAttribBit_Influences = 1 << 5;
  static const Attrib kAttribBit_Color0 = 1 << 6;
  static const Attrib kAttribBit_Color1 = 1 << 7;
  static const Attrib kAttribBit_Color2 = 1 << 8;
  static const Attrib kAttribBit_Color3 = 1 << 9;
  static const Attrib kAttribBit_Uv0 = 1 << 10;
  static const Attrib kAttribBit_Uv1 = 1 << 11;
  static const Attrib kAttribBit_Uv2 = 1 << 12;
  static const Attrib kAttribBit_Uv3 = 1 << 13;
  static const Attrib kAttribBit_Uv4 = 1 << 14;
  static const Attrib kAttribBit_Uv5 = 1 << 15;
  static const Attrib kAttribBit_Uv6 = 1 << 16;
  static const Attrib kAttribBit_Uv7 = 1 << 17;
  static const Attrib kAttribAllBits = 0xffffffff;

  // Weighted bone index that influences the final position of a vertex.
  struct Influence {
    Influence() {}
    Influence(int bone_index, float weight)
        : bone_index(bone_index), weight(weight) {}
    int bone_index = Bone::kInvalidBoneIndex;
    float weight = 0.f;

    bool operator==(const Influence& rhs) const {
      return bone_index == rhs.bone_index && weight == rhs.weight;
    }
    bool operator<(const Influence& rhs) const { return weight < rhs.weight; }
  };

  // Information about a blend shape for each vertex.
  struct Blend {
    std::string name;
    mathfu::vec3 position = {0, 0, 0};
    mathfu::vec3 normal = {0, 0, 0};
    mathfu::vec4 tangent = {0, 0, 0, 0};  // 4th element is handedness: +1 or -1
    mathfu::vec4 orientation = {0, 0, 0, 0};  // Sign of scalar is handedness.

    bool operator==(const Blend& rhs) const {
      return name == rhs.name && position == rhs.position &&
             normal == rhs.normal && tangent == rhs.tangent &&
             orientation == rhs.orientation;
    }
  };

  Vertex() {}

  // Compares all vertex elements so that vertices can be deduped.
  bool operator==(const Vertex& rhs) const {
    return position == rhs.position && normal == rhs.normal &&
           tangent == rhs.tangent && bitangent == rhs.bitangent &&
           orientation == rhs.orientation && color0 == rhs.color0 &&
           color1 == rhs.color1 && color2 == rhs.color2 &&
           color3 == rhs.color3 && uv0 == rhs.uv0 && uv1 == rhs.uv1 &&
           uv2 == rhs.uv2 && uv3 == rhs.uv3 && uv4 == rhs.uv4 &&
           uv5 == rhs.uv5 && uv6 == rhs.uv6 && uv7 == rhs.uv7 &&
           influences == rhs.influences && blends == rhs.blends;
  }

  mathfu::vec3 position = {0, 0, 0};
  mathfu::vec3 normal = {0, 0, 0};
  mathfu::vec4 tangent = {0, 0, 0, 0};  // 4th element is handedness: +1 or -1
  mathfu::vec3 bitangent = {0, 0, 0};
  mathfu::vec4 orientation = {0, 0, 0, 0};  // Sign of scalar is handedness.
  mathfu::vec4 color0 = {0, 0, 0, 0};
  mathfu::vec4 color1 = {0, 0, 0, 0};
  mathfu::vec4 color2 = {0, 0, 0, 0};
  mathfu::vec4 color3 = {0, 0, 0, 0};
  mathfu::vec2 uv0 = {0, 0};
  mathfu::vec2 uv1 = {0, 0};
  mathfu::vec2 uv2 = {0, 0};
  mathfu::vec2 uv3 = {0, 0};
  mathfu::vec2 uv4 = {0, 0};
  mathfu::vec2 uv5 = {0, 0};
  mathfu::vec2 uv6 = {0, 0};
  mathfu::vec2 uv7 = {0, 0};
  std::vector<Influence> influences;
  std::vector<Blend> blends;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_VERTEX_H_
