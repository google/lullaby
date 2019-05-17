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

#ifndef VR_INTERNAL_LULLABY_SCRIPTS_MODEL_PIPELINE_MODEL_H_
#define VR_INTERNAL_LULLABY_SCRIPTS_MODEL_PIPELINE_MODEL_H_

#include <vector>
#include "lullaby/util/bits.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "lullaby/tools/model_pipeline/bone.h"
#include "lullaby/tools/model_pipeline/drawable.h"
#include "lullaby/tools/model_pipeline/vertex.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace tool {

// Contains all the necessary information to represent a model.
//
// Different importers for different formats will return an instance of this
// class which will then be exported into a lullmodel binary file.
class Model {
 public:
  using Usage = Bits;
  static constexpr Usage kForRendering = 1 << 0;
  static constexpr Usage kForSkeleton = 1 << 1;
  static constexpr Usage kForCollision = 1 << 2;
  static constexpr Usage kForEverything = 0xffffffff;

  explicit Model(const ModelPipelineImportDefT& import_def);

  // Returns true if the model contains valid data.
  bool IsValid() const;

  // Sets the LOD level for the model.
  void SetLodLevel(int lod_level) { lod_level_ = lod_level; }

  // Sets the usage flags for this model.
  void SetUsage(Usage usage_flags) { usage_flags_ |= usage_flags; }

  // Adds a bone to the skeletal information in the model.
  int AppendBone(const Bone& bone);

  // Updates the bind transform for a given bone.
  void SetInverseBindTransform(int bone, const mathfu::mat4& inverse);

  // Internally binds the drawable with the associated material.  A Drawable
  // must be bound before vertices can be added to the model.
  void BindDrawable(const Material& material,
                    bool combine_same_materials = true);

  // Specifies that the given vertex attribute is valid for all vertices added
  // to the model.
  void EnableAttribute(Vertex::Attrib attribute);
  void DisableAttribute(Vertex::Attrib attribute);

  // Adds a vertex to the mesh in the model.  The vertex is automatically
  // associated with the drawable that was most recently "bound" by calling
  // BindDrawable.
  void AddVertex(const Vertex& vertex);

  // Returns the material with the given name.
  Material* GetMutableMaterial(const std::string& name);
  // Returns the material with the given index.
  Material* GetMutableMaterial(int index);

  // Accessors used during the export process.
  int GetLodLevel() const { return lod_level_; }
  Usage GetUsageFlags() const { return usage_flags_; }
  const ModelPipelineImportDefT& GetImportDef() const { return import_def_; }
  const mathfu::vec3& GetMinPosition() const { return min_position_; }
  const mathfu::vec3& GetMaxPosition() const { return max_position_; }
  const std::vector<Bone>& GetBones() const { return bones_; }
  const std::vector<Vertex>& GetVertices() const { return vertices_; }
  const std::vector<Drawable>& GetDrawables() const { return drawables_; }
  Vertex::Attrib GetAttributes() const { return vertex_attributes_; }
  bool CheckUsage(Bits usage) const { return CheckBit(usage_flags_, usage); }
  bool CheckAttrib(Vertex::Attrib attrib) const {
    return CheckBit(vertex_attributes_, attrib);
  }
  void AddImportedPath(const std::string& imported_file_path) {
    imported_file_paths_.emplace_back(imported_file_path);
  }
  const std::vector<std::string>& GetImportedPaths() const {
    return imported_file_paths_;
  }
  void Recenter();

  // Uses positions, normals, and tex coords to compute tangents and bitangents.
  void ComputeTangentSpacesFromNormalsAndUvs();
  // Uses normals and tangents to compute orientation quaternions. If
  // ensure_w_nonzero is true, and the computed orientation quaternion results
  // in w == 0, w will be set to a small value such that its sign can be used to
  // determine bitangent direction using the glsl method sign().
  void ComputeOrientationsFromTangentSpaces(bool ensure_w_not_zero = false);

 protected:
  size_t AddOrGetVertex(const Vertex& vertex);

  std::vector<Bone> bones_;
  std::vector<Vertex> vertices_;
  std::vector<Drawable> drawables_;
  std::vector<std::string> imported_file_paths_;
  ModelPipelineImportDefT import_def_;

  // Map of vertex hash to index in vertices_.
  std::unordered_multimap<size_t, size_t> vertex_map_;
  // Map of material hash to index in drawables_.
  std::unordered_map<size_t, size_t> drawable_map_;

  mathfu::vec3 min_position_;
  mathfu::vec3 max_position_;
  int lod_level_ = 0;
  Usage usage_flags_ = 0;
  size_t current_drawable_ = 0;
  Vertex::Attrib vertex_attributes_ = 0;
};

}  // namespace tool
}  // namespace lull

#endif  // VR_INTERNAL_LULLABY_SCRIPTS_MODEL_PIPELINE_MODEL_H_
