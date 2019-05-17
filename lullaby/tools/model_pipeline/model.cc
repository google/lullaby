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

#include "lullaby/tools/model_pipeline/model.h"

#include "lullaby/modules/render/tangent_generation.h"
#include "lullaby/tools/model_pipeline/util.h"

namespace lull {
namespace tool {

// Hashes the values for all the textures and properties stored in a Material.
// This should be sufficient to perform material deduplication.
static size_t MaterialHash(const Material& material) {
  uint64_t hash = 0;
  for (auto& it : material.properties) {
    hash ^= std::hash<std::string>()(it.first);
    const TypeId type = it.second.GetTypeId();
    if (type == GetTypeId<bool>()) {
      hash ^= std::hash<bool>()(*it.second.Get<bool>());
    } else if (type == GetTypeId<int>()) {
      hash ^= std::hash<int>()(*it.second.Get<int>());
    } else if (type == GetTypeId<float>()) {
      hash ^= std::hash<float>()(*it.second.Get<float>());
    } else if (type == GetTypeId<double>()) {
      hash ^= std::hash<double>()(*it.second.Get<double>());
    } else if (type == GetTypeId<std::string>()) {
      hash ^= std::hash<std::string>()(*it.second.Get<std::string>());
    } else if (type == GetTypeId<mathfu::vec2>()) {
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec2>()->x);
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec2>()->y);
    } else if (type == GetTypeId<mathfu::vec3>()) {
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec3>()->x);
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec3>()->y);
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec3>()->z);
    } else if (type == GetTypeId<mathfu::vec4>()) {
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec4>()->x);
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec4>()->y);
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec4>()->z);
      hash ^= std::hash<float>()(it.second.Get<mathfu::vec4>()->w);
    } else {
      LOG(ERROR) << "Unknown type, bad hash: " << type;
    }
  }
  for (auto& it : material.textures) {
    hash ^= std::hash<std::string>()(it.first);
    for (auto& usage : it.second.usages) {
      hash ^= std::hash<int>()(usage);
    }
  }
  return static_cast<size_t>(hash);
}

// Hashes only the position, orientation, and uv0 of the Vertex. This hash
// should only be used as a first-level filter for deduplication. For actual
// deduplication, the vertices should be compared directly.
static size_t VertexHash(const Vertex& vertex) {
  uint64_t hash = 0;
  hash ^= std::hash<float>()(vertex.position.x);
  hash ^= std::hash<float>()(vertex.position.y);
  hash ^= std::hash<float>()(vertex.position.z);
  hash ^= std::hash<float>()(vertex.orientation.x);
  hash ^= std::hash<float>()(vertex.orientation.y);
  hash ^= std::hash<float>()(vertex.orientation.z);
  hash ^= std::hash<float>()(vertex.orientation.w);
  hash ^= std::hash<float>()(vertex.uv0.x);
  hash ^= std::hash<float>()(vertex.uv0.y);
  return static_cast<size_t>(hash);
}

// Generates a unit vector (+ handedness) orthogonal to the given normal.
static mathfu::vec4 GenerateTangent(const mathfu::vec3& normal) {
  const mathfu::vec3 axis =
      (std::fabs(mathfu::dot(normal, mathfu::kAxisX3f)) < 0.99f)
          ? mathfu::kAxisX3f
          : mathfu::kAxisY3f;
  return mathfu::vec4(mathfu::normalize(mathfu::cross(normal, axis)), 1.f);
}

Model::Model(const ModelPipelineImportDefT& import_def)
    : import_def_(import_def),
      min_position_(std::numeric_limits<float>::max()),
      max_position_(std::numeric_limits<float>::lowest()) {}

bool Model::IsValid() const {
  if (vertices_.empty()) {
    return false;
  }
  if (vertex_attributes_ == 0) {
    return false;
  }
  if (drawables_.empty()) {
    return false;
  }
  return true;
}

int Model::AppendBone(const Bone& bone) {
  const int bone_index = static_cast<int>(bones_.size());
  bones_.emplace_back(bone);
  return bone_index;
}

void Model::SetInverseBindTransform(int bone, const mathfu::mat4& inverse) {
  if (static_cast<size_t>(bone) < bones_.size()) {
    bones_[bone].inverse_bind_transform = inverse;
  }
}

void Model::BindDrawable(const Material& material,
                         bool combine_same_materials) {
  const size_t key = MaterialHash(material);
  auto iter = drawable_map_.find(key);
  if (iter != drawable_map_.end() && combine_same_materials) {
    current_drawable_ = iter->second;
  } else {
    current_drawable_ = drawables_.size();
    drawable_map_.emplace(key, current_drawable_);

    drawables_.emplace_back();
    drawables_.back().material = material;
  }
}

Material* Model::GetMutableMaterial(const std::string& name) {
  for (Drawable& drawable : drawables_) {
    if (drawable.material.name == name) {
      return &drawable.material;
    }
  }
  return nullptr;
}

Material* Model::GetMutableMaterial(int index) {
  return &drawables_[index].material;
}

