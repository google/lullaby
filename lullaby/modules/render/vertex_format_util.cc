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

#include "lullaby/modules/render/vertex_format_util.h"

namespace lull {

// Shader attribute hashes.
constexpr HashValue kAttributeHashPosition = ConstHash("ATTR_POSITION");
constexpr HashValue kAttributeHashUV = ConstHash("ATTR_UV");
constexpr HashValue kAttributeHashColor = ConstHash("ATTR_COLOR");
constexpr HashValue kAttributeHashNormal = ConstHash("ATTR_NORMAL");
constexpr HashValue kAttributeHashOrientation = ConstHash("ATTR_ORIENTATION");
constexpr HashValue kAttributeHashTangent = ConstHash("ATTR_TANGENT");
constexpr HashValue kAttributeHashBoneIndices = ConstHash("ATTR_BONE_INDICES");
constexpr HashValue kAttributeHashBoneWeights = ConstHash("ATTR_BONE_WEIGHTS");

// Shader feature hashes.
constexpr HashValue kFeatureHashTransform = ConstHash("Transform");
constexpr HashValue kFeatureHashVertexColor = ConstHash("VertexColor");
constexpr HashValue kFeatureHashTexture = ConstHash("Texture");
constexpr HashValue kFeatureHashTexture1 = ConstHash("Texture1");
constexpr HashValue kFeatureHashLight = ConstHash("Light");
constexpr HashValue kFeatureHashSkin = ConstHash("Skin");

void SetFeatureFlags(const VertexFormat& vertex_format,
                     std::set<HashValue>* flags) {
  int texture_count = 0;
  for (size_t i = 0; i < vertex_format.GetNumAttributes(); ++i) {
    const VertexAttribute* attrib = vertex_format.GetAttributeAt(i);
    switch (attrib->usage()) {
      case VertexAttributeUsage_Position:
        flags->insert(kFeatureHashTransform);
        break;
      default:
        break;
      case VertexAttributeUsage_TexCoord:
        flags->insert(texture_count ? kFeatureHashTexture1
                                    : kFeatureHashTexture);
        ++texture_count;
        break;
      case VertexAttributeUsage_Color:
        flags->insert(kFeatureHashVertexColor);
        break;
      case VertexAttributeUsage_Normal:
        flags->insert(kFeatureHashLight);
        break;
      case VertexAttributeUsage_BoneIndices:
        flags->insert(kFeatureHashSkin);
        break;
    }
  }
}

void SetEnvironmentFlags(const VertexFormat& vertex_format,
                         std::set<HashValue>* flags) {
  for (size_t i = 0; i < vertex_format.GetNumAttributes(); ++i) {
    const VertexAttribute* attrib = vertex_format.GetAttributeAt(i);
    switch (attrib->usage()) {
      case VertexAttributeUsage_Position:
        flags->insert(kAttributeHashPosition);
        break;
      case VertexAttributeUsage_TexCoord:
        flags->insert(kAttributeHashUV);
        break;
      case VertexAttributeUsage_Color:
        flags->insert(kAttributeHashColor);
        break;
      case VertexAttributeUsage_Normal:
        flags->insert(kAttributeHashNormal);
        break;
      case VertexAttributeUsage_Orientation:
        flags->insert(kAttributeHashOrientation);
        break;
      case VertexAttributeUsage_Tangent:
        flags->insert(kAttributeHashTangent);
        break;
      case VertexAttributeUsage_BoneIndices:
        flags->insert(kAttributeHashBoneIndices);
        break;
      case VertexAttributeUsage_BoneWeights:
        flags->insert(kAttributeHashBoneWeights);
        break;
      default:
        break;
    }
  }
}

}  // namespace lull
