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

#include "assimp/DefaultIOSystem.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "lullaby/tools/common/assimp_base_importer.h"
#include "lullaby/tools/model_pipeline/model.h"
#include "lullaby/tools/model_pipeline/util.h"

namespace lull {
namespace tool {

// These property names are not exposed publicly by Assimp, but are needed to
// extract PBR material properties from GLTF files.
const char* AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR =
    "$mat.gltf.pbrMetallicRoughness.baseColorFactor";
const char* AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR =
    "$mat.gltf.pbrMetallicRoughness.metallicFactor";
const char* AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR =
    "$mat.gltf.pbrMetallicRoughness.roughnessFactor";
const char* AI_MATKEY_GLTF_ALPHAMODE = "$mat.gltf.alphaMode";
const char* AI_MATKEY_GLTF_ALPHACUTOFF = "$mat.gltf.alphaCutoff";
const char* AI_MATKEY_GLTF_UNLIT = "$mat.gltf.unlit";
#define AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE \
  aiTextureType_DIFFUSE, 1
#define AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS \
  "$mat.gltf.pbrSpecularGlossiness", 0, 0
#define AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS_GLOSSINESS_FACTOR \
  "$mat.gltf.pbrMetallicRoughness.glossinessFactor", 0, 0
#define AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE \
  aiTextureType_UNKNOWN, 0

static mathfu::vec3 Convert(const aiVector3D& vec) {
  return {vec.x, vec.y, vec.z};
}

static mathfu::vec4 Convert(const aiColor4D& color) {
  return {color.r, color.g, color.b, color.a};
}

static mathfu::mat4 Convert(const aiMatrix4x4& mat) {
  return {
      mat.a1, mat.b1, mat.c1, mat.d1, mat.a2, mat.b2, mat.c2, mat.d2,
      mat.a3, mat.b3, mat.c3, mat.d3, mat.a4, mat.b4, mat.c4, mat.d4,
  };
}

static mathfu::vec2 ConvertUv(const aiVector3D& vec) { return {vec.x, vec.y}; }

static mathfu::vec4 ConvertTangent(const aiVector3D& normal,
                                   const aiVector3D& tangent,
                                   const aiVector3D& bitangent) {
  const mathfu::vec3 n(normal.x, normal.y, normal.z);
  const mathfu::vec3 t(tangent.x, tangent.y, tangent.z);
  const mathfu::vec3 b(bitangent.x, bitangent.y, bitangent.z);

  const mathfu::vec3 n2 = mathfu::vec3::CrossProduct(t, b).Normalized();
  const bool sign = std::signbit(mathfu::vec3::DotProduct(n2, n));
  return {tangent.x, tangent.y, tangent.z, sign ? -1.f : 1.f};
}

static TextureWrap ConvertTextureWrapMode(const aiTextureMapMode& mode) {
  switch (mode) {
    case aiTextureMapMode_Wrap:
      return TextureWrap_Repeat;
    case aiTextureMapMode_Clamp:
      return TextureWrap_ClampToEdge;
    case aiTextureMapMode_Mirror:
      return TextureWrap_MirroredRepeat;
    default:
      LOG(ERROR) << "Unsupported wrap mode: " << mode;
      return TextureWrap_Repeat;
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
    dst->properties[dst_name] =
        mathfu::vec4(value4.r, value4.g, value4.b, value4.a);
    return;
  }
  aiColor3D value3;
  if (src->Get(src_name, a1, a2, value3) == AI_SUCCESS) {
    dst->properties[dst_name] = mathfu::vec4(value3.r, value3.g, value3.b, 1.f);
    return;
  }
}

class AssetImporter : public AssimpBaseImporter {
 public:
  AssetImporter() {}
  Model Import(const ModelPipelineImportDefT& import_def);

 private:
  void ReadMaterial(const aiMaterial* src, Material* dst);
  void ReadShadingModel(const aiMaterial* src, Material* dst);
  void ReadTexture(const aiMaterial* src, Material* dst, aiTextureType src_type,
                   unsigned int index, MaterialTextureUsage usage);

  void ReadMesh(const aiNode* node, const aiMesh* src);
  void AddVertex(const aiNode* node, const aiMesh* src, int index);
  std::vector<Vertex::Influence> GatherInfluences(const aiMesh* src, int index);

