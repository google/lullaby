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

#ifndef REDUX_TOOLS_COMMON_ASSIMP_UTILS_
#define REDUX_TOOLS_COMMON_ASSIMP_UTILS_

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "redux/tools/common/axis_system.h"

namespace redux::tool {

// Base class for importing Open Asset Importer (assimp) scenes.
//
// Provides several other useful functions like rescaling the geometry,
// triangulating the geometry, and extracting skeletal information from the
// loaded scene.
class AssimpBaseImporter {
 public:
  AssimpBaseImporter() = default;
  virtual ~AssimpBaseImporter() = default;

 protected:
  // Options used during the import process.
  struct Options {
    bool recenter = false;
    float scale_multiplier = 0.f;
    AxisSystem axis_system = AxisSystem::Unspecified;
    float smoothing_angle = 0.f;
    int max_bone_weights = 4;
    bool flip_texture_coordinates = false;
    bool flatten_hierarchy_and_transform_vertices_to_root_space = false;
    bool fix_infacing_normals = true;
    bool optimize_mesh = false;
    // If set to true, turns off default singleton logger (which breaks thread
    // safety), but results in less verbose error messages.
    bool require_thread_safe = false;
  };

  // Loads the scene with the given path.
  bool LoadScene(const std::string& filename, const Options& options);

  // Iterates over each bone in the scene and invokes the BoneFn callback.
  using BoneFn =
      std::function<void(const aiNode*, const aiNode*, const aiMatrix4x4&)>;
  void ForEachBone(const BoneFn& fn);

  // Iterates over each material in the scene and invokes the MaterialFn
  // callback.
  using MaterialFn = std::function<void(const aiMaterial* material)>;
  void ForEachMaterial(const MaterialFn& fn);

  // Iterates over each mesh in the scene and invokes the MeshFn callback.
  using MeshFn =
      std::function<void(const aiMesh*, const aiNode*, const aiMaterial*)>;
  void ForEachMesh(const MeshFn& fn);

  // Iterates over each filename that was opened during the import process and
  // invokes the FileOpenedFn callback.
  using FileOpenedFn = std::function<void(const std::string&)>;
  void ForEachOpenedFile(const FileOpenedFn& fn);

  // Returns the internal aiScene that represents the scene.
  const aiScene* GetScene() const;

 private:
  void AddNodeToHierarchy(const aiNode* node);
  void PopulateHierarchyRecursive(const aiNode* node);

  void ReadSkeletonRecursive(const BoneFn& fn, const aiNode* node,
                             const aiNode* parent,
                             const aiMatrix4x4& base_transform);
  void ReadMeshRecursive(const MeshFn& fn, const aiNode* node);

  Assimp::Importer importer_;
  const aiScene* scene_ = nullptr;
  absl::flat_hash_set<const aiNode*> valid_nodes_;
  std::vector<std::string> imported_files_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_ASSIMP_UTILS_
