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

#ifndef LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_H_
#define LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_H_

#include <memory>
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/modules/render/texture_params.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/span.h"
#include "lullaby/generated/model_def_generated.h"

namespace lull {

/// Parses a lullmodel file and extracts the relevant information so that it
/// can be consumed by appropriate runtime Systems.
class ModelAsset : public Asset {
 public:
  explicit ModelAsset(std::function<void()> finalize_callback);

  /// Extracts the data from the .lullmodel file and stores it locally.
  void OnLoad(const std::string& filename, std::string* data) override;

  /// Updates all Entities that were waiting for the model to finish loading.
  void OnFinalize(const std::string& filename, std::string* data) override;

  /// Returns the id of the model asset, which is a hash of the filename.
  HashValue GetId() const { return id_; }

  /// Returns the MeshData contained in the model asset.
  MeshData& GetMutableMeshData() { return mesh_data_; }

  /// Returns the MeshData contained in the model asset.
  const MeshData& GetMeshData() const { return mesh_data_; }

  /// Returns the list of materials contained in the model asset.
  std::vector<MaterialInfo>& GetMutableMaterials() { return materials_; }

  /// Information about a texture referenced by the model asset.
  struct TextureInfo {
    std::string name;
    std::string file;
    TextureParams params;
    ImageData data;
  };

  /// Returns the list of textures references by the model asset.
  std::vector<TextureInfo>& GetMutableTextures() { return textures_; }

  /// Returns true if the asset has a valid skeleton.
  bool HasValidSkeleton() const;

  /// Returns the list of parent bone indices stored in the model asset.
  Span<uint8_t> GetParentBoneIndices() const;

  /// Returns the list of shader bone indices stored in the model asset.
  Span<uint8_t> GetShaderBoneIndices() const;

  /// Returns the inverse bind pose of the skeleton stored in the model asset.
  /// These matrices are the bone-from-mesh transform for each bone.
  Span<mathfu::AffineTransform> GetInverseBindPose() const;

  /// Returns the list of bone names stored in the model asset.
  Span<std::string> GetBoneNames() const { return bone_names_; }

  /// Returns true if the model contains blend shapes.
  bool HasBlendShapes() const { return !blend_shape_names_.empty(); }

  /// Returns the vertex format of the blend shapes stored in the model asset.
  const VertexFormat& GetBlendShapeFormat() const { return blend_format_; }

  /// Returns the list of names of blend shapes stored in the model asset.
  Span<HashValue> GetBlendShapeNames() const { return blend_shape_names_; }

  /// Returns the "base" blend shape.
  const DataContainer& GetBaseBlendShapeData() const {
    return base_blend_shape_;
  }

  const MeshData& GetBaseBlendMesh() const {
    return base_blend_mesh_;
  }

  /// Returns the vertex data for the given blend shape (by index).
  const DataContainer& GetBlendShapeData(size_t index) const {
    return blend_shapes_[index];
  }

  /// Creates collision data by copying the mesh data.
  void CopyMeshToCollisionData();

  /// Returns the collision MeshData contained in the model asset.
  const MeshData& GetCollisionData() const { return collision_data_; }

 private:
  /// Provides a simple wrapper around a flatbuffer table class where the actual
  /// data is stored in a contiguous std container (ie. vector or string).
  template <typename T, typename Data>
  struct FlatbufferDataObject {
    void Set(Data data) { data_ = std::move(data); }

    explicit operator bool() { return data_.data() != nullptr; }
    const T* operator->() const { return get(); }
    const T* get() const { return flatbuffers::GetRoot<T>(data_.data()); }
    Data data_;
  };

  void PrepareMesh();
  void PrepareMaterials();
  void PrepareTextures();
  void PrepareSkeleton();

  HashValue id_;
  FlatbufferDataObject<ModelDef, std::string> model_def_;
  MeshData mesh_data_;
  MeshData collision_data_;
  VertexFormat blend_format_;
  DataContainer base_blend_shape_;
  MeshData base_blend_mesh_;
  std::vector<MaterialInfo> materials_;
  std::vector<TextureInfo> textures_;
  std::vector<std::string> bone_names_;
  std::vector<HashValue> blend_shape_names_;
  std::vector<DataContainer> blend_shapes_;
  std::function<void()> finalize_callback_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_MODEL_ASSET_MODEL_ASSET_H_
