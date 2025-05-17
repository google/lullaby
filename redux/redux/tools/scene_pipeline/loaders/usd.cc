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

#include "redux/tools/scene_pipeline/loaders/usd.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "redux/tools/scene_pipeline/buffer.h"
#include "redux/tools/scene_pipeline/buffer_view.h"
#include "redux/tools/scene_pipeline/collider.h"
#include "redux/tools/scene_pipeline/drawable.h"
#include "redux/tools/scene_pipeline/image.h"
#include "redux/tools/scene_pipeline/index_buffer.h"
#include "redux/tools/scene_pipeline/loaders/import_options.h"
#include "redux/tools/scene_pipeline/loaders/import_utils.h"
#include "redux/tools/scene_pipeline/loaders/usd_utils.h"
#include "redux/tools/scene_pipeline/material.h"
#include "redux/tools/scene_pipeline/model.h"
#include "redux/tools/scene_pipeline/sampler.h"
#include "redux/tools/scene_pipeline/scene.h"
#include "redux/tools/scene_pipeline/type_id.h"
#include "redux/tools/scene_pipeline/types.h"
#include "redux/tools/scene_pipeline/vertex_buffer.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
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
#include "pxr/usd/usdPhysics/collisionAPI.h"
#include "pxr/usd/usdPhysics/massAPI.h"
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/types.h"
#include "pxr/usd/usdShade/utils.h"

namespace redux::tool {

class UsdLoader {
 public:
  explicit UsdLoader(const ImportOptions& opts) : opts_(opts) {}

  std::unique_ptr<Scene> ImportScene(std::string_view path) {
    scene_ = std::make_unique<Scene>();

    Buffer file = opts_.file_loader(path);
    scene_->buffers.emplace_back(std::move(file));

    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(std::string(path));
    stage->Flatten(false);
    Traverse(stage->GetPseudoRoot().GetChildren().front());

    // Meshes and materials may appear in any order when traversing the USD. As
    // such, we've had to keep track of which material is associated with each
    // drawable by name. Now that we're done processing all the drawables and
    // materials, we can resolve the material indices.
    for (int i = 0; i < scene_->drawables.size(); ++i) {
      DrawableIndex index(i);
      Drawable& drawable = scene_->drawables[index];
      const std::string& material_name = unresolved_materials_[index];
      drawable.material_index = material_lookup_[material_name];
      CHECK(drawable.material_index.valid());
    }

    return std::move(scene_);
  }

 private:
  const ImportOptions& opts_;
  std::unique_ptr<Scene> scene_;
  Model::Node* curr_node_ = nullptr;
  absl::flat_hash_map<DrawableIndex, std::string> unresolved_materials_;
  absl::flat_hash_map<std::string, MaterialIndex> material_lookup_;
  absl::flat_hash_map<std::string, ImageIndex> image_lookup_;

  // Adds an Image to the scene from the given USD texture node.
  ImageIndex ResolveTextureImage(pxr::UsdShadeShader usd_texture) {
    auto path =
        ReadShaderAttributeAs<pxr::SdfAssetPath>(usd_texture, kInputsFile);
    CHECK(path.has_value()) << "Texture node is missing reference to image.";

    const std::string& uri = path->GetResolvedPath();
    auto it = image_lookup_.find(uri);
    if (it != image_lookup_.end()) {
      return it->second;
    }

    ImageIndex index = LoadImageIntoScene(scene_.get(), opts_, uri);
    image_lookup_[uri] = index;
    return index;
  }

  // Adds a Buffer to the scene from a VtArray of type T.
  template <typename T>
  BufferView AddBuffer(const pxr::VtArray<T>& arr) {
    BufferView buffer_view;
    buffer_view.offset = 0;
    buffer_view.length = arr.size() * sizeof(T);
    buffer_view.buffer_index = BufferIndex(scene_->buffers.size());
    scene_->buffers.emplace_back(Buffer::Copy(arr.data(), arr.size()));
    return buffer_view;
  }

  // Adds a vertex attribute to the drawable using the `arr` data. Also, adds
  // the data to the active scene as a Buffer.
  template <typename T>
  void AddAttribute(Drawable& drawable, std::string_view name,
                    const pxr::VtArray<T>& arr) {
    TypeId type_id;
    if (std::is_same_v<T, float>) {
      type_id = GetTypeId<float>();
    } else if (std::is_same_v<T, pxr::GfVec2f>) {
      type_id = GetTypeId<float2>();
    } else if (std::is_same_v<T, pxr::GfVec3f>) {
      type_id = GetTypeId<float3>();
    } else if (std::is_same_v<T, pxr::GfVec4f>) {
      type_id = GetTypeId<float4>();
    } else {
      LOG(FATAL) << "Unsupported attribute type: " << type_id;
    }

    VertexBuffer::Attribute attribute;
    attribute.name = name;
    attribute.type = type_id;
    attribute.stride = sizeof(T);
    attribute.buffer_view = AddBuffer(arr);
    drawable.vertex_buffer.attributes.emplace_back(attribute);
  }

