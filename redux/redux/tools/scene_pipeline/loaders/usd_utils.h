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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_LOADERS_USD_UTILS_H_
#define REDUX_TOOLS_SCENE_PIPELINE_LOADERS_USD_UTILS_H_

#include <optional>
#include <string>

#include "redux/tools/scene_pipeline/sampler.h"
#include "redux/tools/scene_pipeline/types.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/shader.h"

namespace redux::tool {

inline static const pxr::TfToken kComponent{"component"};
inline static const pxr::TfToken kDiffuseColor{"diffuseColor"};
inline static const pxr::TfToken kEmissiveColor{"emissiveColor"};
inline static const pxr::TfToken kInputsBias{"inputs:bias"};
inline static const pxr::TfToken kInputsFile{"inputs:file"};
inline static const pxr::TfToken kInputsScale{"inputs:scale"};
inline static const pxr::TfToken kInputsWrapS{"inputs:wrapS"};
inline static const pxr::TfToken kInputsWrapT{"inputs:wrapT"};
inline static const pxr::TfToken kMaterialBinding{"material:binding"};
inline static const pxr::TfToken kMetallic{"metallic"};
inline static const pxr::TfToken kNormal{"normal"};
inline static const pxr::TfToken kOcclusion{"occlusion"};
inline static const pxr::TfToken kRoughness{"roughness"};
inline static const pxr::TfToken kSt0{"st0"};
inline static const pxr::TfToken kTangents{"tangents"};
inline static const pxr::TfToken kUsdPreviewSurface{"UsdPreviewSurface"};
inline static const pxr::TfToken kUsdPrimvarReaderFloat2{
    "UsdPrimvarReader_float2"};
inline static const pxr::TfToken kUsdUVTexture{"UsdUVTexture"};

// Returns true if the given prim is a component kind.
bool IsComponentKind(pxr::UsdPrim prim);

// Converts USD types into their redux::tool equivalents.
float3 ToFloat3(const pxr::GfVec3f& vec);
float4 ToFloat4(const pxr::GfVec4f& vec);
float4x4 ToFloat4x4(const pxr::GfMatrix4f& mat);
std::optional<float3> ToFloat3(const std::optional<pxr::GfVec3f>& vec);
std::optional<float4> ToFloat4(const std::optional<pxr::GfVec4f>& vec);

// Converts a USD wrap mode to a redux::tool Sampler::WrapMode.
Sampler::WrapMode ToWrapMode(const std::optional<pxr::TfToken>& mode);

// Returns the channels that are used for a texture source.
float4 GetChannelMask(const pxr::TfToken& source);

// Returns the name of the material bound to the given prim, or an empty string
// if no material is bound.
std::string ReadMaterialBinding(pxr::UsdPrim prim);

// Reads data from a USD GeomMesh prim.
pxr::VtArray<pxr::GfVec3f> ReadMeshPositions(const pxr::UsdGeomMesh& usd_mesh);
pxr::VtArray<pxr::GfVec3f> ReadMeshNormals(const pxr::UsdGeomMesh& usd_mesh);
pxr::VtArray<pxr::GfVec4f> ReadMeshTangents(const pxr::UsdGeomMesh& usd_mesh);
pxr::VtArray<pxr::GfVec2f> ReadMeshUvs(const pxr::UsdGeomMesh& usd_mesh);
pxr::VtArray<int> ReadMeshFaceCounts(const pxr::UsdGeomMesh& usd_mesh);
pxr::VtArray<int> ReadMeshFaceIndices(const pxr::UsdGeomMesh& usd_mesh);

// Reads data from a USD ShadeShader prim.
pxr::VtValue ReadShaderAttribute(const pxr::UsdShadeShader& usd_shader,
                                 const pxr::TfToken& name);

// Attempts to read an attribute from a USD ShadeShader prim as a specific type.
// Returns nullopt if the attribute is not found or is not of the given type.
template <typename T>
std::optional<T> ReadShaderAttributeAs(const pxr::UsdShadeShader& usd_shader,
                                       const pxr::TfToken& name) {
  pxr::VtValue value = ReadShaderAttribute(usd_shader, name);
  if (value.IsHolding<T>()) {
    return value.Get<T>();
  } else {
    return std::nullopt;
  }
}

// Attempts to read an attribute from a USD ShadeShader prim as a specific type.
// Returns the default_value if the attribute is not found or is not of the
// given type.
template <typename T>
T ReadShaderAttributeOr(const pxr::UsdShadeShader& usd_shader,
                        const pxr::TfToken& name, const T& default_value) {
  pxr::VtValue value = ReadShaderAttribute(usd_shader, name);
  if (value.IsHolding<T>()) {
    return value.Get<T>();
  } else {
    return default_value;
  }
}

// Attempts to read an input from a USD ShadeInput as a specific type. Returns
// nullopt if the input is not found or is not of the given type.
template <typename T>
std::optional<T> ReadInputAs(pxr::UsdShadeInput usd_input) {
  pxr::VtValue val;
  if (!usd_input.Get(&val)) {
    return std::nullopt;
  }
  if (val.IsHolding<T>()) {
    return val.Get<T>();
  }
  return std::nullopt;
}

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_LOADERS_USD_UTILS_H_
