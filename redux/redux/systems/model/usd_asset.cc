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

#include "redux/systems/model/usd_asset.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/graphics/vertex_attribute.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/graphics/vertex_utils.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/vector.h"
#include "redux/modules/usd/usd_asset_resolver.h"
#include "redux/systems/model/model_asset.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/types.h"
#include "pxr/usd/usdShade/utils.h"
#include "redux/systems/model/model_asset_factory.h"

namespace redux {

static const TextureUsage kBaseColorTextureUsage(
    {MaterialTextureType::BaseColor});

static const TextureUsage kNormalTextureUsage({MaterialTextureType::Normal,
                                               MaterialTextureType::Normal,
                                               MaterialTextureType::Normal});

static const TextureUsage kMetallicRoughnessTextureUsage(
    {MaterialTextureType::Unspecified, MaterialTextureType::Roughness,
     MaterialTextureType::Metallic});

static pxr::VtArray<pxr::GfVec3f> ReadPositions(
    const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<pxr::GfVec3f> points;
  usd_mesh.GetPointsAttr().Get(&points);
  return points;
}

static pxr::VtArray<pxr::GfVec3f> ReadNormals(
    const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<pxr::GfVec3f> normals;
  usd_mesh.GetNormalsAttr().Get(&normals);
  return normals;
}

static pxr::VtArray<pxr::GfVec4f> ReadTangents(
    const pxr::UsdGeomMesh& usd_mesh) {
  static const pxr::TfToken kTangents = pxr::TfToken("tangents");
  pxr::VtArray<pxr::GfVec4f> tangents;
  pxr::UsdGeomPrimvarsAPI(usd_mesh).GetPrimvar(kTangents).Get(&tangents);
  return tangents;
}

static pxr::VtArray<pxr::GfVec2f> ReadUvs(const pxr::UsdGeomMesh& usd_mesh) {
  static const pxr::TfToken kSt = pxr::TfToken("st");
  pxr::VtArray<pxr::GfVec2f> uvs;
  pxr::UsdGeomPrimvarsAPI(usd_mesh).GetPrimvar(kSt).Get(&uvs);
  return uvs;
}

static pxr::VtArray<int> ReadFaceCounts(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<int> face_vertex_counts;
  CHECK(usd_mesh.GetFaceVertexCountsAttr().Get(&face_vertex_counts));
  return face_vertex_counts;
}

static pxr::VtArray<int> ReadFaceIndices(const pxr::UsdGeomMesh& usd_mesh) {
  pxr::VtArray<int> face_vertex_indices;
  CHECK(usd_mesh.GetFaceVertexIndicesAttr().Get(&face_vertex_indices));
  return face_vertex_indices;
}

static pxr::VtValue ReadAttribute(const pxr::UsdShadeShader& usd_shader,
                                  const pxr::TfToken& name) {
  const auto& attrib = usd_shader.GetPrim().GetAttribute(name);
  pxr::VtValue value;
  attrib.Get(&value);
  return value;
}

template <typename T>
static T ReadAttributeAs(const pxr::UsdShadeShader& usd_shader,
                         const pxr::TfToken& name) {
  return ReadAttribute(usd_shader, name).Get<T>();
}

void UsdAsset::ProcessData() {
  GetGlobalUsdAssetResolver()->RegisterAsset(uri_, asset_data_);

  stage_ = pxr::UsdStage::Open(uri_);
  stage_->Flatten(false);
  Traverse(stage_->GetPseudoRoot().GetChildren().front());

  materials_.reserve(mesh_materials_.size());
  for (const std::string& material : mesh_materials_) {
    const std::size_t material_index = material_lookup_[material];
    materials_.push_back(parsed_materials_[material_index]);
  }
}

void UsdAsset::Traverse(pxr::UsdPrim prim) {
  if (prim.IsA<pxr::UsdGeomScope>()) {
    // Ignore, this is just a grouping structure.
  } else if (prim.IsA<pxr::UsdGeomXform>()) {
    // Ignore, we'll use USD to generate the world-space matrix.
  } else if (prim.IsA<pxr::UsdGeomMesh>()) {
    pxr::UsdGeomMesh mesh(prim);
    ProcessMesh(mesh);
  } else if (prim.IsA<pxr::UsdGeomSubset>()) {
    pxr::UsdGeomSubset subset(prim);
    ProcessSubMesh(subset);
  } else if (prim.IsA<pxr::UsdShadeShader>()) {
    pxr::UsdShadeShader shader(prim);
    ProcessShader(shader);
  } else if (prim.IsA<pxr::UsdShadeMaterial>()) {
    pxr::UsdShadeMaterial material(prim);
    ProcessMaterial(material);
  } else {
    LOG(FATAL) << "Unknown prim type: " << prim.GetTypeName().GetString();
  }

  for (auto child : prim.GetChildren()) {
    Traverse(child);
  }
}

void UsdAsset::ProcessMaterial(pxr::UsdShadeMaterial usd_material) {
  const std::string& path = usd_material.GetPath().GetString();
  material_lookup_[path] = parsed_materials_.size();
  parsed_materials_.emplace_back();
}

ModelAsset::TextureData UsdAsset::ProcessTexture(
    pxr::UsdShadeShader usd_texture) {
  static const pxr::TfToken kInputsFile("inputs:file");
  auto uri = ReadAttributeAs<pxr::SdfAssetPath>(usd_texture, kInputsFile);

  TextureData texture_info;
  texture_info.uri = uri.GetResolvedPath();
  CHECK(!texture_info.uri.empty()) << "No uri found in texture.";
  return texture_info;
}



void UsdAsset::ProcessShader(pxr::UsdShadeShader usd_shader) {
  static const pxr::TfToken kUsdPreviewSurface("UsdPreviewSurface");
  static const pxr::TfToken kUsdUvTexture("UsdUVTexture");
  static const pxr::TfToken kDiffuseColor("diffuseColor");
  static const pxr::TfToken kNormal("normal");
  static const pxr::TfToken kOpacity("opacity");
  static const pxr::TfToken kMetallic("metallic");
  static const pxr::TfToken kRoughness("roughness");
  static const pxr::TfToken kInputsFile("inputs:file");

  MaterialData& material_data = parsed_materials_.back();

  pxr::TfToken id;
  usd_shader.GetShaderId(&id);
  // We only support USD Preview Surface shading models for now.
  if (id == kUsdPreviewSurface) {
    material_data.shading_model = "metallic_roughness";

    for (const pxr::UsdShadeInput& input : usd_shader.GetInputs()) {
      pxr::TfToken name = input.GetBaseName();
      pxr::UsdShadeConnectableAPI source;
      pxr::TfToken source_name;
      pxr::UsdShadeAttributeType source_type;
      input.GetConnectedSource(&source, &source_name, &source_type);

      pxr::UsdPrim source_prim = source.GetPrim();
      CHECK(source_prim.IsA<pxr::UsdShadeShader>());
      pxr::UsdShadeShader input_shader(source_prim);

      pxr::TfToken input_id;
      input_shader.GetShaderId(&input_id);

      if (input_id == kUsdUvTexture) {
        textures_[Hash(name.GetString())] = ProcessTexture(input_shader);
      }

      if (name == kDiffuseColor) {
        if (input_id == kUsdUvTexture) {
          material_data.textures.push_back(
              {kBaseColorTextureUsage, name.GetString()});
        } else {
          LOG(ERROR) << "Unimplemented.";
        }
      } else if (name == kNormal) {
        if (input_id == kUsdUvTexture) {
          material_data.textures.push_back(
              {kNormalTextureUsage, name.GetString()});
        } else {
          LOG(ERROR) << "Unimplemented.";
        }
      } else if (name == kMetallic) {
        if (input_id == kUsdUvTexture) {
          LOG(ERROR) << "Assuming roughness is packed with metallic texture.";
          material_data.textures.push_back(
              {kMetallicRoughnessTextureUsage, name.GetString()});
        } else {
          LOG(ERROR) << "Unimplemented.";
        }
      } else if (name == kRoughness) {
          LOG(ERROR) << "Unimplemented.";
      } else if (name == kOpacity) {
          LOG(ERROR) << "Unimplemented.";
      } else {
        LOG(FATAL) << "Unknown input type: " << name.GetString();
      }
    }
  }
}

void UsdAsset::ProcessMesh(pxr::UsdGeomMesh usd_mesh) {
  pxr::VtArray<int> face_vertex_counts = ReadFaceCounts(usd_mesh);
  pxr::VtArray<int> face_vertex_indices = ReadFaceIndices(usd_mesh);

  // Calculate the number of triangles/vertices in this mesh.
  std::size_t num_triangles = 0;
  for (int face_vertex_count : face_vertex_counts) {
    // For every face containing N vertices, we'll end up with N-2 triangles.
    num_triangles += face_vertex_count - 2;
  }
  const std::size_t num_vertices = num_triangles * 3;
  CHECK_GT(num_vertices, 0);

  pxr::VtArray<pxr::GfVec3f> positions = ReadPositions(usd_mesh);
  pxr::VtArray<pxr::GfVec3f> normals = ReadNormals(usd_mesh);
  pxr::VtArray<pxr::GfVec4f> tangents = ReadTangents(usd_mesh);
  pxr::VtArray<pxr::GfVec2f> uvs = ReadUvs(usd_mesh);

  VertexFormat vertex_format;
  std::size_t offset = 0;

  if (!positions.empty()) {
    const VertexAttribute attrib(VertexUsage::Position, VertexType::Vec3f);
    const std::size_t stride = VertexFormat::GetVertexTypeSize(attrib.type);
    vertex_format.AppendAttribute(attrib, offset, stride);
    offset += num_vertices * stride;
  }
  if (!normals.empty()) {
    // Note: we're targeting a rendering backend that only supports orientation
    // attributes. We will use the normal data (and potentially tangent data)
    // to dynamically calculate the orientations below.
    const VertexAttribute attrib(VertexUsage::Orientation, VertexType::Vec4f);
    const std::size_t stride = VertexFormat::GetVertexTypeSize(attrib.type);
    vertex_format.AppendAttribute(attrib, offset, stride);
    CHECK_EQ(normals.size(), num_vertices);
    offset += num_vertices * stride;
  }
  if (!uvs.empty()) {
    const VertexAttribute attrib(VertexUsage::TexCoord0, VertexType::Vec2f);
    const std::size_t stride = VertexFormat::GetVertexTypeSize(attrib.type);
    vertex_format.AppendAttribute(attrib, offset, stride);
    CHECK_EQ(uvs.size(), num_vertices);
    offset += num_vertices * stride;
  }

  const std::size_t num_vertex_bytes =
      vertex_format.GetVertexSize() * num_vertices;
  DataBuilder vertex_builder(num_vertex_bytes);

  // Used to convert mesh from local coordinates to world coordinates.
  pxr::GfMatrix4d matrix =
      usd_mesh.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());

