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

#include "redux/tools/scene_pipeline/loaders/usd_utils.h"

#include <optional>
#include <string>

#include "redux/tools/scene_pipeline/sampler.h"
#include "redux/tools/scene_pipeline/types.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdShade/shader.h"

namespace redux::tool {

bool IsComponentKind(pxr::UsdPrim prim) {
  pxr::TfToken kind;
  pxr::UsdModelAPI(prim).GetKind(&kind);
  return kind == kComponent;
}

float3 ToFloat3(const pxr::GfVec3f& vec) {
  return {vec[0], vec[1], vec[2]};
}

float4 ToFloat4(const pxr::GfVec4f& vec) {
  return {vec[0], vec[1], vec[2], vec[3]};
}

float4x4 ToFloat4x4(const pxr::GfMatrix4f& mat) {
  return float4x4(mat.data());
}

std::optional<float3> ToFloat3(const std::optional<pxr::GfVec3f>& vec) {
  using OptT = std::optional<float3>;
  return vec ? OptT(ToFloat3(vec.value())) : OptT();
}

std::optional<float4> ToFloat4(const std::optional<pxr::GfVec4f>& vec) {
  using OptT = std::optional<float4>;
  return vec ? OptT(ToFloat4(vec.value())) : OptT();
}

pxr::VtArray<pxr::GfVec3f> ReadMeshPositions(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<pxr::GfVec3f> points;
  usd_mesh.GetPointsAttr().Get(&points);
  return points;
}

pxr::VtArray<pxr::GfVec3f> ReadMeshNormals(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<pxr::GfVec3f> normals;
  usd_mesh.GetNormalsAttr().Get(&normals);
  return normals;
}

pxr::VtArray<pxr::GfVec4f> ReadMeshTangents(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<pxr::GfVec4f> tangents;
  pxr::UsdGeomPrimvarsAPI(usd_mesh).GetPrimvar(kTangents).Get(&tangents);
  return tangents;
}

pxr::VtArray<pxr::GfVec2f> ReadMeshUvs(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<pxr::GfVec2f> uvs;
  pxr::UsdGeomPrimvarsAPI(usd_mesh).GetPrimvar(kSt0).Get(&uvs);
  return uvs;
}

pxr::VtArray<int> ReadMeshFaceCounts(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<int> face_vertex_counts;
  CHECK(usd_mesh.GetFaceVertexCountsAttr().Get(&face_vertex_counts));
  return face_vertex_counts;
}

pxr::VtArray<int> ReadMeshFaceIndices(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<int> face_vertex_indices;
  CHECK(usd_mesh.GetFaceVertexIndicesAttr().Get(&face_vertex_indices));
  return face_vertex_indices;
}

pxr::VtValue ReadShaderAttribute(const pxr::UsdShadeShader& usd_shader,
                                  const pxr::TfToken& name) {
  const auto& attrib = usd_shader.GetPrim().GetAttribute(name);
  pxr::VtValue value;
  attrib.Get(&value);
  return value;
}

std::string ReadMaterialBinding(pxr::UsdPrim prim) {
  pxr::UsdProperty binding = prim.GetProperty(kMaterialBinding);
  if (!binding.Is<pxr::UsdRelationship>()) {
    return "";
  }

  pxr::SdfPathVector targets;
  binding.As<pxr::UsdRelationship>().GetTargets(&targets);
  return targets.front().GetString();
}

Sampler::WrapMode ToWrapMode(const std::optional<pxr::TfToken>& mode) {
  if (!mode.has_value()) {
    return Sampler::WrapMode::kClampToEdge;
  }

  if (*mode == "" || *mode == "clamp") {
    return Sampler::WrapMode::kClampToEdge;
  } else if (*mode == "mirror") {
    return Sampler::WrapMode::kMirroredRepeat;
  } else if (*mode == "repeat") {
    return Sampler::WrapMode::kRepeat;
  } else {
    LOG(ERROR) << "Unknown texture wrap mode: " << mode->GetString();
    return Sampler::WrapMode::kUnspecified;
  }
}

float4 GetChannelMask(const pxr::TfToken& source) {
  if (source == "r") {
    return Sampler::kRedMask;
  } else if (source == "g") {
    return Sampler::kGreenMask;
  } else if (source == "b") {
    return Sampler::kBlueMask;
  } else if (source == "a") {
    return Sampler::kAlphaMask;
  } else if (source == "rgb") {
    return Sampler::kRgbaMask;
  } else {
    LOG(ERROR) << "Unknown channel source: " << source.GetString();
    return Sampler::kRgbaMask;
  }
}

}  // namespace redux::tool