void Model::EnableAttribute(Vertex::Attrib attribute) {
  vertex_attributes_ = SetBit(vertex_attributes_, attribute);
}

void Model::DisableAttribute(Vertex::Attrib attribute) {
  vertex_attributes_ = ClearBit(vertex_attributes_, attribute);
}

void Model::AddVertex(const Vertex& vertex) {
  if (current_drawable_ >= drawables_.size()) {
    return;
  }
  if (!vertices_.empty()) {
    const Vertex& first_vertex = vertices_[0];
    if (first_vertex.blends.size() != vertex.blends.size()) {
      return;
    }
    for (size_t i = 0; i < first_vertex.blends.size(); ++i) {
      if (first_vertex.blends[i].name != vertex.blends[i].name) {
        return;
      }
    }
  }

  const size_t vertex_index = AddOrGetVertex(vertex);

  Drawable& drawable = drawables_[current_drawable_];
  drawable.indices.push_back(vertex_index);

  drawable.min_position_ =
      mathfu::vec3::Min(drawable.min_position_, vertex.position);
  drawable.max_position_ =
      mathfu::vec3::Max(drawable.max_position_, vertex.position);
}

size_t Model::AddOrGetVertex(const Vertex& vertex) {
  const size_t key = VertexHash(vertex);

  auto range = vertex_map_.equal_range(key);
  for (auto iter = range.first; iter != range.second; ++iter) {
    const size_t index = iter->second;
    if (vertices_[index] == vertex) {
      return index;
    }
  }

  const size_t new_index = vertices_.size();
  vertices_.push_back(vertex);

  min_position_ = mathfu::vec3::Min(min_position_, vertex.position);
  max_position_ = mathfu::vec3::Max(max_position_, vertex.position);
  vertex_map_.emplace(key, new_index);
  return new_index;
}

void Model::Recenter() {
  // TODO: Remove this placeholder method and move to
  // aiProcess_PreTransformVertices
  const mathfu::vec3 center = 0.5f * (max_position_ + min_position_);
  std::for_each(vertices_.begin(), vertices_.end(),
                [center](Vertex& vertex) { vertex.position -= center; });
  min_position_ -= center;
  max_position_ -= center;
}

void Model::ComputeTangentSpacesFromNormalsAndUvs() {
  if (!CheckAttrib(Vertex::kAttribBit_Position) ||
      !CheckAttrib(Vertex::kAttribBit_Normal) ||
      !CheckAttrib(Vertex::kAttribBit_Uv0)) {
    return;
  }

  if (!CheckAttrib(Vertex::kAttribBit_Tangent) ||
      !CheckAttrib(Vertex::kAttribBit_Bitangent)) {
    for (const Drawable& drawable : drawables_) {
      ComputeTangentsWithIndexedTriangles(
          reinterpret_cast<const uint8_t*>(&vertices_[0].position),
          sizeof(vertices_[0]),
          reinterpret_cast<const uint8_t*>(&vertices_[0].normal),
          sizeof(vertices_[0]),
          reinterpret_cast<const uint8_t*>(&vertices_[0].uv0),
          sizeof(vertices_[0]), vertices_.size(),
          reinterpret_cast<const uint8_t*>(drawable.indices.data()),
          sizeof(drawable.indices[0]), drawable.indices.size() / 3,
          reinterpret_cast<uint8_t*>(&vertices_[0].tangent),
          sizeof(vertices_[0]),
          reinterpret_cast<uint8_t*>(&vertices_[0].bitangent),
          sizeof(vertices_[0]));
    }

    EnableAttribute(Vertex::kAttribBit_Tangent);
    EnableAttribute(Vertex::kAttribBit_Bitangent);
  }
}

void Model::ComputeOrientationsFromTangentSpaces(bool ensure_w_not_zero) {
  if (CheckAttrib(Vertex::kAttribBit_Orientation)) {
    return;
  }
  if (!CheckAttrib(Vertex::kAttribBit_Normal)) {
    return;
  }

  if (ensure_w_not_zero) {
    if (CheckAttrib(Vertex::kAttribBit_Tangent)) {
      for (Vertex& vertex : vertices_) {
        vertex.orientation =
            CalculateOrientationNonZeroW(vertex.normal, vertex.tangent);
      }
    } else {
      for (Vertex& vertex : vertices_) {
        mathfu::vec4 tangent = GenerateTangent(vertex.normal);
        vertex.orientation =
            CalculateOrientationNonZeroW(vertex.normal, tangent);
      }
    }
  } else {
    if (CheckAttrib(Vertex::kAttribBit_Tangent)) {
      for (Vertex& vertex : vertices_) {
        vertex.orientation =
            CalculateOrientation(vertex.normal, vertex.tangent);
      }
    } else {
      for (Vertex& vertex : vertices_) {
        mathfu::vec4 tangent = GenerateTangent(vertex.normal);
        vertex.orientation = CalculateOrientation(vertex.normal, tangent);
      }
    }
  }

  EnableAttribute(Vertex::kAttribBit_Orientation);
}

}  // namespace tool
}  // namespace lull