  int face_vertex_offset = 0;
  for (int face_vertex_count : face_vertex_counts) {
    const uint16_t v0 = face_vertex_indices[face_vertex_offset];
    const auto p1 = matrix.Transform(positions[v0]);
    for (int offset = 2; offset < face_vertex_count; offset++) {
      int v1_vertex_offset = face_vertex_offset + (offset - 1);
      int v2_vertex_offset = face_vertex_offset + offset;

      const uint16_t v1 = face_vertex_indices[v1_vertex_offset];
      const uint16_t v2 = face_vertex_indices[v2_vertex_offset];
      const auto p2 = matrix.Transform(positions[v1]);
      const auto p3 = matrix.Transform(positions[v2]);

      vertex_builder.Append(p1);
      vertex_builder.Append(p2);
      vertex_builder.Append(p3);
    }
    face_vertex_offset += face_vertex_count;
  }

  // If we have normals (and potentially tangents), then calculate the
  // orientations of the vertices.
  if (!normals.empty() && !tangents.empty()) {
    CHECK_EQ(normals.size(), tangents.size());
    for (std::size_t i = 0; i < normals.size(); ++i) {
      const pxr::GfVec3f& normal = normals[i];
      const pxr::GfVec4f& tangent = tangents[i];
      const vec4 orientation = CalculateOrientation(
          vec3{normal[0], normal[1], normal[2]},
          vec4{tangent[0], tangent[1], tangent[2], tangent[3]});
      vertex_builder.Append(orientation);
    }
  } else {
    for (const pxr::GfVec3f& normal : normals) {
      const vec4 orientation =
          CalculateOrientation(vec3{normal[0], normal[1], normal[2]});
      vertex_builder.Append(orientation);
    }
  }

