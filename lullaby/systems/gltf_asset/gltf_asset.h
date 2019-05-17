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

#ifndef LULLABY_SYSTEMS_GLTF_ASSET_GLTF_ASSET_H_
#define LULLABY_SYSTEMS_GLTF_ASSET_GLTF_ASSET_H_

#include <memory>
#include <unordered_map>

#include "lullaby/modules/animation_channels/skeleton_channel.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/texture_params.h"
#include "lullaby/modules/render/vertex_format.h"
#include "lullaby/modules/tinygltf/tinygltf_util.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"
#include "mathfu/glsl_mappings.h"
#include "motive/math/compact_spline.h"
#include "motive/rig_anim.h"
#include "tiny_gltf.h"

namespace lull {

/// Parses a GLTF file and extracts the relevant information so that it can be
/// consumed by appropriate runtime Systems.
class GltfAsset : public Asset {
 public:
  explicit GltfAsset(Registry* registry,
                     bool preserve_normal_tangent,
                     std::function<void()> finalize_callback);

  /// Extracts the data from the .gltf file and stores it locally.
  void OnLoad(const std::string& filename, std::string* data) override;

  /// Updates all Entities that were waiting for the GLTF to finish loading.
  void OnFinalize(const std::string& filename, std::string* data) override;

  /// Returns the id of the GLTF asset, which is a hash of the filename.
  HashValue GetId() const { return id_; }

  /// Information about a GLTF Node.
  struct NodeInfo {
    std::string name;
    Sqt transform;
    /// Indices into the list of NodeInfos of children of this Node.
    std::vector<int> children;
    /// Index into the list of MeshInfos of this Node's Mesh.
    int mesh = kInvalidTinyGltfIndex;
    /// Index into the list of SkinInfos of this Node's Skin.
    int skin = kInvalidTinyGltfIndex;
    /// List of default blend shape weights. May be empty, in which case the
    /// weights of the Mesh will be used.
    std::vector<float> blend_shape_weights;
  };

  /// Returns a list of NodeInfos: one per GLTF Node and in the same order they
  /// appear in the GLTF.
  Span<NodeInfo> GetNodeInfos() const { return node_infos_; }

  /// Returns a list of indices into the list of NodeInfos that indicates all
  /// the root Nodes in the default scene of the GLTF.
  Span<int> GetRootNodes() const { return root_nodes_; }

  /// Information about a GLTF Mesh and its blend shapes.
  struct MeshInfo {
    MeshData mesh_data;
    /// Index of the Mesh's Material.
    int material_index = kInvalidTinyGltfIndex;
    /// Vertex format for all blend shapes in the Mesh. Empty if no blend shapes
    /// are present.
    VertexFormat blend_shape_format;
    /// A copy of the vertex data in |mesh_data| for all attributes in
    /// |blend_shape_format|. Empty if no blend shapes are present.
    DataContainer base_blend_shape;
    /// Vertex data for each blend shape using |blend_shape_format|.
    std::vector<DataContainer> blend_shapes;
    /// List of default blend shape weights. Overridden by NodeInfo::weights.
    std::vector<float> blend_shape_weights;

    bool HasBlendShapes() const { return !blend_shapes.empty(); }
  };

  /// Returns a mutable list of MeshInfos: one per GLTF Mesh and in the same
  /// order they appear in the GLTF. The list is mutable because the MeshFactory
  /// must take ownership of MeshData when creating usable meshes.
  std::vector<MeshInfo>& GetMutableMeshInfos() { return mesh_infos_; }

  /// Returns an immutable MeshInfo for a specific GLTF index.
  const MeshInfo& GetMeshInfo(int index) const { return mesh_infos_[index]; }

  /// Information about a GLTF Skin.
  struct SkinInfo {
    std::string name;
    /// Indices into the list of NodeInfos of the bones of this skin.
    std::vector<int> bones;
    /// Inverse bind poses for each bone.
    std::vector<mathfu::AffineTransform,
                mathfu::simd_allocator<mathfu::AffineTransform>>
        inverse_bind_matrices;
  };

