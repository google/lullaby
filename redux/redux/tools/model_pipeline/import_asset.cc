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

#include "assimp/GltfMaterial.h"
#include "assimp/material.h"
#include "assimp/scene.h"
#include "redux/modules/graphics/image_utils.h"
#include "redux/tools/common/assimp_utils.h"
#include "redux/tools/model_pipeline/config_generated.h"
#include "redux/tools/model_pipeline/model.h"
#include "redux/tools/model_pipeline/util.h"

namespace redux::tool {

static vec3 Convert(const aiVector3D& vec) {
  return {static_cast<float>(vec.x), static_cast<float>(vec.y),
          static_cast<float>(vec.z)};
}

static vec4 Convert(const aiColor4D& color) {
  return {static_cast<float>(color.r), static_cast<float>(color.g),
          static_cast<float>(color.b), static_cast<float>(color.a)};
}

static mat4 Convert(const aiMatrix4x4& mat) {
  return {static_cast<float>(mat.a1), static_cast<float>(mat.a2),
          static_cast<float>(mat.a3), static_cast<float>(mat.a4),
          static_cast<float>(mat.b1), static_cast<float>(mat.b2),
          static_cast<float>(mat.b3), static_cast<float>(mat.b4),
          static_cast<float>(mat.c1), static_cast<float>(mat.c2),
          static_cast<float>(mat.c3), static_cast<float>(mat.c4),
          static_cast<float>(mat.d1), static_cast<float>(mat.d2),
          static_cast<float>(mat.d3), static_cast<float>(mat.d4)};
}

static vec2 ConvertUv(const aiVector3D& vec) {
  return {static_cast<float>(vec.x), static_cast<float>(vec.y)};
}

static vec4 ConvertTangent(const aiVector3D& normal, const aiVector3D& tangent,
                           const aiVector3D& bitangent) {
  const vec3 n(normal.x, normal.y, normal.z);
  const vec3 t(tangent.x, tangent.y, tangent.z);
  const vec3 b(bitangent.x, bitangent.y, bitangent.z);

  const vec3 n2 = Cross(t, b).Normalized();
  const bool sign = std::signbit(Dot(n2, n));
  return {static_cast<float>(tangent.x), static_cast<float>(tangent.y),
          static_cast<float>(tangent.z), sign ? -1.0f : 1.0f};
}

static TextureWrap ConvertTextureWrapMode(const aiTextureMapMode& mode) {
  switch (mode) {
    case aiTextureMapMode_Wrap:
      return TextureWrap::Repeat;
    case aiTextureMapMode_Clamp:
      return TextureWrap::ClampToEdge;
    case aiTextureMapMode_Mirror:
      return TextureWrap::MirroredRepeat;
    default:
      LOG(ERROR) << "Unsupported wrap mode: " << mode;
      return TextureWrap::Repeat;
  }
}

static void ReadStringProperty(const aiMaterial* src, Material* dst,
                               const char* src_name, int a1, int a2,
                               const char* dst_name) {
  aiString value;
  if (src->Get(src_name, a1, a2, value) == AI_SUCCESS) {
    dst->properties[dst_name] = std::string(value.C_Str());
  }
}

static void ReadFloatProperty(const aiMaterial* src, Material* dst,
                              const char* src_name, int a1, int a2,
                              const char* dst_name) {
  float value;
  if (src->Get(src_name, a1, a2, value) == AI_SUCCESS) {
    dst->properties[dst_name] = value;
  }
}

static void ReadColorProperty(const aiMaterial* src, Material* dst,
                              const char* src_name, int a1, int a2,
                              const char* dst_name) {
  aiColor4D value4;
  if (src->Get(src_name, a1, a2, value4) == AI_SUCCESS) {
    dst->properties[dst_name] = vec4(value4.r, value4.g, value4.b, value4.a);
    return;
  }
  aiColor3D value3;
  if (src->Get(src_name, a1, a2, value3) == AI_SUCCESS) {
    dst->properties[dst_name] = vec4(value3.r, value3.g, value3.b, 1.0f);
    return;
  }
}

class AssetImporter : public AssimpBaseImporter {
 public:
  explicit AssetImporter(const ModelConfig& config) : config_(config) {}
  ModelPtr Import();