  for (const pxr::GfVec2f& uv : uvs) {
    vertex_builder.Append(vec2{uv[0], -uv[1]});
  }

  const std::size_t num_index_bytes = sizeof(uint16_t) * num_vertices;
  DataBuilder index_builder(num_index_bytes);
  for (uint16_t i = 0; i < num_vertices; ++i) {
    index_builder.Append<uint16_t>(i);
  }

  pxr::VtVec3fArray extent;
  usd_mesh.ComputeExtent(positions, &extent);
  CHECK_EQ(extent.size(), 2);
  const Box bounds({extent[0][0], extent[0][1], extent[0][2]},
                   {extent[1][0], extent[1][1], extent[1][2]});

  auto mesh = std::make_shared<MeshData>();
  mesh->SetName(Hash(usd_mesh.GetPrim().GetName().GetString()));
  mesh->SetVertexData(vertex_format, vertex_builder.Release(), num_vertices,
                      bounds);
  mesh->SetIndexData(MeshIndexType::U16, MeshPrimitiveType::Triangles,
                     index_builder.Release());
  meshes_.emplace_back(std::move(mesh));
}

void UsdAsset::ProcessSubMesh(pxr::UsdGeomSubset usd_subset) {
  static const pxr::TfToken kMaterialBinding("material:binding");
  pxr::UsdProperty binding = usd_subset.GetPrim().GetProperty(kMaterialBinding);
  if (binding.Is<pxr::UsdRelationship>()) {
    pxr::SdfPathVector targets;
    binding.As<pxr::UsdRelationship>().GetTargets(&targets);
    const std::string material_name = targets.front().GetString();
    mesh_materials_.push_back(material_name);
  } else {
    LOG(FATAL) << "Cannot bind material to mesh";
  }
}
}  // namespace redux

REDUX_REGISTER_MODEL_ASSET(usd, redux::UsdAsset);
REDUX_REGISTER_MODEL_ASSET(usda, redux::UsdAsset);
REDUX_REGISTER_MODEL_ASSET(usdc, redux::UsdAsset);
