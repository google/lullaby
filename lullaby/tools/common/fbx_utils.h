/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_TOOLS_COMMON_FBX_UTILS_H_
#define LULLABY_TOOLS_COMMON_FBX_UTILS_H_

#include <fbxsdk.h>
#include "lullaby/generated/axis_system_generated.h"

namespace lull {
namespace tool {

// Base class for importing FBX scenes.
//
// Provides several other useful functions like rescaling the geometry,
// triangulating the geometry, and extracting skeletal information from the
// loaded scene.
class FbxBaseImporter {
 public:
  FbxBaseImporter();
  virtual ~FbxBaseImporter();

 protected:
  // Information about a single "bone" in the skeletal hierarchy.
  static const int kInvalidBoneIndex = -1;
  struct BoneInfo {
    BoneInfo(FbxNode* node, int parent_index)
        : node(node), parent_index(parent_index) {}
    FbxNode* node = nullptr;
    int parent_index = kInvalidBoneIndex;
  };

  // Loads the scene with the given path.
  bool LoadScene(const std::string& filename);

  // Rescales the geometry to the provided unit distance.
  void ConvertFbxScale(float distance_unit);

  // Converts the geometry from the given axis system to the Lullaby axis
  // system.
  void ConvertFbxAxes(AxisSystem axis_system);

  // Performs default geometry conversion operations like triangulating the
  // meshes and automatically generated missing normals.
  void ConvertGeometry(bool recenter);

  // Returns the hierarchical list of bones in the scene.
  std::vector<BoneInfo> BuildBoneList();

  // Returns the root node in the loaded scene.
  FbxNode* GetRootNode();

 private:
  // Gets the version of libfbxsdk being used.
  std::string GetSdkVersion() const;

  // Gets the version of the file imported by the importer.
  std::string GetFileVersion(FbxImporter* importer) const;

  // Recursive functions used to navigate the scene hierarchy.
  void ConvertGeometryRecursive(FbxNode* node);
  bool MarkBoneNodesRecursive(FbxNode* node, std::set<FbxNode*>* valid_nodes);
  void BuildBonesRecursive(const BoneInfo& bone,
                           const std::set<FbxNode*>& valid_nodes,
                           std::vector<BoneInfo>* bones);

  FbxManager* manager_;
  FbxScene* scene_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_COMMON_FBX_UTILS_H_