 private:
  void ReadMaterial(const aiMaterial* src, Material* dst);
  void ReadShadingModel(const aiMaterial* src, Material* dst);
  void ReadTexture(const aiMaterial* src, Material* dst, aiTextureType src_type,
                   unsigned int index, TextureUsage usage);

  void ReadMesh(const aiNode* node, const aiMesh* src);
  void AddVertex(const aiNode* node, const aiMesh* src, int index);
  std::vector<Vertex::Influence> GatherInfluences(const aiMesh* src, int index);

  const ModelConfig& config_;
  ModelPtr model_ = nullptr;
  absl::flat_hash_map<const aiNode*, int> hierarchy_;
  std::vector<Material> materials_;
};

std::vector<Vertex::Influence> AssetImporter::GatherInfluences(
    const aiMesh* src, int index) {
  std::vector<Vertex::Influence> influences;
  for (int i = 0; i < src->mNumBones; ++i) {
    const aiBone* bone = src->mBones[i];
    for (int j = 0; j < bone->mNumWeights; ++j) {
      const aiVertexWeight& weight = bone->mWeights[j];
      if (weight.mVertexId == index) {
        for (auto& iter : hierarchy_) {
          if (iter.first->mName == bone->mName) {
            influences.emplace_back(iter.second, weight.mWeight);
          }
        }
      }
    }
  }
  return influences;
}

void AssetImporter::AddVertex(const aiNode* node, const aiMesh* src,
                              int index) {
  const float global_scale = config_.scale();
  Vertex vertex;
  if (src->HasPositions()) {
    vertex.attribs.Set(Vertex::kAttribBit_Position);
    vertex.position = global_scale * Convert(src->mVertices[index]);
  }
  if (src->HasNormals()) {
    vertex.attribs.Set(Vertex::kAttribBit_Normal);
    vertex.normal = Convert(src->mNormals[index]);
  }
  if (src->HasTangentsAndBitangents()) {
    vertex.attribs.Set(Vertex::kAttribBit_Tangent);
    vertex.tangent = ConvertTangent(src->mNormals[index], src->mTangents[index],
                                    src->mBitangents[index]);
  }
  if (src->mColors[0]) {
    vertex.attribs.Set(Vertex::kAttribBit_Color0);
    vertex.color0 = Convert(src->mColors[0][index]);
  }
  if (src->mColors[1]) {
    vertex.attribs.Set(Vertex::kAttribBit_Color1);
    vertex.color1 = Convert(src->mColors[1][index]);
  }
  if (src->mColors[2]) {
    vertex.attribs.Set(Vertex::kAttribBit_Color2);
    vertex.color2 = Convert(src->mColors[2][index]);
  }
  if (src->mColors[3]) {
    vertex.attribs.Set(Vertex::kAttribBit_Color3);
    vertex.color3 = Convert(src->mColors[3][index]);
  }
  if (src->mTextureCoords[0]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv0);
    vertex.uv0 = ConvertUv(src->mTextureCoords[0][index]);
  }
  if (src->mTextureCoords[1]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv1);
    vertex.uv1 = ConvertUv(src->mTextureCoords[1][index]);
  }
  if (src->mTextureCoords[2]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv2);
    vertex.uv2 = ConvertUv(src->mTextureCoords[2][index]);
  }
  if (src->mTextureCoords[3]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv3);
    vertex.uv3 = ConvertUv(src->mTextureCoords[3][index]);
  }
  if (src->mTextureCoords[4]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv4);
    vertex.uv4 = ConvertUv(src->mTextureCoords[4][index]);
  }
  if (src->mTextureCoords[5]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv5);
    vertex.uv5 = ConvertUv(src->mTextureCoords[5][index]);
  }
  if (src->mTextureCoords[6]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv6);
    vertex.uv6 = ConvertUv(src->mTextureCoords[6][index]);
  }
  if (src->mTextureCoords[7]) {
    vertex.attribs.Set(Vertex::kAttribBit_Uv7);
    vertex.uv7 = ConvertUv(src->mTextureCoords[7][index]);
  }

  vertex.influences = GatherInfluences(src, index);
  if (!vertex.influences.empty()) {
    vertex.attribs.Set(Vertex::kAttribBit_Influences);
  } else {
    auto iter = hierarchy_.find(node);
    if (iter != hierarchy_.end()) {
      vertex.influences.emplace_back(iter->second, 1.0f);
    }
  }

  model_->AddVertex(vertex);
}