  // Recursively traverses the USD scene graph, visiting and processing all the
  // prims.
  void Traverse(pxr::UsdPrim prim) {
    Model::Node* parent_node = curr_node_;

    if (IsComponentKind(prim)) {
      CHECK(curr_node_ == nullptr) << "Component kind should be the root node.";
      CHECK(prim.IsA<pxr::UsdGeomXform>())
          << "Only XForm prims should have a Component kind.";

      Model& model = scene_->models.emplace_back();
      curr_node_ = &model.root_node;

      pxr::UsdGeomXform xform(prim);
      pxr::GfMatrix4f matrix(
          xform.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default()));
      model.transform = ToFloat4x4(matrix);
    }

    if (prim.IsA<pxr::UsdShadeMaterial>()) {
      pxr::UsdShadeMaterial material(prim);
      ProcessMaterial(material);
    } else if (prim.IsA<pxr::UsdShadeShader>()) {
      pxr::UsdShadeShader shader(prim);
      ProcessShader(shader);
    } else if (prim.IsA<pxr::UsdGeomMesh>()) {
      pxr::UsdGeomMesh mesh(prim);
      if (prim.HasAPI<pxr::UsdPhysicsCollisionAPI>()) {
        ProcessPhysicsMesh(mesh);
      } else {
        ProcessGeomMesh(mesh);
      }
    } else if (prim.IsA<pxr::UsdGeomXform>()) {
      pxr::UsdGeomXform xform(prim);
      if (curr_node_) {
        curr_node_ = &curr_node_->children.emplace_back();
        curr_node_->name = xform.GetPath().GetString();
      }
      ProcessXForm(xform);
    } else if (prim.IsA<pxr::UsdGeomScope>()) {
      // Ignore, this is just a grouping structure.
    } else {
      LOG(ERROR) << "Unsupported prim type: " << prim.GetTypeName().GetString();
    }

    for (auto child : prim.GetChildren()) {
      Traverse(child);
    }

    curr_node_ = parent_node;
  }

  // Updates the current node's transform based on the given XForm prim.
  void ProcessXForm(pxr::UsdGeomXform xform) {
    if (curr_node_) {
      pxr::GfMatrix4d mat4d;
      bool reset_stack;
      xform.GetLocalTransformation(&mat4d, &reset_stack);
      curr_node_->transform = ToFloat4x4(pxr::GfMatrix4f(mat4d));
    }
  }

  void ProcessPhysicsMesh(pxr::UsdGeomMesh usd_mesh) {
    pxr::VtArray<int> face_vertex_counts = ReadMeshFaceCounts(usd_mesh);
    for (const int count : face_vertex_counts) {
      CHECK_EQ(count, 3) << "Only triangle meshes are supported.";
    }

    pxr::VtArray<pxr::GfVec3f> positions = ReadMeshPositions(usd_mesh);
    pxr::VtArray<int> indices = ReadMeshFaceIndices(usd_mesh);

    ColliderIndex index(scene_->colliders.size());
    Collider& collider = scene_->colliders.emplace_back();
    collider.collider_type = Collider::ColliderType::kTriMesh;
    collider.tri_mesh.vertices = AddBuffer(positions);
    collider.tri_mesh.triangles = AddBuffer(indices);

    CHECK(curr_node_) << "Mesh encountered without having found a Component?";
    curr_node_->collider_indexes.emplace_back(index);
  }

