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

#ifndef LULLABY_TOOLS_COMMON_FBX_UTILS_H_
#define LULLABY_TOOLS_COMMON_FBX_UTILS_H_

#include <fbxsdk.h>
#include <functional>
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
  struct Options {
    bool recenter = false;
    float cm_per_unit = 0.f;
    float scale_multiplier = 1.f;
    AxisSystem axis_system = AxisSystem_Unspecified;
  };

  // Loads the scene with the given path.
  bool LoadScene(const std::string& filename, const Options& options);

  // Returns the root node in the loaded scene.
  FbxNode* GetRootNode();

  // Invokes the provided function for each bone in the scene.
  using BoneFn = std::function<void(FbxNode*, FbxNode*)>;
  void ForEachBone(const BoneFn& fn);

  // Invokes the provided function for each mesh in the scene.
  using MeshFn = std::function<void(FbxNode*)>;
  void ForEachMesh(const MeshFn& fn);

  using AnimationStackFn = std::function<void(FbxAnimStack*)>;
  void ForEachAnimationStack(const AnimationStackFn& fn);

 private:
  // Gets the version of libfbxsdk being used.
  std::string GetSdkVersion() const;

  // Gets the version of the file imported by the importer.
  std::string GetFileVersion(FbxImporter* importer) const;

  // Rescales the geometry to the provided unit distance.
  void ApplyScale(float cm_per_unit, float scale_multiplier);

  // Converts the geometry from the given axis system to the Lullaby axis
  // system.
  void ConvertAxis(AxisSystem axis_system);

  // Performs default geometry conversion operations like triangulating the
  // meshes and automatically generated missing normals.
  void ConvertGeometry(bool recenter);

  // Recursive function used to navigate the scene hierarchy.
  void ConvertGeometryRecursive(FbxNode* node);

  // Recursive function that will generate a set of FbxNodes that are valid
  // bones.
  bool MarkBoneNodesRecursive(FbxNode* node, std::set<FbxNode*>* valid_bones);

  // Recursive function that registers a bone with the callback function in
  // order.
  void ProcessBonesRecursive(const BoneFn& fn, FbxNode* node, FbxNode* parent,
                             const std::set<FbxNode*>& valid_bones);

  // Recursive function that registers a mesh with the callback function.
  void ProcessMeshRecursive(const MeshFn& fn, FbxNode* node);

  FbxManager* manager_;
  FbxScene* scene_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_COMMON_FBX_UTILS_H_
