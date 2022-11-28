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

#include "redux/tools/model_pipeline/model.h"

#include "redux/modules/flatbuffers/var.h"
#include "redux/modules/graphics/image_utils.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/model_pipeline/util.h"

namespace redux::tool {

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
    } else if (type == GetTypeId<vec2>()) {
      hash ^= std::hash<float>()(it.second.Get<vec2>()->x);
      hash ^= std::hash<float>()(it.second.Get<vec2>()->y);
    } else if (type == GetTypeId<vec3>()) {
      hash ^= std::hash<float>()(it.second.Get<vec3>()->x);
      hash ^= std::hash<float>()(it.second.Get<vec3>()->y);
      hash ^= std::hash<float>()(it.second.Get<vec3>()->z);
    } else if (type == GetTypeId<vec4>()) {
      hash ^= std::hash<float>()(it.second.Get<vec4>()->x);
      hash ^= std::hash<float>()(it.second.Get<vec4>()->y);
      hash ^= std::hash<float>()(it.second.Get<vec4>()->z);
      hash ^= std::hash<float>()(it.second.Get<vec4>()->w);
    } else {
      LOG(ERROR) << "Unknown type, bad hash: " << type;
    }
  }
  for (auto& it : material.textures) {
    hash ^= std::hash<std::string>()(it.first);
    hash ^= absl::Hash<TextureUsage>()(it.second.usage);
  }
  return static_cast<size_t>(hash);
}

