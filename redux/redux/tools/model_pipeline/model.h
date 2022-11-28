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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_MODEL_H_
#define REDUX_TOOLS_MODEL_PIPELINE_MODEL_H_

#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/math/vector.h"
#include "redux/tools/model_pipeline/bone.h"
#include "redux/tools/model_pipeline/config_generated.h"
#include "redux/tools/model_pipeline/drawable.h"
#include "redux/tools/model_pipeline/vertex.h"

namespace redux::tool {

// Contains all the necessary information to represent a model.
//
// Different importers for different formats will return an instance of this
// class which will then be exported into a rxmodel binary file.
class Model {
 public:
  Model();
  explicit Model(std::string name);

  // Returns true if the model contains valid data.
  bool IsValid() const;

  // Adds a bone to the skeletal information in the model.
  int AppendBone(const Bone& bone);

  // Updates the bind transform for a given bone.
  void SetInverseBindTransform(int bone, const mat4& inverse);

  // Internally binds the drawable with the associated material.  A Drawable
  // must be bound before vertices can be added to the model.
  void BindDrawable(const Material& material,
                    bool combine_same_materials = true);

  // Adds a vertex to the mesh in the model.  The vertex is automatically
  // associated with the drawable that was most recently "bound" by calling
  // BindDrawable.
  void AddVertex(const Vertex& vertex);

  // Applies the configuration to the model to get it ready for export.
  using TextureResolver = std::function<std::string(std::string_view)>;
  void Finish(const ModelConfig* config, const TextureResolver& resolver);

  // Accessors used during the export process.
  const std::string& GetName() const { return name_; }
  const vec3& GetMinPosition() const { return min_position_; }
  const vec3& GetMaxPosition() const { return max_position_; }
  const std::vector<Bone>& GetBones() const { return bones_; }
  const std::vector<Vertex>& GetVertices() const { return vertices_; }
  const std::vector<Drawable>& GetDrawables() const { return drawables_; }
  const Vertex::Attrib GetAttribs() const { return vertex_attributes_; }

 protected:
  size_t AddOrGetVertex(const Vertex& vertex);

  // Uses normals and tangents to compute orientation quaternions. If
  // ensure_w_nonzero is true, and the computed orientation quaternion results
  // in w == 0, w will be set to a small value such that its sign can be used to
  // determine bitangent direction using the glsl method sign().
  void ComputeOrientationsFromTangentSpaces(bool ensure_w_not_zero);

  Material* FindMaterialByName(std::string_view name);

  std::string name_;
  std::vector<Bone> bones_;
  std::vector<Vertex> vertices_;
  std::vector<Drawable> drawables_;

  // Map of vertex hash to index in vertices_.
  absl::flat_hash_map<size_t, std::vector<size_t>> vertex_map_;
  // Map of material hash to index in drawables_.
  absl::flat_hash_map<size_t, size_t> drawable_map_;

  vec3 min_position_;
  vec3 max_position_;
  size_t current_drawable_ = 0;
  Vertex::Attrib vertex_attributes_;
};

using ModelPtr = std::shared_ptr<Model>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_MODEL_H_