void AssetImporter::ReadShadingModel(const aiMaterial* src, Material* dst) {
  int model = 0;
  const auto res = src->Get(AI_MATKEY_SHADING_MODEL, model);
  if (res != AI_SUCCESS) {
    LOG(ERROR) << "Unable to determine shading model. Defaulting to Phong.";
    dst->properties["ShadingModel"] = std::string("Phong");
    return;
  }
  switch (model) {
    case aiShadingMode_NoShading:
      dst->properties["ShadingModel"] = std::string("flat");
      break;
    case aiShadingMode_Flat:
      dst->properties["ShadingModel"] = std::string("flat");
      break;
    case aiShadingMode_Gouraud:
      dst->properties["ShadingModel"] = std::string("gouraud");
      break;
    case aiShadingMode_Phong:
      dst->properties["ShadingModel"] = std::string("phong");
      break;
    case aiShadingMode_PBR_BRDF:
      dst->properties["ShadingModel"] = std::string("metallic_roughness");
      break;
    default:
      LOG(ERROR) << "Unknown shading model: " << model;
      dst->properties["ShadingModel"] = std::string("phong");
      break;
  }
}

void AssetImporter::ReadTexture(const aiMaterial* src, Material* dst,
                                aiTextureType src_type, unsigned int index,
                                TextureUsage usage) {
  auto texture_count = src->GetTextureCount(src_type);
  if (texture_count == 0) {
    return;
  }

  aiString path;
  aiTextureMapMode src_modes[] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
  const auto res = src->GetTexture(src_type, index, &path, nullptr, nullptr,
                                   nullptr, nullptr, src_modes);
  if (res != AI_SUCCESS) {
    LOG(ERROR) << "Unable to get texture information.";
    return;
  }

  std::string name = path.C_Str();
  auto iter = dst->textures.find(name);
  if (iter != dst->textures.end()) {
    TextureUsage& src = iter->second.usage;
    for (int i = 0; i < TextureUsage::kNumChannels; ++i) {
      if (src.channel[i] == MaterialTextureType::Unspecified) {
        src.channel[i] = usage.channel[i];
      } else if (usage.channel[i] != MaterialTextureType::Unspecified) {
        CHECK(src.channel[i] == usage.channel[i]);
      }
    }
  } else {
    TextureInfo info;
    info.usage = usage;
    info.wrap_s = ConvertTextureWrapMode(src_modes[0]);
    info.wrap_t = ConvertTextureWrapMode(src_modes[1]);
    info.export_name = name;

    // Embedded textures from Assimp are named "*#" where # is a decimal number
    // corresponding to a texture index.
    if (path.length > 1 && path.data[0] == '*') {
      char* path_end = path.data + path.length;
      uint32_t embedded_index = std::strtoul(path.data + 1, &path_end, 10);
      const aiTexture* texture = GetScene()->mTextures[embedded_index];

      // If mHeight is 0, it implies that the embedded texture is compressed and
      // the total size is stored in mWidth. Otherwise, the embedded data is
      // RGBA data.
      bool is_compressed = texture->mHeight == 0;
      const size_t num_texels = is_compressed
                                    ? texture->mWidth / sizeof(aiTexel)
                                    : texture->mHeight * texture->mWidth;
      auto data = DataContainer::WrapData(texture->pcData, num_texels);
      info.format = is_compressed
                        ? IdentifyImageTypeFromHeader(data.GetByteSpan())
                        : ImageFormat::Rgba8888;
      info.data = std::make_shared<DataContainer>(data.Clone());
      info.size.x = is_compressed ? 0 : texture->mWidth;
      info.size.y = is_compressed ? 0 : texture->mHeight;
      CHECK(info.format != ImageFormat::Invalid);
    }
    dst->textures.emplace(std::move(name), std::move(info));
  }
}

