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

#ifndef REDUX_SYSTEMS_MODEL_USD_ASSET_H_
#define REDUX_SYSTEMS_MODEL_USD_ASSET_H_

#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/material_data.h"
#include "redux/systems/model/model_asset.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"

namespace redux {

// Parses a USD file and extracts the relevant information for use by the
// runtime.
//
// Currently, only rendering data (i.e. meshes and materials) are supported.
class UsdAsset : public ModelAsset {
 public:
  UsdAsset(Registry* registry, std::string_view uri)
      : ModelAsset(registry, uri) {}

 protected:
  // Reads the raw binary data (stored in ModelAsset::data_) into internal
  // structures that can then be read by the ModelSystem.
  //
  // Invoked by the base ModelAsset class.
  void ProcessData() override;

 private:
  void Traverse(pxr::UsdPrim prim);

  void ProcessShader(pxr::UsdShadeShader usd_shader);
  void ProcessMaterial(pxr::UsdShadeMaterial usd_material);
  void ProcessMesh(pxr::UsdGeomMesh usd_mesh);
  void ProcessSubMesh(pxr::UsdGeomSubset usd_subset);
  TextureData ProcessTexture(pxr::UsdShadeShader usd_texture);

  pxr::UsdStageRefPtr stage_;
  std::vector<MaterialData> parsed_materials_;
  absl::flat_hash_map<std::string, int> material_lookup_;
  std::vector<std::string> mesh_materials_;
};

}  // namespace redux

#endif  // REDUX_SYSTEMS_MODEL_USD_ASSET_H_
