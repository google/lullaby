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

#include "lullaby/systems/model_asset/model_asset.h"

#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/make_unique.h"

namespace lull {

ModelAsset::ModelAsset(std::function<void()> finalize_callback)
    : finalize_callback_(std::move(finalize_callback)) {}

void ModelAsset::OnLoad(const std::string& filename, std::string* data) {
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(data->data()),
                                 data->size(), 64 /* max_depth */,
                                 1000000 /* max_tables */,
                                 false /* check_alignment */);
  if (!verifier.VerifyBuffer<ModelDef>()) {
    LOG(DFATAL) << filename << " is an invalid model_asset.";
    return;
  }

  model_def_.Set(std::move(*data));
  if (!model_def_) {
    return;
  }
  id_ = Hash(filename);
  PrepareMesh();
  PrepareMaterials();
  PrepareTextures();
  PrepareSkeleton();
}

void ModelAsset::OnFinalize(const std::string& filename, std::string* data) {
  if (finalize_callback_) {
    finalize_callback_();
  }
}

void ModelAsset::PrepareMesh() {
  const int lod = 0;
  if (model_def_->lods() == nullptr || model_def_->lods()->size() != 1) {
    LOG(DFATAL) << "Only support single LODs for now.";
    return;
  }
  const ModelInstanceDef* model = model_def_->lods()->Get(lod);

  const uint32_t num_vertices = model->num_vertices();
  if (num_vertices == 0) {
    LOG(DFATAL) << "Model must have vertices.";
    return;
  }

  MeshData::IndexType index_type = MeshData::kIndexU16;
  const uint8_t* index_bytes = nullptr;
  uint32_t num_indices = 0;

  if (model->indices16()) {
    index_type = MeshData::kIndexU16;
    index_bytes = reinterpret_cast<const uint8_t*>(model->indices16()->data());
    num_indices = model->indices16()->size();
  } else if (model->indices32()) {
    index_type = MeshData::kIndexU32;
    index_bytes = reinterpret_cast<const uint8_t*>(model->indices32()->data());
    num_indices = model->indices32()->size();
  } else {
    LOG(DFATAL) << "Model must have indices.";
    return;
  }

  const uint32_t num_materials =
      model->materials() ? model->materials()->size() : 0;
  if (num_materials == 0) {
    LOG(DFATAL) << "Model must have materials.";
    return;
  }

  if (model->vertex_attributes() == nullptr) {
    LOG(DFATAL) << "Model must have attributes.";
    return;
  }

  VertexFormat vertex_format;
  for (auto attrib : *model->vertex_attributes()) {
    vertex_format.AppendAttribute(*attrib);
  }

  const uint8_t* vertex_bytes = model->vertex_data()->data();
  const size_t vertex_num_bytes = model->vertex_data()->size();
  const size_t index_num_bytes =
      num_indices * MeshData::GetIndexSize(index_type);
  const auto* range_bytes =
      reinterpret_cast<const uint8_t*>(model->ranges()->data());
  const size_t range_num_bytes = num_materials * sizeof(MeshData::IndexRange);

  DataContainer vertices =
      DataContainer::WrapDataAsReadOnly(vertex_bytes, vertex_num_bytes);
  DataContainer indices =
      DataContainer::WrapDataAsReadOnly(index_bytes, index_num_bytes);
  DataContainer submeshes =
      DataContainer::WrapDataAsReadOnly(range_bytes, range_num_bytes);
  mesh_data_ =
      MeshData(MeshData::kTriangles, vertex_format, std::move(vertices),
               index_type, std::move(indices), std::move(submeshes));
  base_blend_mesh_ = mesh_data_.CreateHeapCopy();

  if (model->blend_shapes()) {
    for (auto attrib : *model->blend_attributes()) {
      blend_format_.AppendAttribute(*attrib);
    }

    // Do not bother with the rest of the logic if the blend format is not
    // defined.
    if (blend_format_.GetVertexSize() > 0) {
      // Create a mapping of attributes between the main mesh and the blend
      // mesh.  We will use this mapping to create a "base" blend shape.
      std::vector<std::pair<const VertexAttribute*, const VertexAttribute*>>
          attrib_map;
      for (size_t i = 0; i < vertex_format.GetNumAttributes(); ++i) {
        const VertexAttribute* mesh_attrib = vertex_format.GetAttributeAt(i);
        for (size_t j = 0; j < blend_format_.GetNumAttributes(); ++j) {
          const VertexAttribute* blend_attrib = blend_format_.GetAttributeAt(j);
          if (mesh_attrib->usage() == blend_attrib->usage() &&
              mesh_attrib->type() == blend_attrib->type()) {
            attrib_map.emplace_back(mesh_attrib, blend_attrib);
          }
        }
      }

      // Create a copy of the mesh that just contains the data needed for
      // blending.
      // TODO: Do this in the model pipeline instead.
      const size_t num_vertices = mesh_data_.GetNumVertices();
      base_blend_shape_ = DataContainer::CreateHeapDataContainer(
          blend_format_.GetVertexSize() * num_vertices);
      for (size_t i = 0; i < num_vertices; ++i) {
        const uint8_t* vertex =
            mesh_data_.GetVertexBytes() + (i * vertex_format.GetVertexSize());

        for (auto& pair : attrib_map) {
          const size_t offset = vertex_format.GetAttributeOffset(pair.first);
          const size_t size = vertex_format.GetAttributeSize(*pair.first);
          base_blend_shape_.Append(vertex + offset, size);
        }
      }

      for (uint32_t ii = 0; ii < model->blend_shapes()->size(); ++ii) {
        const auto* blend_shape = model->blend_shapes()->Get(ii);
        const uint8_t* bytes = blend_shape->vertex_data()->Data();
        const size_t num_bytes = blend_shape->vertex_data()->size();
        DataContainer data =
            DataContainer::WrapDataAsReadOnly(bytes, num_bytes);
        blend_shape_names_.emplace_back(blend_shape->name());
        blend_shapes_.emplace_back(std::move(data));
      }
    }
  }

  if (model->aabbs()) {
    std::vector<Aabb> submesh_aabbs;
    submesh_aabbs.reserve(model->aabbs()->size());
    for (uint32_t ii = 0; ii < model->aabbs()->size(); ++ii) {
      auto min = (*model->aabbs())[ii]->min_position();
      auto max = (*model->aabbs())[ii]->max_position();

      submesh_aabbs.push_back(Aabb(mathfu::vec3(min->x(), min->y(), min->z()),
                                   mathfu::vec3(max->x(), max->y(), max->z())));
    }

    mesh_data_.SetSubmeshAabbs(std::move(submesh_aabbs));
  }
}