void AssetImporter::ReadMaterial(const aiMaterial* src, Material* dst) {
  auto has_gltf_specular_glossiness = false;
  src->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS, has_gltf_specular_glossiness);
  auto should_use_specular_glossiness =
      config_.use_specular_glossiness_textures_if_present() &&
      has_gltf_specular_glossiness;

  if (should_use_specular_glossiness) {
    dst->properties["UsesSpecularGlossiness"] = true;
  }

  ReadShadingModel(src, dst);
  ReadStringProperty(src, dst, AI_MATKEY_NAME, "Name");
  ReadStringProperty(src, dst, AI_MATKEY_GLTF_ALPHAMODE, "AlphaMode");
  if (dst->properties.find("AlphaMode") != dst->properties.end()) {
    dst->properties["IsOpaque"] = dst->properties["AlphaMode"].ValueOr(
                                      std::string()) == std::string("OPAQUE");
  }
  int is_double_sided = 0;
  if (src->Get(AI_MATKEY_TWOSIDED, is_double_sided) == AI_SUCCESS) {
    dst->properties["IsDoubleSided"] = is_double_sided;
  }
  ReadFloatProperty(src, dst, AI_MATKEY_GLTF_ALPHACUTOFF, "AlphaCutoff");
  ReadFloatProperty(src, dst, AI_MATKEY_OPACITY, "Opacity");
  ReadFloatProperty(src, dst, AI_MATKEY_BUMPSCALING, "BumpScaling");
  ReadFloatProperty(src, dst, AI_MATKEY_REFLECTIVITY, "Reflectivity");
  ReadFloatProperty(src, dst, AI_MATKEY_SHININESS, "Shininess");
  ReadFloatProperty(src, dst, AI_MATKEY_SHININESS_STRENGTH,
                    "ShininessStrength");
  ReadFloatProperty(src, dst, AI_MATKEY_REFRACTI, "RefractiveIndex");

  // Conditially reads in either specular-glossiness or metallic-roughness
  // factors depending on configuration.
  if (should_use_specular_glossiness) {
    ReadFloatProperty(src, dst, AI_MATKEY_GLOSSINESS_FACTOR, "Glossiness");
  } else {
    ReadFloatProperty(src, dst, AI_MATKEY_METALLIC_FACTOR, "Metallic");
    ReadFloatProperty(src, dst, AI_MATKEY_ROUGHNESS_FACTOR, "Roughness");
  }

  ReadColorProperty(src, dst, AI_MATKEY_COLOR_DIFFUSE, "DiffuseColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_AMBIENT, "AmbientColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_SPECULAR, "SpecularColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_EMISSIVE, "EmissiveColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_REFLECTIVE, "ReflectiveColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_TRANSPARENT, "TransparentColor");
  ReadColorProperty(src, dst, AI_MATKEY_BASE_COLOR, "BaseColor");

  // If configured to use specular-glossines, reads in diffuse type as diffuse
  // usage. Otherwise, reads in metallic-roughness textures. If both
  // specular-glossiness and metallic-roughness textures are available, the base
  // color texture should be used for the base color usage. Otherwise, the
  // diffuse type will have the base color usage in it.
  if (should_use_specular_glossiness) {
    ReadTexture(src, dst, aiTextureType_DIFFUSE, 0,
                TextureUsage(MaterialTextureType::BaseColor));
  } else {
    if (has_gltf_specular_glossiness) {
      ReadTexture(src, dst, AI_MATKEY_BASE_COLOR_TEXTURE,
                  TextureUsage(MaterialTextureType::BaseColor));
    } else {
      ReadTexture(src, dst, aiTextureType_DIFFUSE, 0,
                  TextureUsage(MaterialTextureType::BaseColor));
    }
    ReadTexture(
        src, dst, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
        {MaterialTextureType::Unspecified, MaterialTextureType::Roughness,
         MaterialTextureType::Metallic});
  }

  ReadTexture(src, dst, aiTextureType_HEIGHT, 0,
              TextureUsage(MaterialTextureType::Height));
  ReadTexture(src, dst, aiTextureType_NORMALS, 0,
              TextureUsage(MaterialTextureType::Normal));
  ReadTexture(src, dst, aiTextureType_SPECULAR, 0,
              TextureUsage(MaterialTextureType::Specular));
  ReadTexture(src, dst, aiTextureType_LIGHTMAP, 0,
              TextureUsage(MaterialTextureType::Occlusion));
  ReadTexture(src, dst, aiTextureType_DISPLACEMENT, 0,
              TextureUsage(MaterialTextureType::Bump));
  ReadTexture(src, dst, aiTextureType_EMISSIVE, 0,
              TextureUsage(MaterialTextureType::Emissive));
  ReadTexture(src, dst, aiTextureType_REFLECTION, 0,
              TextureUsage(MaterialTextureType::Reflection));

  // Read the name from the property table.
  dst->name = dst->properties["Name"].ValueOr(std::string());
}