  // Adds a Drawable to the scene for the given USD mesh.
  void ProcessGeomMesh(pxr::UsdGeomMesh usd_mesh) {
    DrawableIndex index(scene_->drawables.size());
    Drawable& drawable = scene_->drawables.emplace_back();

    // We associate materials with drawables after we're done processing all
    // the materials and drawables. For now, just store the information we need
    // to resolve the material later.
    const std::string material_binding =
        ReadMaterialBinding(usd_mesh.GetPrim());
    CHECK(!material_binding.empty());
    unresolved_materials_[index] = material_binding;

    pxr::VtArray<int> face_vertex_counts = ReadMeshFaceCounts(usd_mesh);
    for (const int count : face_vertex_counts) {
      CHECK_EQ(count, 3) << "Only triangle meshes are supported.";
    }
    drawable.primitive_type = Drawable::PrimitiveType::kTriangleList;

    pxr::VtArray<pxr::GfVec3f> positions = ReadMeshPositions(usd_mesh);
    pxr::VtArray<pxr::GfVec3f> normals = ReadMeshNormals(usd_mesh);
    pxr::VtArray<pxr::GfVec4f> tangents = ReadMeshTangents(usd_mesh);
    pxr::VtArray<pxr::GfVec2f> uvs = ReadMeshUvs(usd_mesh);
    pxr::VtArray<int> indices = ReadMeshFaceIndices(usd_mesh);

    // Normals, tangents, and uvs are defined with a "faceVarying"
    // interpolation. This means that each face vertex has its own normal,
    // tangent, and/or uv. However, it's possible that the points are shared
    // between faces. In this case, we need to expand the points so that each
    // face vertex has its own point as well (to match the normal, etc.).
    if (!normals.empty() && normals.size() != positions.size()) {
      if (!uvs.empty()) {
        CHECK_EQ(uvs.size(), normals.size());
      }
      if (!tangents.empty()) {
        CHECK_EQ(tangents.size(), normals.size());
      }

      pxr::VtArray<int> expanded_indices;
      pxr::VtArray<pxr::GfVec3f> expanded_positions;
      expanded_indices.reserve(normals.size());
      expanded_positions.reserve(normals.size());
      for (int i = 0; i < normals.size(); ++i) {
        expanded_positions.push_back(positions[indices[i]]);
        expanded_indices.push_back(i);
      }
      positions = std::move(expanded_positions);
      indices = std::move(expanded_indices);
    }

    if (!positions.empty()) {
      AddAttribute(drawable, VertexBuffer::kPosition, positions);
    }
    if (!normals.empty()) {
      AddAttribute(drawable, VertexBuffer::kNormal, normals);
    }
    if (!tangents.empty()) {
      AddAttribute(drawable, VertexBuffer::kTangent, tangents);
    }
    if (!uvs.empty()) {
      AddAttribute(drawable, VertexBuffer::kTexCoord, uvs);
    }

    // We are effectively casting the ints to unsigned ints. We assume there are
    // no negative indices.
    drawable.index_buffer.type = GetTypeId<std::uint32_t>();
    drawable.index_buffer.buffer_view = AddBuffer(indices);
    drawable.index_buffer.num_indices = indices.size();

    drawable.offset = 0;
    drawable.count = indices.size();
    drawable.vertex_buffer.num_vertices = positions.size();

    CHECK(curr_node_) << "Mesh encountered without having found a Component?";
    curr_node_->drawable_indexes.emplace_back(index);
  }

  // Adds a Material to the scene for the given USD material. As this function
  // is called during traversal, the actual "contents" of the Material will be
  // filled in as we encounter the inputs during traversal.
  void ProcessMaterial(pxr::UsdShadeMaterial usd_material) {
    // Begin a new material and keep track of its path. We'll use this to
    // associate materials to drawables later.
    const std::string& path = usd_material.GetPath().GetString();
    material_lookup_[path] = MaterialIndex(scene_->materials.size());
    scene_->materials.emplace_back();
  }

  // Processes a USD shader node during traversal.
  void ProcessShader(pxr::UsdShadeShader usd_shader) {
    pxr::TfToken id;
    usd_shader.GetShaderId(&id);
    if (id == kUsdPreviewSurface) {
      ProcessUsdPreviewSurface(usd_shader);
    } else if (id == kUsdUVTexture) {
      // Ignore.  We'll process this when we encounter it as an input to a
      // material property.
    } else if (id == kUsdPrimvarReaderFloat2) {
      // Ignore.  We'll process this when we encounter it as an input to a
      // material property.
    } else {
      LOG(ERROR) << "Unknown UsdShadeShader: " << id.GetString();
    }
  }

  // Data read from a shader input.
  template <typename T>
  struct ResolvedShaderInput {
    // The value of the input if it is a constant.
    std::optional<T> value;

    // The value of the input if it is a texture.
    std::optional<Sampler> sampler;
  };

  // Reads UsdShadeInput as a value of type T or a texture sampler.
  template <typename T>
  ResolvedShaderInput<T> ReadShaderInput(pxr::UsdShadeInput input) {
    ResolvedShaderInput<T> out;
    if (!input.GetPrim().IsValid()) {
      return out;
    }

    pxr::UsdShadeConnectableAPI source;
    pxr::TfToken source_name;
    pxr::UsdShadeAttributeType source_type;
    if (input.GetConnectedSource(&source, &source_name, &source_type)) {
      pxr::UsdPrim prim = source.GetPrim();
      CHECK(prim && prim.IsA<pxr::UsdShadeShader>());
      out.sampler = ReadSampler(pxr::UsdShadeShader(prim), source_name);
    } else {
      if constexpr (std::is_same_v<T, float>) {
        out.value = ReadInputAs<float>(input);
      } else if constexpr (std::is_same_v<T, float3>) {
        out.value = ToFloat3(ReadInputAs<pxr::GfVec3f>(input));
      } else if constexpr (std::is_same_v<T, float4>) {
        out.value = ToFloat4(ReadInputAs<pxr::GfVec4f>(input));
      }
    }
    return out;
  }