  Model* model_ = nullptr;
  std::unordered_map<const aiNode*, int> hierarchy_;
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
  const float global_scale = this->model_->GetImportDef().scale;
  Vertex vertex;
  if (src->HasPositions()) {
    model_->EnableAttribute(Vertex::kAttribBit_Position);
    vertex.position = global_scale * Convert(src->mVertices[index]);
  }
  if (src->HasNormals()) {
    model_->EnableAttribute(Vertex::kAttribBit_Normal);
    vertex.normal = Convert(src->mNormals[index]);
  }
  if (src->HasTangentsAndBitangents()) {
    model_->EnableAttribute(Vertex::kAttribBit_Tangent);
    vertex.tangent = ConvertTangent(src->mNormals[index], src->mTangents[index],
                                    src->mBitangents[index]);
  }
  if (src->mColors[0]) {
    model_->EnableAttribute(Vertex::kAttribBit_Color0);
    vertex.color0 = Convert(src->mColors[0][index]);
  }
  if (src->mColors[1]) {
    model_->EnableAttribute(Vertex::kAttribBit_Color1);
    vertex.color1 = Convert(src->mColors[1][index]);
  }
  if (src->mColors[2]) {
    model_->EnableAttribute(Vertex::kAttribBit_Color2);
    vertex.color2 = Convert(src->mColors[2][index]);
  }
  if (src->mColors[3]) {
    model_->EnableAttribute(Vertex::kAttribBit_Color3);
    vertex.color3 = Convert(src->mColors[3][index]);
  }
  if (src->mTextureCoords[0]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv0);
    vertex.uv0 = ConvertUv(src->mTextureCoords[0][index]);
  }
  if (src->mTextureCoords[1]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv1);
    vertex.uv1 = ConvertUv(src->mTextureCoords[1][index]);
  }
  if (src->mTextureCoords[2]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv2);
    vertex.uv2 = ConvertUv(src->mTextureCoords[2][index]);
  }
  if (src->mTextureCoords[3]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv3);
    vertex.uv3 = ConvertUv(src->mTextureCoords[3][index]);
  }
  if (src->mTextureCoords[4]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv4);
    vertex.uv4 = ConvertUv(src->mTextureCoords[4][index]);
  }
  if (src->mTextureCoords[5]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv5);
    vertex.uv5 = ConvertUv(src->mTextureCoords[5][index]);
  }
  if (src->mTextureCoords[6]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv6);
    vertex.uv6 = ConvertUv(src->mTextureCoords[6][index]);
  }
  if (src->mTextureCoords[7]) {
    model_->EnableAttribute(Vertex::kAttribBit_Uv7);
    vertex.uv7 = ConvertUv(src->mTextureCoords[7][index]);
  }

  vertex.influences = GatherInfluences(src, index);
  if (!vertex.influences.empty()) {
    model_->EnableAttribute(Vertex::kAttribBit_Influences);
  } else {
    auto iter = hierarchy_.find(node);
    if (iter != hierarchy_.end()) {
      vertex.influences.emplace_back(iter->second, 1.f);
    }
  }

  model_->AddVertex(vertex);
}

void AssetImporter::ReadShadingModel(const aiMaterial* src, Material* dst) {
  const std::string& file_name = model_->GetImportDef().file;
  if (EndsWith(file_name, ".gltf") || EndsWith(file_name, ".glb")) {
    bool unlit;
    if (src->Get(AI_MATKEY_GLTF_UNLIT, 0, 0, unlit) == AI_SUCCESS && unlit) {
      dst->properties["ShadingModel"] = std::string("Unlit");
    } else {
      dst->properties["ShadingModel"] = std::string("Pbr");
    }
    return;
  }

  int model = 0;
  const auto res = src->Get(AI_MATKEY_SHADING_MODEL, model);
  if (res != AI_SUCCESS) {
    LOG(ERROR) << "Unable to determine shading model. Defaulting to Phong.";
    dst->properties["ShadingModel"] = std::string("Phong");
    return;
  }
  switch (model) {
    case aiShadingMode_NoShading:
      dst->properties["ShadingModel"] = std::string("None");
      break;
    case aiShadingMode_Flat:
      dst->properties["ShadingModel"] = std::string("Flat");
      break;
    case aiShadingMode_Gouraud:
      dst->properties["ShadingModel"] = std::string("Gouraud");
      break;
    case aiShadingMode_Phong:
      dst->properties["ShadingModel"] = std::string("Phong");
      break;
    default:
      LOG(ERROR) << "Unknown shading model: " << model;
      dst->properties["ShadingModel"] = std::string("Phong");
      break;
  }
}