ModelPtr AssetImporter::Import() {
  AssimpBaseImporter::Options opts;
  opts.recenter = config_.recenter();
  opts.axis_system = config_.axis_system();
  opts.scale_multiplier = config_.scale();
  opts.smoothing_angle = config_.smoothing_angle();
  opts.max_bone_weights = config_.max_bone_weights();
  opts.flip_texture_coordinates = config_.flip_texture_coordinates();
  opts.flatten_hierarchy_and_transform_vertices_to_root_space =
      opts.flatten_hierarchy_and_transform_vertices_to_root_space;

  CHECK(config_.uri());
  const bool success = LoadScene(config_.uri()->c_str(), opts);
  if (success) {
    model_ = std::make_shared<Model>(config_.uri()->str());
    absl::flat_hash_map<const aiMaterial*, size_t> material_map;

    ForEachMaterial([&](const aiMaterial* material) {
      material_map[material] = materials_.size();
      materials_.emplace_back();
      ReadMaterial(material, &materials_.back());
    });

    ForEachBone([&](const aiNode* node, const aiNode* parent,
                    const aiMatrix4x4& transform) {
      const std::string name = node->mName.C_Str();
      auto iter = hierarchy_.find(parent);
      const int parent_index =
          iter == hierarchy_.end() ? Bone::kInvalidBoneIndex : iter->second;

      const Bone bone(name, parent_index, Convert(transform).Inversed());
      const int index = model_->AppendBone(bone);
      hierarchy_[node] = index;
    });

    ForEachMesh([=](const aiMesh* mesh, const aiNode* node,
                    const aiMaterial* material) {
      auto iter = material_map.find(material);
      if (iter == material_map.end()) {
        return;
      }

      if (!IsValidMesh(config_, node->mName.C_Str())) {
        return;
      }

      model_->BindDrawable(materials_[iter->second]);
      for (int face_index = 0; face_index < mesh->mNumFaces; ++face_index) {
        const aiFace& face = mesh->mFaces[face_index];
        if (face.mNumIndices != 3) {
          // Points and lines are serialized as faces with < 3 vertices.
          continue;
        }

        for (int j = 0; j < face.mNumIndices; ++j) {
          const int vertex_index = face.mIndices[j];
          AddVertex(node, mesh, vertex_index);
        }
      }
    });
  }
  return model_;
}

ModelPtr ImportAsset(const ModelConfig& config) {
  AssetImporter importer(config);
  return importer.Import();
}

}  // namespace redux::tool