  /// Returns a SkinInfo for a given index.
  const SkinInfo& GetSkinInfo(int index) const { return skin_infos_[index]; }

  /// Information about a GLTF Animation
  struct AnimationInfo {
    std::string name;
    DataContainer splines;
    size_t num_splines;
    std::unique_ptr<SkeletonChannel::AnimationContext> context;
  };

  /// Returns the AnimationInfo for the GLTF. The return is mutable because the
  /// AnimationSystem will take ownership of the RigAnim.
  std::vector<AnimationInfo>& GetMutableAnimationInfos() { return anim_infos_; }

  /// Returns a MaterialInfo for a given index.
  const MaterialInfo& GetMaterialInfo(int index) const {
    return material_infos_[index];
  }

  /// Information about a Texture referenced by the GLTF.
  struct TextureInfo {
    std::string name;
    std::string file;
    TextureParams params;
    ImageData data;
  };

  /// Returns a mutable list of TextureInfos: one per Texture referenced by the
  /// GLTF. The list is mutable because TextureFactory must take ownership of
  /// ImageData when creating usable textures.
  std::vector<TextureInfo>& GetMutableTextures() { return texture_infos_; }

 private:
  /// Functions to iterate though the various GLTF properties and create Infos
  /// for them. Infos reference other Infos by index, so all Infos must be
  /// created in the order they are present in |model|.
  void PrepareNodes(const tinygltf::Model& model);
  void PrepareMeshes(const tinygltf::Model& model);
  void PrepareSkins(const tinygltf::Model& model);
  void PrepareAnimations(const tinygltf::Model& model);
  void PrepareTextures(const tinygltf::Model& model, string_view directory);
  void PrepareMaterials(const tinygltf::Model& model);

  /// Functions to convert individual GLTF structures into individual Infos.
  /// If errors occur during parsing, these functions should simply exit and
  /// leave the Info unchanged.
  void PrepareMesh(MeshInfo* mesh_info, const tinygltf::Mesh& gltf_mesh,
                   const tinygltf::Model& model);
  void PrepareSkin(SkinInfo* skin_info, const tinygltf::Skin& gltf_skin,
                   const tinygltf::Model& model);
  void PrepareAnimation(AnimationInfo* anim_info,
                        const tinygltf::Animation& gltf_anim,
                        const tinygltf::Model& model);

  /// Converts a |gltf_primitive| into MeshData using the data in |model|.
  MeshData PreparePrimitive(const tinygltf::Primitive& gltf_primitive,
                            const tinygltf::Model& model);

  /// Converts all Morph Targets in |gltf_primitive| into blend shapes in
  /// |mesh_info| using the data in |model|.
  void PrepareBlendShapes(MeshInfo* mesh_info,
                          const tinygltf::Primitive& gltf_primitive,
                          const tinygltf::Model& model);

  /// Converts an individual Morph Target represented by |attr_map|, a map from
  /// attribute names to GLTF Accessors in |model|, into a blend shape that is
  /// appended to |mesh_info.blend_shapes|.
  void PrepareBlendShape(MeshInfo* mesh_info,
                         const std::map<std::string, int>& attr_map,
                         const tinygltf::Model& model);

  Registry* registry_;
  HashValue id_;
  bool preserve_normal_tangent_ = false;
  std::function<void()> finalize_callback_;

  std::vector<NodeInfo> node_infos_;
  std::vector<int> root_nodes_;
  std::vector<MeshInfo> mesh_infos_;
  std::vector<SkinInfo> skin_infos_;
  std::vector<AnimationInfo> anim_infos_;
  std::vector<TextureInfo> texture_infos_;
  std::vector<MaterialInfo> material_infos_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_GLTF_ASSET_GLTF_ASSET_H_