void AssetImporter::ReadTexture(const aiMaterial* src, Material* dst,
                                aiTextureType src_type, unsigned int index,
                                MaterialTextureUsage usage) {
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
  auto texture_iter = dst->textures.find(name);
  if (texture_iter != dst->textures.end()) {
    // Adds usage to texture only if it doesn't already have this usage.
    auto& usages = texture_iter->second.usages;
    auto usage_iter = std::find(usages.begin(), usages.end(), usage);
    if (usage_iter == usages.end()) {
      usages.push_back(usage);
    }
  } else {
    TextureInfo info;
    info.usages = {usage};
    info.wrap_s = ConvertTextureWrapMode(src_modes[0]);
    info.wrap_t = ConvertTextureWrapMode(src_modes[1]);

    // Embedded textures from Assimp are named "*#" where # is a decimal number
    // corresponding to a texture index.
    if (path.length > 1 && path.data[0] == '*') {
      char* path_end = path.data + path.length;
      uint32_t embedded_index = std::strtoul(path.data + 1, &path_end, 10);
      const aiTexture* texture = GetScene()->mTextures[embedded_index];

      // If mHeight is 0, it implies that the embedded texture is compressed and
      // the total size is stored in mWidth. Otherwise, the embedded data is
      // RGBA data.
      uint32_t embedded_texture_byte_count =
          texture->mHeight ? texture->mHeight * texture->mWidth * 4
                           : texture->mWidth;
      auto embedded_texture_bytes =
          reinterpret_cast<const uint8_t*>(texture->pcData);

      info.data = std::make_shared<ByteArray>(
          embedded_texture_bytes,
          embedded_texture_bytes + embedded_texture_byte_count);
    }

    dst->textures.emplace(std::move(name), std::move(info));
  }
}

void AssetImporter::ReadMaterial(const aiMaterial* src, Material* dst) {
  auto has_gltf_specular_glossiness = false;
  src->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS, has_gltf_specular_glossiness);
  auto use_specular_glossiness_textures_if_present =
      model_->GetImportDef().use_specular_glossiness_textures_if_present;
  auto should_use_specular_glossiness =
      use_specular_glossiness_textures_if_present &&
      has_gltf_specular_glossiness;

  if (should_use_specular_glossiness) {
    dst->properties["UsesSpecularGlossiness"] = true;
  }

  ReadShadingModel(src, dst);
  ReadStringProperty(src, dst, AI_MATKEY_NAME, "Name");
  ReadStringProperty(src, dst, AI_MATKEY_GLTF_ALPHAMODE, 0, 0, "AlphaMode");
  if (dst->properties.find("AlphaMode") != dst->properties.end()) {
    dst->properties["IsOpaque"] = dst->properties["AlphaMode"].ValueOr(
                                      std::string()) == std::string("OPAQUE");
  }
  ReadFloatProperty(src, dst, AI_MATKEY_GLTF_ALPHACUTOFF, 0, 0, "AlphaCutoff");
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
    ReadFloatProperty(src, dst,
                      AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS_GLOSSINESS_FACTOR,
                      "Glossiness");
  } else {
    ReadFloatProperty(src, dst,
                      AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, 0, 0,
                      "Metallic");
    ReadFloatProperty(src, dst,
                      AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, 0,
                      0, "Roughness");
  }
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_DIFFUSE, "DiffuseColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_AMBIENT, "AmbientColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_SPECULAR, "SpecularColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_EMISSIVE, "EmissiveColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_REFLECTIVE, "ReflectiveColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_TRANSPARENT, "TransparentColor");
  ReadColorProperty(src, dst,
                    AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, 0, 0,
                    "BaseColor");

  // If configured to use specular-glossines, reads in diffuse type as diffuse
  // usage. Otherwise, reads in metallic-roughness textures. If both
  // specular-glossiness and metallic-roughness textures are available, the base
  // color texture should be used for the base color usage. Otherwise, the
  // diffuse type will have the base color usage in it.
  if (should_use_specular_glossiness) {
    ReadTexture(src, dst, aiTextureType_DIFFUSE, 0,
                MaterialTextureUsage_DiffuseColor);
  } else {
    if (has_gltf_specular_glossiness) {
      ReadTexture(src, dst,
                  AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE,
                  MaterialTextureUsage_BaseColor);
    } else {
      ReadTexture(src, dst, aiTextureType_DIFFUSE, 0,
                  MaterialTextureUsage_BaseColor);
    }
    ReadTexture(src, dst,
                AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
                MaterialTextureUsage_Roughness);
    ReadTexture(src, dst,
                AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
                MaterialTextureUsage_Metallic);
  }

  ReadTexture(src, dst, aiTextureType_SPECULAR, 0,
              MaterialTextureUsage_Specular);
  ReadTexture(src, dst, aiTextureType_LIGHTMAP, 0,
              MaterialTextureUsage_Occlusion);
  ReadTexture(src, dst, aiTextureType_NORMALS, 0, MaterialTextureUsage_Normal);
  ReadTexture(src, dst, aiTextureType_DISPLACEMENT, 0,
              MaterialTextureUsage_Bump);
  ReadTexture(src, dst, aiTextureType_SHININESS, 0,
              MaterialTextureUsage_Shininess);
  ReadTexture(src, dst, aiTextureType_AMBIENT, 0, MaterialTextureUsage_Ambient);
  ReadTexture(src, dst, aiTextureType_EMISSIVE, 0,
              MaterialTextureUsage_Emissive);
  ReadTexture(src, dst, aiTextureType_HEIGHT, 0, MaterialTextureUsage_Height);
  ReadTexture(src, dst, aiTextureType_OPACITY, 0, MaterialTextureUsage_Opacity);
  ReadTexture(src, dst, aiTextureType_REFLECTION, 0,
              MaterialTextureUsage_Reflection);
  dst->name = dst->properties["Name"].ValueOr(std::string());
}

