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

#include "redux/tools/model_pipeline/vertex.h"

namespace redux::tool {

Vertex::Attrib Vertex::BuildAttrib(std::string_view str) {
  int color_count = 0;
  int uv_count = 0;

  Attrib attrib;
  for (char c : str) {
    switch (c) {
      case 'p':
        attrib.Set(kAttribBit_Position);
        break;
      case 'c':
        attrib.Set(kAttribBit_Color0.Value() << color_count);
        ++color_count;
        break;
      case 'u':
        attrib.Set(kAttribBit_Uv0.Value() << uv_count);
        ++uv_count;
        break;
      case 'n':
        attrib.Set(kAttribBit_Normal);
        break;
      case 't':
        attrib.Set(kAttribBit_Tangent);
        break;
      case 'q':
        attrib.Set(kAttribBit_Orientation);
        break;
      case 'b':
        attrib.Set(kAttribBit_Influences);
        break;
      default:
        LOG(FATAL) << "Unknown vertex usage: " << c;
    }
  }
  return attrib;
}

Vertex::Attrib Vertex::BuildAttrib(absl::Span<const VertexUsage> usages) {
  Attrib attrib;
  for (const VertexUsage usage : usages) {
    switch (usage) {
      case VertexUsage::Position:
        attrib.Set(kAttribBit_Position);
        break;
      case VertexUsage::Color0:
        attrib.Set(kAttribBit_Color0);
        break;
      case VertexUsage::Color1:
        attrib.Set(kAttribBit_Color1);
        break;
      case VertexUsage::Color2:
        attrib.Set(kAttribBit_Color2);
        break;
      case VertexUsage::Color3:
        attrib.Set(kAttribBit_Color3);
        break;
      case VertexUsage::TexCoord0:
        attrib.Set(kAttribBit_Uv0);
        break;
      case VertexUsage::TexCoord1:
        attrib.Set(kAttribBit_Uv1);
        break;
      case VertexUsage::TexCoord2:
        attrib.Set(kAttribBit_Uv2);
        break;
      case VertexUsage::TexCoord3:
        attrib.Set(kAttribBit_Uv3);
        break;
      case VertexUsage::TexCoord4:
        attrib.Set(kAttribBit_Uv4);
        break;
      case VertexUsage::TexCoord5:
        attrib.Set(kAttribBit_Uv5);
        break;
      case VertexUsage::TexCoord6:
        attrib.Set(kAttribBit_Uv6);
        break;
      case VertexUsage::TexCoord7:
        attrib.Set(kAttribBit_Uv7);
        break;
      case VertexUsage::Normal:
        attrib.Set(kAttribBit_Normal);
        break;
      case VertexUsage::Tangent:
        attrib.Set(kAttribBit_Tangent);
        break;
      case VertexUsage::Orientation:
        attrib.Set(kAttribBit_Orientation);
        break;
      case VertexUsage::BoneIndices:
        attrib.Set(kAttribBit_Influences);
        break;
      case VertexUsage::BoneWeights:
        attrib.Set(kAttribBit_Influences);
        break;
      default:
        LOG(FATAL) << "Unknown vertex usage: " << static_cast<int>(usage);
    }
  }
  return attrib;
}

}  // namespace redux::tool
