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

#include "lullaby/systems/render/next/mesh_factory.h"

#include "fplbase/mesh_generated.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/make_unique.h"

namespace lull {
namespace {

VertexAttribute ConvertAttribute(uint8_t in) {
  VertexAttribute out;
  switch (in) {
    case meshdef::Attribute_Position2f:
      out.mutate_usage(VertexAttributeUsage_Position);
      out.mutate_type(VertexAttributeType_Vec2f);
      break;
    case meshdef::Attribute_Position3f:
      out.mutate_usage(VertexAttributeUsage_Position);
      out.mutate_type(VertexAttributeType_Vec3f);
      break;
    case meshdef::Attribute_Normal3f:
      out.mutate_usage(VertexAttributeUsage_Normal);
      out.mutate_type(VertexAttributeType_Vec3f);
      break;
    case meshdef::Attribute_Tangent4f:
      out.mutate_usage(VertexAttributeUsage_Tangent);
      out.mutate_type(VertexAttributeType_Vec4f);
      break;
    case meshdef::Attribute_Orientation4f:
      LOG(DFATAL) << "Add proper orientation support to lull::VertexFormat";
      out.mutate_usage(VertexAttributeUsage_Tangent);
      out.mutate_type(VertexAttributeType_Vec4f);
      break;
    case meshdef::Attribute_TexCoord2f:
      out.mutate_usage(VertexAttributeUsage_TexCoord);
      out.mutate_type(VertexAttributeType_Vec2f);
      break;
    case meshdef::Attribute_TexCoordAlt2f:
      out.mutate_usage(VertexAttributeUsage_TexCoord);
      out.mutate_type(VertexAttributeType_Vec2f);
      break;
    case meshdef::Attribute_TexCoord2us:
      out.mutate_usage(VertexAttributeUsage_TexCoord);
      out.mutate_type(VertexAttributeType_Vec2us);
      break;
    case meshdef::Attribute_Color4ub:
      out.mutate_usage(VertexAttributeUsage_Color);
      out.mutate_type(VertexAttributeType_Vec4ub);
      break;
    case meshdef::Attribute_BoneIndices4ub:
      out.mutate_usage(VertexAttributeUsage_BoneIndices);
      out.mutate_type(VertexAttributeType_Vec4ub);
      break;
    case meshdef::Attribute_BoneWeights4ub:
      out.mutate_usage(VertexAttributeUsage_BoneWeights);
      out.mutate_type(VertexAttributeType_Vec4ub);
      break;
    case meshdef::Attribute_END:
      break;
    default:
      LOG(DFATAL) << "Unknown attribute type: " << static_cast<int>(in);
      break;
  }
  return out;
}

bool IsSkinned(const meshdef::Mesh* meshdef) {
  return meshdef->bone_transforms() && meshdef->bone_transforms()->size() &&
         meshdef->skin_indices() && meshdef->skin_indices()->size();
}

VertexFormat BuildVertexFormat(const meshdef::Mesh* meshdef) {
  int attr_count = 0;
  VertexAttribute attributes[VertexFormat::kMaxAttributes];
  if (meshdef->attributes()) {
    for (const auto iter : *meshdef->attributes()) {
      if (attr_count >= VertexFormat::kMaxAttributes) {
        break;
      }
      attributes[attr_count] = ConvertAttribute(iter);
      if (attributes[attr_count].usage()) {
        ++attr_count;
      } else {
        break;
      }
    }
  } else {
    attributes[attr_count] = ConvertAttribute(meshdef::Attribute_Position3f);
    ++attr_count;

    if (meshdef->normals() && meshdef->normals()->size()) {
      attributes[attr_count] = ConvertAttribute(meshdef::Attribute_Normal3f);
      ++attr_count;
    }
    if (meshdef->tangents() && meshdef->tangents()->size()) {
      attributes[attr_count] = ConvertAttribute(meshdef::Attribute_Tangent4f);
      ++attr_count;
    }
    if (meshdef->orientations() && meshdef->orientations()->size()) {
      attributes[attr_count] =
          ConvertAttribute(meshdef::Attribute_Orientation4f);
      ++attr_count;
    }
    if (meshdef->colors() && meshdef->colors()->size()) {
      attributes[attr_count] = ConvertAttribute(meshdef::Attribute_Color4ub);
      ++attr_count;
    }
    if (meshdef->texcoords() && meshdef->texcoords()->size()) {
      attributes[attr_count] = ConvertAttribute(meshdef::Attribute_TexCoord2f);
      ++attr_count;
    }
    if (meshdef->texcoords_alt() && meshdef->texcoords_alt()->size()) {
      attributes[attr_count] =
          ConvertAttribute(meshdef::Attribute_TexCoordAlt2f);
      ++attr_count;
    }
    if (IsSkinned(meshdef)) {
      attributes[attr_count] =
          ConvertAttribute(meshdef::Attribute_BoneIndices4ub);
      ++attr_count;
      attributes[attr_count] =
          ConvertAttribute(meshdef::Attribute_BoneWeights4ub);
      ++attr_count;
    }
  }
  return VertexFormat(attributes, attributes + attr_count);
}

template <typename T>
void CopyAttribute(const T* src, uint8_t** ptr) {
  T* dst = reinterpret_cast<T*>(*ptr);
  *dst = *src;
  *ptr += sizeof(T);
}

void BuildVertexDataFromArrays(const meshdef::Mesh* meshdef,
                               MeshData* mesh_data, uint32_t num_vertices) {
  const bool is_skinned = IsSkinned(meshdef);
  uint8_t vertex[64];
  for (uint32_t i = 0; i < num_vertices; i++) {
    uint8_t* ptr = vertex;
    flatbuffers::uoffset_t index = static_cast<flatbuffers::uoffset_t>(i);

    CopyAttribute(meshdef->positions()->Get(index), &ptr);
    if (meshdef->normals() && meshdef->normals()->size()) {
      CopyAttribute(meshdef->normals()->Get(index), &ptr);
    }
    if (meshdef->tangents() && meshdef->tangents()->size()) {
      CopyAttribute(meshdef->tangents()->Get(index), &ptr);
    }
    if (meshdef->orientations() && meshdef->orientations()->size()) {
      CopyAttribute(meshdef->orientations()->Get(index), &ptr);
    }
    if (meshdef->colors() && meshdef->colors()->size()) {
      CopyAttribute(meshdef->colors()->Get(index), &ptr);
    }
    if (meshdef->texcoords() && meshdef->texcoords()->size()) {
      CopyAttribute(meshdef->texcoords()->Get(index), &ptr);
    }
    if (meshdef->texcoords_alt() && meshdef->texcoords_alt()->size()) {
      CopyAttribute(meshdef->texcoords_alt()->Get(index), &ptr);
    }
    if (is_skinned) {
      CopyAttribute(meshdef->skin_indices()->Get(index), &ptr);
      CopyAttribute(meshdef->skin_weights()->Get(index), &ptr);
    }
    mesh_data->AddVertices(vertex, 1,
                           mesh_data->GetVertexFormat().GetVertexSize());
  }
}

class MeshAsset : public Asset {
 public:
  explicit MeshAsset(std::function<void(MeshAsset*)> finalizer)
      : finalizer_(std::move(finalizer)) {}

