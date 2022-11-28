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

#ifndef REDUX_TOOLS_COMMON_FBX_UTILS_
#define REDUX_TOOLS_COMMON_FBX_UTILS_

#include <fbxsdk.h>

#include <functional>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"
#include "redux/tools/common/axis_system.h"

namespace redux::tool {

// Base class for importing FBX scenes.
//
// Provides several other useful functions like rescaling the geometry,
// triangulating the geometry, and extracting skeletal information from the
// loaded scene.
class FbxBaseImporter {
 public:
  FbxBaseImporter();
  virtual ~FbxBaseImporter();

  static vec4 Vec4FromFbx(const FbxColor& v);
  static vec4 Vec4FromFbx(const FbxVector4& v);
  static vec3 Vec3FromFbx(const FbxVector4& v);
  static vec2 Vec2FromFbx(const FbxVector2& v);
  static quat QuatFromFbx(const FbxQuaternion& q);
  static mat4 Mat4FromFbx(const FbxAMatrix& m);

 protected:
  struct Options {
    bool recenter = false;
    float cm_per_unit = 0.f;
    float scale_multiplier = 1.f;
    AxisSystem axis_system = AxisSystem::Unspecified;
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
  bool MarkBoneNodesRecursive(FbxNode* node,
                              absl::flat_hash_set<FbxNode*>* valid_bones);

  // Recursive function that registers a bone with the callback function in
  // order.
  void ProcessBonesRecursive(const BoneFn& fn, FbxNode* node, FbxNode* parent,
                             const absl::flat_hash_set<FbxNode*>& valid_bones);

  // Recursive function that registers a mesh with the callback function.
  void ProcessMeshRecursive(const MeshFn& fn, FbxNode* node);

  FbxManager* manager_;
  FbxScene* scene_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_FBX_UTILS_