static MaterialInfo BuildMaterialInfo(const MaterialDef* material_def) {
  VariantMap properties;
  VariantMapFromFbVariantMap(material_def->properties(), &properties);

  std::string shading_model;
  constexpr HashValue kShadingModel = ConstHash("ShadingModel");
  auto iter = properties.find(kShadingModel);
  if (iter != properties.end()) {
    shading_model = iter->second.ValueOr(shading_model);
  }

  MaterialInfo material(shading_model);
  material.SetProperties(std::move(properties));

  if (material_def->textures()) {
    for (uint32_t i = 0; i < material_def->textures()->size(); ++i) {
      const MaterialTextureDef* texture_def = material_def->textures()->Get(i);
      if (texture_def->name()) {
        // TODO: Propagate TextureDef sampler params (i.e. mipmap,
        // wrap modes).
        if (texture_def->usage_per_channel()) {
          const TextureUsageInfo usage_info(
              {reinterpret_cast<const MaterialTextureUsage*>(
                   texture_def->usage_per_channel()->data()),
               texture_def->usage_per_channel()->size()});
          material.SetTexture(usage_info, texture_def->name()->c_str());
        } else {
          material.SetTexture(texture_def->usage(),
                              texture_def->name()->c_str());
        }
      }
    }
  }
  return material;
}

