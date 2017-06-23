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

#ifndef LULLABY_SYSTEMS_RENDER_FPL_MESH_H_
#define LULLABY_SYSTEMS_RENDER_FPL_MESH_H_

#include <memory>
#include "fplbase/mesh.h"
#include "fplbase/renderer.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/base/asset.h"
#include "lullaby/util/math.h"
#include "lullaby/util/mesh_data.h"
#include "lullaby/util/triangle_mesh.h"

namespace lull {

// Owns fplbase::Mesh and provides access to functionality needed for rendering.
class Mesh {
 public:
  // A unique_ptr to the underlying fplbase::Mesh.
  typedef std::unique_ptr<fplbase::Mesh,
                          std::function<void(const fplbase::Mesh*)>>
      MeshImplPtr;

  // Wraps/owns the provided fplbase::Mesh directly.
  explicit Mesh(MeshImplPtr mesh);

  // Creates a mesh from the provided TriangleMesh.
  explicit Mesh(const TriangleMesh<VertexP>& mesh);

  // Creates a mesh from the provided TriangleMesh.
  explicit Mesh(const TriangleMesh<VertexPT>& mesh);

  // Creates a mesh from the provided MeshData.
  explicit Mesh(const MeshData& mesh);

  // Returns the number of vertices contained in the mesh.
  int GetNumVertices() const;

  // Returns the number of triangles contained in the mesh.
  int GetNumTriangles() const;

  // Gets the axis-aligned bounding box for the mesh.
  Aabb GetAabb() const;

  // Returns the number of bones.
  int GetNumBones() const;

  // Returns the number of bones in the shader.
  int GetNumShaderBones() const;

  // Returns the array of bone indices contained in the mesh.
  const uint8_t* GetBoneParents(int* num) const;

  // Returns the array of bone names.  The length of the array is GetNumBones(),
  // and 'num' will be set to this if non-null.
  const std::string* GetBoneNames(int* num) const;

  // Returns the array of default bone transform inverses (AKA inverse bind-pose
  // matrices).  The length of the array is GetNumBones(), and 'num' will be set
  // to this if non-null.
  const mathfu::AffineTransform* GetDefaultBoneTransformInverses(
      int* num) const;

  // From the mesh's |bone_transforms| (length: GetNumBones()), calculates and
  // fills the |shader_transforms| (length: GetNumShaderBones()).
  void GatherShaderTransforms(const mathfu::AffineTransform* bone_transforms,
                              mathfu::AffineTransform* shader_transforms) const;

  // Draws the mesh.
  void Render(fplbase::Renderer* renderer, fplbase::BlendMode blend_mode);

  // The FPL vertex attributes are terminated with kEND, so increase the array
  // size accordingly.
  static const int kMaxFplAttributeArraySize = VertexFormat::kMaxAttributes + 1;

  static void GetFplAttributes(
      const VertexFormat& format,
      fplbase::Attribute attributes[kMaxFplAttributeArraySize]);

  static fplbase::Mesh::Primitive GetFplPrimitiveType(
      MeshData::PrimitiveType type);

 private:
  MeshImplPtr impl_;
  int num_triangles_;
};

typedef std::shared_ptr<Mesh> MeshPtr;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FPL_MESH_H_