  // Reads sampler ata from a USD shader node.
  Sampler ReadSampler(pxr::UsdShadeShader usd_shader,
                      const pxr::TfToken& source) {
    pxr::TfToken shader_id;
    CHECK(usd_shader.GetShaderId(&shader_id));
    CHECK(shader_id == kUsdUVTexture);

    // ignore: inputs:fallback, inputs:sourceColorSpace

    Sampler sampler;
    sampler.image_index = ResolveTextureImage(usd_shader);
    sampler.channel_mask = GetChannelMask(source);
    sampler.min_filter = Sampler::Filter::kNearest;
    sampler.mag_filter = Sampler::Filter::kNearest;
    sampler.wrap_s = ToWrapMode(
        ReadShaderAttributeAs<pxr::TfToken>(usd_shader, kInputsWrapS));
    sampler.wrap_t = ToWrapMode(
        ReadShaderAttributeAs<pxr::TfToken>(usd_shader, kInputsWrapT));
    sampler.scale = ToFloat4(ReadShaderAttributeOr<pxr::GfVec4f>(
        usd_shader, kInputsScale, pxr::GfVec4f{1, 1, 1, 1}));
    sampler.bias = ToFloat4(ReadShaderAttributeOr<pxr::GfVec4f>(
        usd_shader, kInputsBias, pxr::GfVec4f{0, 0, 0, 0}));

    // TODO: Read the varname for the texcoords. We need to then resolve this
    // with the geometry mesh uvs to get the uv index. For now, we just assume a
    // single set of uvs.
    sampler.texcoord = 0;
    return sampler;
  }

  void ProcessUsdPreviewSurface(pxr::UsdShadeShader usd_shader) {
    Material& material = scene_->materials.back();
    material.shading_model = Material::kMetallicRoughness;

    auto& props = material.properties;
    props[Material::kFlipUv].Set<bool>(true);

    auto diffuse = ReadShaderInput<float3>(usd_shader.GetInput(kDiffuseColor));
    if (diffuse.sampler.has_value()) {
      props[Material::kBaseColor].Set(float3(1, 1, 1));
      props[Material::kBaseColorTexture].Set(*diffuse.sampler);
    }
    if (diffuse.value.has_value()) {
      props[Material::kBaseColor].Set(*diffuse.value);
    }

    auto normal = ReadShaderInput<float3>(usd_shader.GetInput(kNormal));
    if (normal.sampler.has_value()) {
      props[Material::kNormalScale].Set<float>(1.f);
      props[Material::kNormalTexture].Set(*normal.sampler);
    }

    auto metallic = ReadShaderInput<float>(usd_shader.GetInput(kMetallic));
    if (metallic.sampler.has_value()) {
      props[Material::kMetallic].Set<float>(1.f);
      props[Material::kMetallicTexture].Set(*metallic.sampler);
    }
    if (metallic.value.has_value()) {
      props[Material::kMetallic].Set(*metallic.value);
    }

    auto roughness = ReadShaderInput<float>(usd_shader.GetInput(kRoughness));
    if (roughness.sampler.has_value()) {
      props[Material::kRoughness].Set<float>(1.f);
      props[Material::kRoughnessTexture].Set(*roughness.sampler);
    }
    if (roughness.value.has_value()) {
      props[Material::kRoughness].Set(*roughness.value);
    }

    auto occlusion = ReadShaderInput<float>(usd_shader.GetInput(kOcclusion));
    if (occlusion.sampler.has_value()) {
      props[Material::kOcclusionStrength].Set<float>(1.f);
      props[Material::kOcclusionTexture].Set(*occlusion.sampler);
    }
    if (occlusion.value.has_value()) {
      props[Material::kOcclusionStrength].Set(*occlusion.value);
    }

    auto emissive =
        ReadShaderInput<float3>(usd_shader.GetInput(kEmissiveColor));
    if (emissive.sampler.has_value()) {
      props[Material::kEmissive].Set(float3(1, 1, 1));
      props[Material::kEmissiveTexture].Set(*emissive.sampler);
    }
    if (emissive.value.has_value()) {
      props[Material::kEmissive].Set(*emissive.value);
    }
  }
};

std::unique_ptr<Scene> LoadUsd(std::string_view path,
                               const ImportOptions& opts) {
  UsdLoader importer(opts);
  return importer.ImportScene(path);
}

}  // namespace redux::tool