  void OnLoad(const std::string& filename, std::string* data) override {
    const meshdef::Mesh* meshdef = meshdef::GetMesh(data->c_str());

    VertexFormat vertex_format = BuildVertexFormat(meshdef);
    const uint32_t vertex_size =
        static_cast<uint32_t>(vertex_format.GetVertexSize());

    const uint32_t num_vertices =
        meshdef->vertices() ? meshdef->vertices()->size() / vertex_size : 0;
    const uint32_t num_positions =
        meshdef->positions() ? meshdef->positions()->size() : 0;
    if (num_vertices == 0 && num_positions == 0) {
      LOG(DFATAL) << "Mesh must have vertex data.";
      return;
    }

    const uint32_t num_surfaces =
        meshdef->surfaces() ? meshdef->surfaces()->size() : 0;
    if (num_surfaces == 0) {
      LOG(DFATAL) << "Mesh must have surfaces.";
      return;
    }

    const MeshData::IndexType index_type =
        meshdef->surfaces()->Get(0)->indices() ? MeshData::kIndexU16
                                               : MeshData::kIndexU32;
    uint32_t num_indices = 0;
    for (uint32_t i = 0; i < num_surfaces; ++i) {
      const auto* surface = meshdef->surfaces()->Get(i);
      if (!surface) {
        LOG(DFATAL) << "Mesh must have surface.";
        return;
      }
      if (surface->indices()) {
        if (index_type != MeshData::kIndexU16) {
          LOG(DFATAL) << "Mesh has inconsistent index types.";
          return;
        }
        num_indices += surface->indices()->size();
      } else if (surface->indices32()) {
        if (index_type != MeshData::kIndexU32) {
          LOG(DFATAL) << "Mesh has inconsistent index types.";
          return;
        }
        num_indices += surface->indices32()->size();
      } else {
        LOG(ERROR) << "Surface " << i << " is missing indices.";
        return;
      }
      if (surface->material() && !surface->material()->str().empty()) {
        LOG(DFATAL) << "Materials (fplmat) not supported "
                    << surface->material()->c_str();
        return;
      }
    }
    if (num_indices == 0) {
      LOG(DFATAL) << "Mesh must have indices.";
      return;
    }

    DataContainer vertices = DataContainer::CreateHeapDataContainer(
        std::max(num_vertices, num_positions) * vertex_size);
    DataContainer indices = DataContainer::CreateHeapDataContainer(
        num_indices * MeshData::GetIndexSize(index_type));
    DataContainer submeshes = DataContainer::CreateHeapDataContainer(
        num_surfaces * sizeof(MeshData::IndexRange));
    mesh_data_ = MakeUnique<MeshData>(MeshData::kTriangles, vertex_format,
                                      std::move(vertices), index_type,
                                      std::move(indices), std::move(submeshes));

    if (num_vertices == 0) {
      BuildVertexDataFromArrays(meshdef, mesh_data_.get(), num_positions);
    } else {
      const size_t stride = vertex_format.GetVertexSize();
      mesh_data_->AddVertices(meshdef->vertices()->data(),
                              meshdef->vertices()->size() / stride, stride);
    }

    for (uint32_t i = 0; i < num_surfaces; ++i) {
      const auto* surface = meshdef->surfaces()->Get(i);
      if (!surface) {
        continue;
      }
      if (surface->indices()) {
        mesh_data_->AddIndices(surface->indices()->data(),
                               surface->indices()->size());
      } else if (surface->indices32()) {
        mesh_data_->AddIndices(surface->indices32()->data(),
                               surface->indices32()->size());
      } else {
        LOG(DFATAL) << "Surface " << i << " missing indices!";
        continue;
      }
    }

    const uint32_t num_bones =
        meshdef->bone_parents() ? meshdef->bone_parents()->size() : 0;
    if (num_bones > 0) {
      LOG(ERROR) << "Skinning not supported.";
    }
  }

