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

#ifndef REDUX_SYSTEMS_MODEL_MODEL_ASSET_H_
#define REDUX_SYSTEMS_MODEL_MODEL_ASSET_H_

#include <cstddef>
#include <functional>
#include <string>

#include "redux/engines/physics/collision_data.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/graphics/image_data.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Parses a model file and extracts the relevant information so that it
// can be consumed at runtime.
class ModelAsset {
 public:
  using ByteSpan = absl::Span<const std::byte>;
  using ImageDecoder = std::function<ImageData(ImageData)>;

  ModelAsset() = default;

  void OnLoad(std::shared_ptr<DataContainer> data, ImageDecoder decoder);

  void OnFinalize() { is_ready_ = true; }

  bool IsReady() const { return is_ready_; }

  // Returns the MeshData contained in the model asset.
  MeshData GetMeshData() const;

  // Returns the Materials defined in the model asset, one material for each
  // part of the mesh.
  absl::Span<const MaterialData> GetMaterialData() const { return materials_; }

  struct TextureData {
    std::string_view uri;
    std::shared_ptr<ImageData> image;
    TextureParams params;
  };

  // Returns the TextureData associated with the name (as specified in the
  // Material).
  const TextureData* GetTextureData(std::string_view name) const;

  // Returns true if the asset has a valid skeleton.
  bool HasSkeleton() const { return !bone_names_.empty(); }

  // Returns the list of parent bone indices stored in the model asset.
  absl::Span<const uint16_t> GetParentBoneIndices() const {
    return parent_bones_;
  }

  // Returns the list of shader bone indices stored in the model asset.
  absl::Span<const uint16_t> GetShaderBoneIndices() const {
    return shader_bones_;
  }

  // Returns the inverse bind pose of the skeleton stored in the model asset.
  // These matrices are the bone-from-mesh transform for each bone.
  absl::Span<const mat4> GetInverseBindPose() const {
    return inverse_bind_pose_;
  }

  // Returns the list of bone names stored in the model asset.
  absl::Span<const std::string_view> GetBoneNames() const {
    return bone_names_;
  }

  // Returns the collision MeshData contained in the model asset.
  CollisionDataPtr GetCollisionData() const { return collision_data_; }

  // Returns true if the model contains blend shapes.
  bool HasBlendShapes() const { return !blend_shapes_.empty(); }

  const VertexFormat& GetBlendShapeFormat() const { return blend_format_; }

  // Returns the collection of blend shapes for the model.
  const absl::flat_hash_map<HashValue, ByteSpan>& GetBlendShapes() const {
    return blend_shapes_;
  }

 private:
  std::shared_ptr<ImageData> ReadImage(ImageData image,
                                       const ImageDecoder& decoder);

  std::shared_ptr<DataContainer> data_;

  // Information extracted from the raw ModelDef asset. We try as much as
  // possible to simply point to the buffer directly in the asset, but do need
  // to perform some additional processing in order to make the data compatible
  // with our runtime.
  VertexFormat vertex_format_;
  VertexFormat blend_format_;
  MeshIndexType index_format_;
  CollisionDataPtr collision_data_;
  ByteSpan vertex_data_;
  ByteSpan index_data_;
  std::vector<MeshData::PartData> parts_;
  std::vector<MaterialData> materials_;
  std::vector<std::string_view> bone_names_;
  std::vector<mat4> inverse_bind_pose_;
  std::vector<std::shared_ptr<ImageData>> decoded_images_;
  absl::Span<const uint16_t> parent_bones_;
  absl::Span<const uint16_t> shader_bones_;
  absl::flat_hash_map<HashValue, TextureData> textures_;
  absl::flat_hash_map<HashValue, ByteSpan> blend_shapes_;
  Box bounds_;
  bool is_ready_ = false;
};

}  // namespace redux

#endif  // REDUX_SYSTEMS_MODEL_MODEL_ASSET_H_
