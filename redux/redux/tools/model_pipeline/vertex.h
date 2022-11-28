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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_VERTEX_H_
#define REDUX_TOOLS_MODEL_PIPELINE_VERTEX_H_

#include <vector>

#include "redux/modules/base/bits.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/math/vector.h"
#include "redux/tools/model_pipeline/bone.h"

namespace redux::tool {

// Information about a single vertex in a mesh.
struct Vertex {
  // Arbitrary limits on vertex attributes.  Increasing these numbers means
  // increasing the matching member variables and the matching list of vertex
  // attributes in this class.
  static constexpr int kMaxUvs = 8;
  static constexpr int kMaxColors = 4;

  using Attrib = Bits32;
  static constexpr Attrib kAttribBit_Position = Bits32::Nth<0>();
  static constexpr Attrib kAttribBit_Normal = Bits32::Nth<1>();
  static constexpr Attrib kAttribBit_Tangent = Bits32::Nth<2>();
  static constexpr Attrib kAttribBit_Bitangent = Bits32::Nth<3>();
  static constexpr Attrib kAttribBit_Orientation = Bits32::Nth<4>();
  static constexpr Attrib kAttribBit_Influences = Bits32::Nth<5>();
  static constexpr Attrib kAttribBit_Blends = Bits32::Nth<6>();
  static constexpr Attrib kAttribBit_Color0 = Bits32::Nth<7>();
  static constexpr Attrib kAttribBit_Color1 = Bits32::Nth<8>();
  static constexpr Attrib kAttribBit_Color2 = Bits32::Nth<9>();
  static constexpr Attrib kAttribBit_Color3 = Bits32::Nth<10>();
  static constexpr Attrib kAttribBit_Uv0 = Bits32::Nth<11>();
  static constexpr Attrib kAttribBit_Uv1 = Bits32::Nth<12>();
  static constexpr Attrib kAttribBit_Uv2 = Bits32::Nth<13>();
  static constexpr Attrib kAttribBit_Uv3 = Bits32::Nth<14>();
  static constexpr Attrib kAttribBit_Uv4 = Bits32::Nth<15>();
  static constexpr Attrib kAttribBit_Uv5 = Bits32::Nth<16>();
  static constexpr Attrib kAttribBit_Uv6 = Bits32::Nth<17>();
  static constexpr Attrib kAttribBit_Uv7 = Bits32::Nth<18>();
  static constexpr Attrib kAttribAllBits = Bits32::All();

  // Weighted bone index that influences the final position of a vertex.
  struct Influence {
    Influence() {}
    Influence(int bone_index, float weight)
        : bone_index(bone_index), weight(weight) {}

    int bone_index = Bone::kInvalidBoneIndex;
    float weight = 0.0f;

    bool operator==(const Influence& rhs) const {
      return bone_index == rhs.bone_index && weight == rhs.weight;
    }
    bool operator<(const Influence& rhs) const { return weight < rhs.weight; }
  };

  // Information about a blend shape for each vertex.
  struct Blend {
    std::string name;
    vec3 position = {0, 0, 0};
    vec3 normal = {0, 0, 0};
    vec4 tangent = {0, 0, 0, 0};      // 4th element is handedness: +1 or -1
    vec4 orientation = {0, 0, 0, 0};  // Sign of scalar is handedness.

    bool operator==(const Blend& rhs) const {
      return name == rhs.name && position == rhs.position &&
             normal == rhs.normal && tangent == rhs.tangent &&
             orientation == rhs.orientation;
    }

    // Since tangents are often not equal among different blendshapes of any
    // given single vertex, all numerical fields but tangents are compared to
    // determine compresibility of a Blend.
    bool IsCompressibleTo(const Vertex& rhs) const {
      return position == rhs.position && normal == rhs.normal &&
             orientation == rhs.orientation;
    }
  };

  Vertex() = default;

  // Compares all vertex elements so that vertices can be deduped.
  bool operator==(const Vertex& rhs) const {
    return attribs == rhs.attribs && position == rhs.position &&
           normal == rhs.normal && tangent == rhs.tangent &&
           bitangent == rhs.bitangent && orientation == rhs.orientation &&
           color0 == rhs.color0 && color1 == rhs.color1 &&
           color2 == rhs.color2 && color3 == rhs.color3 && uv0 == rhs.uv0 &&
           uv1 == rhs.uv1 && uv2 == rhs.uv2 && uv3 == rhs.uv3 &&
           uv4 == rhs.uv4 && uv5 == rhs.uv5 && uv6 == rhs.uv6 &&
           uv7 == rhs.uv7 && influences == rhs.influences &&
           blends == rhs.blends;
  }

  static Attrib BuildAttrib(std::string_view str);
  static Attrib BuildAttrib(absl::Span<const VertexUsage> usages);

  Attrib attribs;
  vec3 position = {0, 0, 0};
  vec3 normal = {0, 0, 0};
  vec4 tangent = {0, 0, 0, 0};  // 4th element is handedness: +1 or -1
  vec3 bitangent = {0, 0, 0};
  vec4 orientation = {0, 0, 0, 0};  // Sign of scalar is handedness.
  vec4 color0 = {0, 0, 0, 0};
  vec4 color1 = {0, 0, 0, 0};
  vec4 color2 = {0, 0, 0, 0};
  vec4 color3 = {0, 0, 0, 0};
  vec2 uv0 = {0, 0};
  vec2 uv1 = {0, 0};
  vec2 uv2 = {0, 0};
  vec2 uv3 = {0, 0};
  vec2 uv4 = {0, 0};
  vec2 uv5 = {0, 0};
  vec2 uv6 = {0, 0};
  vec2 uv7 = {0, 0};
  std::vector<Influence> influences;
  std::vector<Blend> blends;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_VERTEX_H_