Model AssetImporter::Import(const ModelPipelineImportDefT& import_def) {
  Model model(import_def);

  AssimpBaseImporter::Options opts;
  opts.recenter = import_def.recenter;
  opts.axis_system = import_def.axis_system;
  opts.scale_multiplier = import_def.scale;
  opts.smoothing_angle = import_def.smoothing_angle;
  opts.max_bone_weights = import_def.max_bone_weights;
  opts.flip_texture_coordinates = import_def.flip_texture_coordinates;
  opts.flatten_hierarchy_and_transform_vertices_to_root_space =
      import_def.flatten_hierarchy_and_transform_vertices_to_root_space;
  opts.report_errors_to_stdout = import_def.report_errors_to_stdout;

  const bool success = LoadScene(import_def.file, opts);
  if (success) {
    model_ = &model;
    std::unordered_map<const aiMaterial*, size_t> material_map;

    ForEachMaterial([&](const aiMaterial* material) {
      material_map[material] = materials_.size();

      materials_.emplace_back();
      ReadMaterial(material, &materials_.back());
    });

    ForEachBone([&](const aiNode* node, const aiNode* parent,
                    const aiMatrix4x4& transform) {
      const std::string name = node->mName.C_Str();
      auto iter = hierarchy_.find(parent);
      const int parent_index = iter == hierarchy_.end() ? -1 : iter->second;

      const Bone bone(name, parent_index, Convert(transform).Inverse());
      const int index = model_->AppendBone(bone);
      hierarchy_[node] = index;
    });

    ForEachMesh([=](const aiMesh* mesh, const aiNode* node,
                    const aiMaterial* material) {
      auto iter = material_map.find(material);
      if (iter == material_map.end()) {
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

    model_->ComputeOrientationsFromTangentSpaces(
        import_def.ensure_vertex_orientation_w_not_zero);

    ForEachOpenedFile(
        [=](const std::string& file) { model_->AddImportedPath(file); });

    model_ = nullptr;
  }
  return model;
}

Model ImportAsset(const ModelPipelineImportDefT& import_def) {
  AssetImporter importer;
  return importer.Import(import_def);
}

}  // namespace tool
}  // namespace lull