  void OnFinalize(const std::string& filename, std::string* data) override {
    finalizer_(this);
  }

  std::unique_ptr<MeshData> mesh_data_;
  std::function<void(MeshAsset*)> finalizer_;
};

}  // namespace

MeshFactoryImpl::MeshFactoryImpl(Registry* registry)
    : registry_(registry), meshes_(ResourceManager<Mesh>::kWeakCachingOnly) {}

MeshPtr MeshFactoryImpl::LoadMesh(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());

  MeshPtr mesh = meshes_.Create(key, [&]() {
    auto mesh = std::make_shared<Mesh>();
    auto finalizer = [mesh](MeshAsset* asset) {
      if (asset->mesh_data_) {
        mesh->Init(asset->mesh_data_.get(), 1);
      }
    };

    auto* asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync<MeshAsset>(filename, std::move(finalizer));
    return mesh;
  });

  meshes_.Release(key);
  return mesh;
}

MeshPtr MeshFactoryImpl::CreateMesh(MeshData mesh_data) {
  return CreateMesh(&mesh_data);
}

MeshPtr MeshFactoryImpl::CreateMesh(MeshData* mesh_datas, size_t len) {
  MeshPtr mesh = std::make_shared<Mesh>();
  mesh->Init(mesh_datas, len);
  return mesh;
}

MeshPtr MeshFactoryImpl::CreateMesh(HashValue name, MeshData mesh_data) {
  return CreateMesh(name, &mesh_data);
}

MeshPtr MeshFactoryImpl::CreateMesh(HashValue name, MeshData* mesh_datas,
                                    size_t len) {
  return meshes_.Create(name, [&]() {
    return CreateMesh(mesh_datas, len);
  });
}

void MeshFactoryImpl::CacheMesh(HashValue name, const MeshPtr& mesh) {
  meshes_.Register(name, mesh);
}

MeshPtr MeshFactoryImpl::GetMesh(HashValue name) const {
  return meshes_.Find(name);
}

void MeshFactoryImpl::ReleaseMesh(HashValue name) { meshes_.Release(name); }

MeshPtr MeshFactoryImpl::CreateMesh(const MeshData* mesh_data) {
  if (mesh_data->GetNumVertices() == 0) {
    return MeshPtr();
  }
  MeshPtr mesh = std::make_shared<Mesh>();
  mesh->Init(mesh_data, 1);
  return mesh;
}

MeshPtr MeshFactoryImpl::CreateMesh(HashValue name, const MeshData* mesh_data) {
  return meshes_.Create(name, [&]() { return CreateMesh(mesh_data); });
}

MeshPtr MeshFactoryImpl::EmptyMesh() {
  if (!empty_) {
    empty_ = std::make_shared<Mesh>();
  }
  return empty_;
}

}  // namespace lull
