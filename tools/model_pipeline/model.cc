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

#include "tools/model_pipeline/model.h"

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
    hash ^= std::hash<int>()(it.second.usage);
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

Model::Model(const ModelPipelineImportDefT& import_def)
    : import_def_(import_def),
      min_position_(std::numeric_limits<float>::max()),
      max_position_(std::numeric_limits<float>::lowest()) {
}

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

void Model::BindDrawable(const Material& material) {
  const size_t key = MaterialHash(material);
  auto iter = drawable_map_.find(key);
  if (iter != drawable_map_.end()) {
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

void Model::EnableAttribute(Vertex::Attrib attribute) {
  vertex_attributes_ = SetBit(vertex_attributes_, attribute);
}

void Model::DisableAttribute(Vertex::Attrib attribute) {
  vertex_attributes_ = ClearBit(vertex_attributes_, attribute);
}

int Model::AddVertex(const Vertex& vertex) {
  if (current_drawable_ >= drawables_.size()) {
    return -1;
  }
  const size_t vertex_index = AddOrGetVertex(vertex);
  drawables_[current_drawable_].indices.push_back(vertex_index);
  return static_cast<int>(vertex_index);
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

  const mathfu::vec3 pos(vertex.position.x, vertex.position.y,
                         vertex.position.z);
  min_position_ = mathfu::vec3::Min(min_position_, pos);
  max_position_ = mathfu::vec3::Max(max_position_, pos);
  vertex_map_.emplace(key, new_index);
  return new_index;
}

}  // namespace tool
}  // namespace lull
