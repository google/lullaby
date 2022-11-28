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

#ifndef REDUX_MODULES_GRAPHICS_ENUMS_H_
#define REDUX_MODULES_GRAPHICS_ENUMS_H_

#include <stddef.h>  // for types missing from the autogen file.
#include <stdint.h>  // for types missing from the autogen file.

#include "redux/modules/base/hash.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/graphics/graphics_enums_generated.h"

namespace redux {

#define REDUX_FBS_ENUM_TO_STRING(E) \
  inline const char* ToString(E e) { return EnumName##E(e); }

REDUX_FBS_ENUM_TO_STRING(ImageFormat)
REDUX_FBS_ENUM_TO_STRING(MaterialPropertyType)
REDUX_FBS_ENUM_TO_STRING(MaterialTextureType)
REDUX_FBS_ENUM_TO_STRING(MeshIndexType)
REDUX_FBS_ENUM_TO_STRING(MeshPrimitiveType)
REDUX_FBS_ENUM_TO_STRING(TextureFilter)
REDUX_FBS_ENUM_TO_STRING(TextureTarget)
REDUX_FBS_ENUM_TO_STRING(TextureWrap)
REDUX_FBS_ENUM_TO_STRING(VertexType)
REDUX_FBS_ENUM_TO_STRING(VertexUsage)

#undef REDUX_FBS_ENUM_TO_STRING

inline HashValue Hash(VertexUsage usage) {
  return HashValue(static_cast<int>(usage));
}

inline bool IsEnvironmentMaterialTextureType(MaterialTextureType type) {
  switch (type) {
    case MaterialTextureType::Unspecified:
    case MaterialTextureType::BaseColor:
    case MaterialTextureType::Normal:
    case MaterialTextureType::Metallic:
    case MaterialTextureType::Roughness:
    case MaterialTextureType::Occlusion:
    case MaterialTextureType::Emissive:
    case MaterialTextureType::Diffuse:
    case MaterialTextureType::Specular:
    case MaterialTextureType::Glossiness:
    case MaterialTextureType::Bump:
    case MaterialTextureType::Height:
    case MaterialTextureType::Light:
    case MaterialTextureType::Shadow:
    case MaterialTextureType::Glyph:
    case MaterialTextureType::Reflection:
      return false;
    case MaterialTextureType::EnvReflection:
    case MaterialTextureType::EnvIrradiance:
      return true;
  }
}

inline size_t MaterialPropertyTypeByteSize(MaterialPropertyType type) {
  switch (type) {
    case MaterialPropertyType::Feature:
      return sizeof(bool);
    case MaterialPropertyType::Boolean:
      return sizeof(bool);
    case MaterialPropertyType::Float1:
      return sizeof(float);
    case MaterialPropertyType::Float2:
      return 2 * sizeof(float);
    case MaterialPropertyType::Float3:
      return 3 * sizeof(float);
    case MaterialPropertyType::Float4:
      return 4 * sizeof(float);
    case MaterialPropertyType::Int1:
      return sizeof(int);
    case MaterialPropertyType::Int2:
      return 2 * sizeof(int);
    case MaterialPropertyType::Int3:
      return 3 * sizeof(int);
    case MaterialPropertyType::Int4:
      return 4 * sizeof(int);
    case MaterialPropertyType::Float2x2:
      return 4 * sizeof(float);
    case MaterialPropertyType::Float3x3:
      return 9 * sizeof(float);
    case MaterialPropertyType::Float4x4:
      return 16 * sizeof(float);
    case MaterialPropertyType::Sampler2D:
      return sizeof(int);
    case MaterialPropertyType::SamplerCubeMap:
      return sizeof(int);
    default:
      LOG(FATAL) << "Unknown type: " << ToString(type);
  }
}

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_ENUMS_H_
