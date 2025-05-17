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

#ifndef REDUX_SYSTEMS_MODEL_GLTF_ASSET_H_
#define REDUX_SYSTEMS_MODEL_GLTF_ASSET_H_

#include <memory>
#include <string_view>
#include <vector>

#include "draco/mesh/mesh.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/math/matrix.h"
#include "redux/systems/model/model_asset.h"
#include "tinygltf/tiny_gltf.h"

namespace redux {

// Parses a GLTF file and extracts the relevant information for use by the
// runtime.
//
// Currently, only rendering data (i.e. meshes and materials) are supported.
class GltfAsset : public ModelAsset {
 public:
  GltfAsset(Registry* registry, std::string_view uri)
      : ModelAsset(registry, uri) {}

 protected:
  // Reads the raw binary data (stored in ModelAsset::data_) into internal
  // structures that can then be read by the ModelSystem.
  //
  // Invoked by the base ModelAsset class.
  void ProcessData() override;

 private:
  // Data extracted from each primitive in the GLTF.
  struct MeshPrimitiveData {
    int material_index = 0;
    mat4 transform = mat4::Identity();
  };

  // Decoded data buffers for any GLTF buffers that were Draco encoded.
  struct DracoBuffer {
    std::shared_ptr<draco::Mesh> mesh;
    DataContainer vertex_buffer;
    DataContainer index_buffer;
  };

  void PrepareMeshes();
  void PrepareMaterials();
  void PrepareTextures();

  void AppendAttribute(VertexFormat& vertex_format,
                       const tinygltf::Accessor& accessor, VertexUsage usage,
                       VertexType valid_types);

  void ProcessNodeRecursive(mat4 transform, int node_index);
  void UpdateMeshData(std::shared_ptr<MeshData> mesh_data,
                      const tinygltf::Primitive& gltf_primitive,
                      const mat4& transform);
  void UpdateDracoMeshData(std::shared_ptr<MeshData> mesh_data,
                           DracoBuffer& draco_buffer,
                           const tinygltf::Primitive& gltf_primitive,
                           const mat4& transform);
  void UpdateMaterialData(MaterialData& data,
                          const tinygltf::Material& gltf_material);

  std::shared_ptr<tinygltf::Model> model_;
  std::vector<MeshPrimitiveData> mesh_primitives_;
  std::vector<DracoBuffer> draco_buffers_;
};

}  // namespace redux

#endif  // REDUX_SYSTEMS_MODEL_GLTF_ASSET_H_