// Hashes only the position, orientation, and uv0 of the Vertex. This hash
// should only be used as a first-level filter for deduplication. For actual
// deduplication, the vertices should be compared directly.
static size_t VertexHash(const Vertex& vertex) {
  uint64_t hash = 0;
  hash ^= std::hash<uint32_t>()(vertex.attribs.Value());
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

Model::Model() : Model("") {}

Model::Model(std::string name)
    : name_(std::move(name)),
      min_position_(std::numeric_limits<float>::max()),
      max_position_(std::numeric_limits<float>::lowest()) {}

bool Model::IsValid() const {
  if (vertices_.empty()) {
    return false;
  }
  if (vertex_attributes_.Empty()) {
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

void Model::SetInverseBindTransform(int bone, const mat4& inverse) {
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

void Model::AddVertex(const Vertex& vertex) {
  if (vertex_attributes_.Empty() && vertices_.empty()) {
    vertex_attributes_ = vertex.attribs;
  } else {
    CHECK(vertex_attributes_ == vertex.attribs) << "Attrib mismatch.";
  }

  if (current_drawable_ >= drawables_.size()) {
    return;
  }
  if (!vertices_.empty()) {
    const Vertex& first_vertex = vertices_[0];
    CHECK(first_vertex.blends.size() == vertex.blends.size())
        << "Blend mismatch.";
    for (size_t i = 0; i < first_vertex.blends.size(); ++i) {
      CHECK(first_vertex.blends[i].name == vertex.blends[i].name)
          << "Blend mismatch.";
    }
  }

  const size_t vertex_index = AddOrGetVertex(vertex);

  Drawable& drawable = drawables_[current_drawable_];
  drawable.indices.push_back(vertex_index);

  drawable.min_position = Min(drawable.min_position, vertex.position);
  drawable.max_position = Max(drawable.max_position, vertex.position);
}

size_t Model::AddOrGetVertex(const Vertex& vertex) {
  const size_t key = VertexHash(vertex);

  auto iter = vertex_map_.find(key);
  if (iter != vertex_map_.end()) {
    for (auto index : iter->second) {
      if (vertices_[index] == vertex) {
        return index;
      }
    }
  }

  const size_t new_index = vertices_.size();
  vertices_.push_back(vertex);

  min_position_ = Min(min_position_, vertex.position);
  max_position_ = Max(max_position_, vertex.position);
  vertex_map_[key].push_back(new_index);
  return new_index;
}

// Generates a unit vector (+ handedness) orthogonal to the given normal.
static vec4 GenerateTangent(const vec3& normal) {
  const vec3 axis = (std::fabs(Dot(normal, vec3::XAxis())) < 0.99f)
                        ? vec3::XAxis()
                        : vec3::YAxis();
  return vec4(Normalized(Cross(normal, axis)), 1.0f);
}

void Model::ComputeOrientationsFromTangentSpaces(bool ensure_w_not_zero) {
  if (vertex_attributes_.Any(Vertex::kAttribBit_Orientation)) {
    return;
  }
  if (!vertex_attributes_.Any(Vertex::kAttribBit_Normal)) {
    return;
  }

  if (ensure_w_not_zero) {
    if (vertex_attributes_.Any(Vertex::kAttribBit_Tangent)) {
      for (Vertex& vertex : vertices_) {
        vertex.orientation =
            CalculateOrientationNonZeroW(vertex.normal, vertex.tangent);
      }
    } else {
      for (Vertex& vertex : vertices_) {
        vec4 tangent = GenerateTangent(vertex.normal);
        vertex.orientation =
            CalculateOrientationNonZeroW(vertex.normal, tangent);
      }
    }
  } else {
    if (vertex_attributes_.Any(Vertex::kAttribBit_Tangent)) {
      for (Vertex& vertex : vertices_) {
        vertex.orientation =
            CalculateOrientation(vertex.normal, vertex.tangent);
      }
    } else {
      for (Vertex& vertex : vertices_) {
        vec4 tangent = GenerateTangent(vertex.normal);
        vertex.orientation = CalculateOrientation(vertex.normal, tangent);
      }
    }
  }

  vertex_attributes_.Set(Vertex::kAttribBit_Orientation);
}

Material* Model::FindMaterialByName(std::string_view name) {
  for (Drawable& d : drawables_) {
    if (d.material.name == name) {
      return &d.material;
    }
  }
  return nullptr;
}

static std::string FindTexture(Material* material,
                               const TextureConfig* config) {
  TextureUsage usage;
  if (config->usage()) {
    for (int i = 0; i < config->usage()->size() && TextureUsage::kNumChannels;
         ++i) {
      auto type = static_cast<MaterialTextureType>(config->usage()->Get(i));
      usage.channel[i] = type;
    }
  }

  if (material) {
    for (const auto& iter : material->textures) {
      if (iter.second.usage == usage) {
        return iter.first;
      }
    }
  }
  return "";
}

static void OverrideTexture(TextureInfo* texture, std::string_view uri,
                            const Model::TextureResolver& resolver) {
  const std::string resolved = resolver(uri);
  auto data = std::move(LoadFile(resolved.c_str()).value());
  texture->format = IdentifyImageTypeFromHeader(data.GetByteSpan());
  texture->data = std::make_shared<DataContainer>(std::move(data));
}

static void FinishTexture(TextureInfo* texture, const TextureConfig* config,
                          const Model::TextureResolver& resolver) {
  if (texture == nullptr) {
    return;
  }

  if (config->file_override()) {
    OverrideTexture(texture, config->file_override()->c_str(), resolver);
  }
  if (flatbuffers::IsFieldPresent(config, TextureConfig::VT_WRAP_S)) {
    texture->wrap_s = config->wrap_s();
  }
  if (flatbuffers::IsFieldPresent(config, TextureConfig::VT_WRAP_T)) {
    texture->wrap_t = config->wrap_t();
  }
  if (flatbuffers::IsFieldPresent(config,
                                  TextureConfig::VT_PREMULTIPLY_ALPHA)) {
    texture->premultiply_alpha = config->premultiply_alpha();
  }
  if (flatbuffers::IsFieldPresent(config, TextureConfig::VT_GENERATE_MIPMAPS)) {
    texture->generate_mipmaps = config->generate_mipmaps();
  }
}

static void FinishMaterial(Material* material, const MaterialConfig* opts,
                           const Model::TextureResolver& resolver) {
  if (opts->name_override() && opts->name_override()->size()) {
    material->name = opts->name_override()->str();
  }
  if (opts->textures()) {
    for (const auto texture : *opts->textures()) {
      const std::string key = FindTexture(material, texture);
      auto iter = material->textures.find(key);
      if (iter != material->textures.end()) {
        FinishTexture(&iter->second, texture, resolver);
        iter->second.export_name = iter->first;
      }
    }
  }
  if (opts->properties() && opts->properties()->values()) {
    const auto* properties = opts->properties()->values();
    for (const auto* pair : *properties) {
      if (!pair->key()) {
        continue;
      }
      const std::string key = pair->key()->name()->str();

      Var var;
      if (TryReadFbs(pair->value_type(), pair->value(), &var)) {
        material->properties.emplace(key, std::move(var));
      } else {
        material->properties.erase(key);
      }
    }
  }
}

void Model::Finish(const ModelConfig* config, const TextureResolver& resolver) {
  if (config == nullptr) {
    return;
  }

  if (config->attributes()) {
    absl::Span<const VertexUsage> attribs(
        (const VertexUsage*)config->attributes()->data(),
        config->attributes()->size());
    const Vertex::Attrib requested = Vertex::BuildAttrib(attribs);
    if (requested.Any(Vertex::kAttribBit_Orientation) &&
        !vertex_attributes_.Any(Vertex::kAttribBit_Orientation)) {
      ComputeOrientationsFromTangentSpaces(
          config->ensure_vertex_orientation_w_not_zero());
    }
    vertex_attributes_.Intersect(requested);
  }

  if (config->materials()) {
    for (const MaterialConfig* material_opts : *config->materials()) {
      if (material_opts->name()) {
        Material* material = FindMaterialByName(material_opts->name()->c_str());
        if (material) {
          FinishMaterial(material, material_opts, resolver);
        }
      }
    }
  }

  for (Drawable& drawable : drawables_) {
    for (auto& iter : drawable.material.textures) {
      TextureInfo& texture = iter.second;
      if (texture.data == nullptr) {
        const std::string resolved = resolver(iter.first);
        auto data = std::move(LoadFile(resolved.c_str()).value());
        iter.second.format = IdentifyImageTypeFromHeader(data.GetByteSpan());
        iter.second.data = std::make_shared<DataContainer>(std::move(data));
      }
    }
  }
}

}  // namespace redux::tool