void ModelAsset::PrepareMaterials() {
  if (model_def_->lods() == nullptr) {
    LOG(DFATAL) << "No geometry/model data in the lullmodel file.";
    return;
  }
  if (model_def_->lods()->size() != 1) {
    LOG(DFATAL) << "Lullaby currently does not support multiple LODs";
    return;
  }

  const int lod = 0;
  const ModelInstanceDef* model = model_def_->lods()->Get(lod);
  const uint32_t num_materials =
      model->materials() ? model->materials()->size() : 0;

  materials_.reserve(num_materials);
  for (uint32_t i = 0; i < num_materials; ++i) {
    const MaterialDef* material_def = model->materials()->Get(i);
    if (material_def) {
      materials_.emplace_back(BuildMaterialInfo(material_def));
    } else {
      LOG(DFATAL) << "Invalid/missing MaterialDef.";
    }
  }
}

static ModelAsset::TextureInfo BuildTextureInfo(const TextureDef* texture_def) {
  ModelAsset::TextureInfo info;
  if (texture_def->name()) {
    info.name = texture_def->name()->str();
  }
  if (texture_def->file()) {
    info.file = texture_def->file()->str();
  }
  info.params.generate_mipmaps = texture_def->generate_mipmaps();
  info.params.premultiply_alpha = texture_def->premultiply_alpha();
  info.params.min_filter = texture_def->min_filter();
  info.params.mag_filter = texture_def->mag_filter();
  info.params.wrap_s = texture_def->wrap_s();
  info.params.wrap_t = texture_def->wrap_t();
  if (texture_def->data() && texture_def->data()->data()) {
    const uint8_t* bytes = texture_def->data()->data();
    const size_t num_bytes = texture_def->data()->size();
    DecodeImageFlags decode_flag = kDecodeImage_None;
    // Use the software decoder if no hardware decoding is available.
    if (CpuAstcDecodingAvailable() && !GpuAstcDecodingAvailable()) {
      decode_flag = kDecodeImage_DecodeAstc;
    }
    info.data = DecodeImage(bytes, num_bytes, decode_flag);
  }
  return info;
}

void ModelAsset::PrepareTextures() {
  const uint32_t num_textures =
      model_def_->textures() ? model_def_->textures()->size() : 0;

  textures_.reserve(num_textures);
  for (uint32_t i = 0; i < num_textures; ++i) {
    const TextureDef* texture_def = model_def_->textures()->Get(i);
    if (texture_def) {
      textures_.emplace_back(BuildTextureInfo(texture_def));
    }
  }
}

bool ModelAsset::HasValidSkeleton() const {
  const auto* skeleton = model_def_->skeleton();
  return skeleton != nullptr && skeleton->bone_names() != nullptr;
}

void ModelAsset::PrepareSkeleton() {
  const auto* skeleton = model_def_->skeleton();
  if (!HasValidSkeleton()) {
    return;
  }

  bone_names_.reserve(skeleton->bone_names()->size());
  for (size_t i = 0; i < bone_names_.capacity(); ++i) {
    bone_names_.push_back(
        skeleton->bone_names()->Get(static_cast<uint32_t>(i))->str());
  }
}

Span<uint8_t> ModelAsset::GetParentBoneIndices() const {
  const auto* skeleton = model_def_->skeleton();
  return {skeleton->bone_parents()->data(), skeleton->bone_parents()->size()};
}

Span<uint8_t> ModelAsset::GetShaderBoneIndices() const {
  const ModelInstanceDef* model = model_def_->lods()->Get(0);
  return {model->shader_to_mesh_bones()->data(),
          model->shader_to_mesh_bones()->size()};
}

Span<mathfu::AffineTransform> ModelAsset::GetInverseBindPose() const {
  const auto* skeleton = model_def_->skeleton();
  return {reinterpret_cast<const mathfu::AffineTransform*>(
              skeleton->bone_transforms()->data()),
          skeleton->bone_transforms()->size()};
}

void ModelAsset::CopyMeshToCollisionData() {
  if (collision_data_.GetNumVertices() == 0) {
    collision_data_ = mesh_data_.CreateHeapCopy();
  }
}

}  // namespace lull
