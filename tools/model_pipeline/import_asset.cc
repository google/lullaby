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

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "tools/model_pipeline/model.h"

namespace lull {
namespace tool {

mathfu::vec3 Convert(const aiVector3D& vec) { return {vec.x, vec.y, vec.z}; }

mathfu::vec4 Convert(const aiColor4D& color) {
  return {color.r, color.g, color.b, color.a};
}

mathfu::mat4 Convert(const aiMatrix4x4& mat) {
  return {
      mat.a1, mat.b1, mat.c1, mat.d1, mat.a2, mat.b2, mat.c2, mat.d2,
      mat.a3, mat.b3, mat.c3, mat.d3, mat.a4, mat.b4, mat.c4, mat.d4,
  };
}

mathfu::vec2 ConvertUv(const aiVector3D& vec) { return {vec.x, vec.y}; }

mathfu::vec4 ConvertTangent(const aiVector3D& normal, const aiVector3D& tangent,
                            const aiVector3D& bitangent) {
  const mathfu::vec3 n(normal.x, normal.y, normal.z);
  const mathfu::vec3 t(tangent.x, tangent.y, tangent.z);
  const mathfu::vec3 b(bitangent.x, bitangent.y, bitangent.z);

  const mathfu::vec3 n2 = mathfu::vec3::CrossProduct(t, b).Normalized();
  const bool sign = std::signbit(mathfu::vec3::DotProduct(n2, n));
  return {tangent.x, tangent.y, tangent.z, sign ? -1.f : 1.f};
}

mathfu::vec4 CalculateOrientation(const mathfu::vec3& normal,
                                  const mathfu::vec4& tangent) {
  const mathfu::vec3 n = normal.Normalized();
  const mathfu::vec3 t = tangent.xyz().Normalized();
  const mathfu::vec3 b = mathfu::vec3::CrossProduct(n, t).Normalized();
  const mathfu::mat3 m(t.x, t.y, t.z, b.x, b.y, b.z, n.x, n.y, n.z);
  mathfu::quat q = mathfu::quat::FromMatrix(m).Normalized();
  // Align the sign bit of the orientation scalar to our handedness.
  if (signbit(tangent.w) != signbit(q.scalar())) {
    q = mathfu::quat(-q.scalar(), -q.vector());
  }
  return mathfu::vec4(q.vector(), q.scalar());
}

TextureWrap Convert(const aiTextureMapMode& mode) {
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

class AssetImporter {
 public:
  AssetImporter() {}
  Model Import(const ModelPipelineImportDefT& import_def);

 private:
  void ReadMaterial(const aiMaterial* src, Material* dst);
  void ReadShadingModel(const aiMaterial* src, Material* dst);
  void ReadTexture(const aiMaterial* src, Material* dst, aiTextureType src_type,
                   MaterialTextureUsage usage);
  void ReadStringProperty(const aiMaterial* src, Material* dst,
                          const char* src_name, int a1, int a2,
                          const char* dst_name);
  void ReadFloatProperty(const aiMaterial* src, Material* dst,
                         const char* src_name, int a1, int a2,
                         const char* dst_name);
  void ReadColorProperty(const aiMaterial* src, Material* dst,
                         const char* src_name, int a1, int a2,
                         const char* dst_name);

  void ReadMesh(const aiNode* node, const aiMesh* src);
  void AddVertex(const aiNode* node, const aiMesh* src, int index);
  std::vector<Vertex::Influence> GatherInfluences(const aiMesh* src, int index);

  void AddNodeToHierarchy(const aiNode* node);
  void PopulateHierarchyRecursive(const aiNode* node);
  void ReadSkeletonRecursive(const aiNode* node, int parent_bone_index,
                             const aiMatrix4x4& base_transform);
  void ReadMeshRecursive(const aiNode* node);

  Model* model_ = nullptr;
  const aiScene* scene_ = nullptr;
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
  Vertex vertex;
  if (src->HasPositions()) {
    model_->EnableAttribute(Vertex::kAttribBit_Position);
    vertex.position = Convert(src->mVertices[index]);
  }
  if (src->HasNormals()) {
    model_->EnableAttribute(Vertex::kAttribBit_Normal);
    vertex.normal = Convert(src->mNormals[index]);
  }
  if (src->HasTangentsAndBitangents()) {
    model_->EnableAttribute(Vertex::kAttribBit_Tangent);
    vertex.tangent = ConvertTangent(src->mNormals[index], src->mTangents[index],
                                    src->mBitangents[index]);
    model_->EnableAttribute(Vertex::kAttribBit_Orientation);
    vertex.orientation = CalculateOrientation(vertex.normal, vertex.tangent);
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
                                   aiTextureType src_type,
                                   MaterialTextureUsage usage) {
  auto texture_count = src->GetTextureCount(src_type);
  if (texture_count == 0) {
    return;
  }
  if (texture_count > 1) {
    LOG(ERROR) << "Multiple (" << texture_count
               << ") textures for usage: " << usage;
  }

  aiString path;
  aiTextureMapMode src_modes[] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
  const auto res = src->GetTexture(src_type, 0, &path, nullptr, nullptr,
                                   nullptr, nullptr, src_modes);
  if (res != AI_SUCCESS) {
    LOG(ERROR) << "Unable to get texture information.";
    return;
  }

  TextureInfo info;
  info.usage = usage;
  info.wrap_s = Convert(src_modes[0]);
  info.wrap_t = Convert(src_modes[1]);

  const std::string name = path.C_Str();
  dst->textures[name] = info;
}

void AssetImporter::ReadStringProperty(const aiMaterial* src, Material* dst,
                                          const char* src_name, int a1, int a2,
                                          const char* dst_name) {
  aiString value;
  if (src->Get(src_name, a1, a2, value) == AI_SUCCESS) {
    dst->properties[dst_name] = std::string(value.C_Str());
  }
}

void AssetImporter::ReadFloatProperty(const aiMaterial* src, Material* dst,
                                         const char* src_name, int a1, int a2,
                                         const char* dst_name) {
  float value;
  if (src->Get(src_name, a1, a2, value) == AI_SUCCESS) {
    dst->properties[dst_name] = value;
  }
}

void AssetImporter::ReadColorProperty(const aiMaterial* src, Material* dst,
                                         const char* src_name, int a1, int a2,
                                         const char* dst_name) {
  aiColor3D value;
  if (src->Get(src_name, a1, a2, value) == AI_SUCCESS) {
    dst->properties[dst_name] = mathfu::vec3(value.r, value.g, value.b);
  }
}

void AssetImporter::ReadMaterial(const aiMaterial* src, Material* dst) {
  ReadShadingModel(src, dst);
  ReadStringProperty(src, dst, AI_MATKEY_NAME, "Name");
  ReadFloatProperty(src, dst, AI_MATKEY_OPACITY, "Opacity");
  ReadFloatProperty(src, dst, AI_MATKEY_BUMPSCALING, "BumpScaling");
  ReadFloatProperty(src, dst, AI_MATKEY_REFLECTIVITY, "Reflectivity");
  ReadFloatProperty(src, dst, AI_MATKEY_SHININESS, "Shininess");
  ReadFloatProperty(src, dst, AI_MATKEY_SHININESS_STRENGTH,
                    "ShininessStrength");
  ReadFloatProperty(src, dst, AI_MATKEY_REFRACTI, "RefractiveIndex");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_DIFFUSE, "DiffuseColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_AMBIENT, "AmbientColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_SPECULAR, "SpecularColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_EMISSIVE, "EmissiveColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_REFLECTIVE, "ReflectiveColor");
  ReadColorProperty(src, dst, AI_MATKEY_COLOR_TRANSPARENT, "TransparentColor");
  ReadTexture(src, dst, aiTextureType_DIFFUSE, MaterialTextureUsage_Diffuse);
  ReadTexture(src, dst, aiTextureType_SPECULAR, MaterialTextureUsage_Specular);
  ReadTexture(src, dst, aiTextureType_LIGHTMAP, MaterialTextureUsage_Light);
  ReadTexture(src, dst, aiTextureType_NORMALS, MaterialTextureUsage_Normal);
  ReadTexture(src, dst, aiTextureType_DISPLACEMENT, MaterialTextureUsage_Bump);
  ReadTexture(src, dst, aiTextureType_SHININESS, MaterialTextureUsage_Gloss);
  ReadTexture(src, dst, aiTextureType_AMBIENT, MaterialTextureUsage_Ambient);
  ReadTexture(src, dst, aiTextureType_EMISSIVE, MaterialTextureUsage_Emissive);
  ReadTexture(src, dst, aiTextureType_HEIGHT, MaterialTextureUsage_Height);
  ReadTexture(src, dst, aiTextureType_OPACITY, MaterialTextureUsage_Opacity);
  ReadTexture(src, dst, aiTextureType_REFLECTION,
              MaterialTextureUsage_Reflection);
  dst->name = dst->properties["Name"].ValueOr(std::string());
}

void AssetImporter::AddNodeToHierarchy(const aiNode* node) {
  while (node != nullptr && node != scene_->mRootNode) {
    // Nodes with $ symbols seem to be generated as part of the assimp importer
    // itself and are not part of the original asset.
    if (!strstr(node->mName.C_Str(), "$")) {
      const int index = Bone::kInvalidBoneIndex;
      hierarchy_.emplace(node, index);
    }
    node = node->mParent;
  }
}

void AssetImporter::PopulateHierarchyRecursive(const aiNode* node) {
  for (int i = 0; i < node->mNumMeshes; ++i) {
    AddNodeToHierarchy(node);

    const int mesh_index = node->mMeshes[i];
    const aiMesh* mesh = scene_->mMeshes[mesh_index];
    for (int j = 0; j < mesh->mNumBones; ++j) {
      const aiBone* bone = mesh->mBones[j];
      const aiNode* bone_node = scene_->mRootNode->FindNode(bone->mName);
      AddNodeToHierarchy(bone_node);
    }
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    PopulateHierarchyRecursive(node->mChildren[i]);
  }
}

void AssetImporter::ReadSkeletonRecursive(
    const aiNode* node, int parent_bone_index,
    const aiMatrix4x4& base_transform) {
  const aiMatrix4x4 transform = base_transform * node->mTransformation;
  auto iter = hierarchy_.find(node);
  if (iter != hierarchy_.end()) {
    const std::string name = node->mName.C_Str();
    const mathfu::mat4 binding_transform = Convert(transform).Inverse();
    const Bone bone(name, parent_bone_index, binding_transform);
    parent_bone_index = model_->AppendBone(bone);
    iter->second = parent_bone_index;
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    ReadSkeletonRecursive(node->mChildren[i], parent_bone_index, transform);
  }
}

void AssetImporter::ReadMeshRecursive(const aiNode* node) {
  for (int i = 0; i < node->mNumMeshes; ++i) {
    const int mesh_index = node->mMeshes[i];
    const aiMesh* mesh = scene_->mMeshes[mesh_index];

    if (mesh->mNumAnimMeshes != 0 || mesh->mAnimMeshes != nullptr) {
      LOG(ERROR) << "Animated meshes are unsupported.";
      continue;
    } else if (!mesh->HasPositions()) {
      LOG(ERROR) << "Mesh does not have positions.";
      continue;
    } else if (mesh->mMaterialIndex >= materials_.size()) {
      LOG(ERROR) << "Invalid material index. Cannot bind material.";
      continue;
    }

    model_->BindDrawable(materials_[mesh->mMaterialIndex]);
    for (int face_index = 0; face_index < mesh->mNumFaces; ++face_index) {
      const aiFace& face = mesh->mFaces[face_index];
      CHECK_EQ(face.mNumIndices, 3);
      for (int j = 0; j < face.mNumIndices; ++j) {
        const int vertex_index = face.mIndices[j];
        AddVertex(node, mesh, vertex_index);
      }
    }
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    ReadMeshRecursive(node->mChildren[i]);
  }
}

Model AssetImporter::Import(const ModelPipelineImportDefT& import_def) {
  Model model(import_def);
  model_ = &model;

  int flags = 0;
  flags |= aiProcess_CalcTangentSpace;
  flags |= aiProcess_JoinIdenticalVertices;
  flags |= aiProcess_Triangulate;
  flags |= aiProcess_GenSmoothNormals;
  flags |= aiProcess_FixInfacingNormals;
  flags |= aiProcess_GenUVCoords;
  flags |= aiProcess_FlipUVs;
  flags |= aiProcess_ImproveCacheLocality;
  flags |= aiProcess_RemoveRedundantMaterials;
  // TODO: Allow these flags to be enabled via the command line. They are
  // currently incompatible with anim_pipeline.
  // flags |= aiProcess_OptimizeMeshes;
  // flags |= aiProcess_OptimizeGraph;

  Assimp::Importer importer;
  const std::string& filename = model_->GetImportDef().file;
  scene_ = importer.ReadFile(filename.c_str(), flags);

  materials_.resize(scene_->mNumMaterials);
  for (int i = 0; i < scene_->mNumMaterials; ++i) {
    ReadMaterial(scene_->mMaterials[i], &materials_[i]);
  }
  // TODO: Handle embedded textures.
  PopulateHierarchyRecursive(scene_->mRootNode);
  ReadSkeletonRecursive(scene_->mRootNode, Bone::kInvalidBoneIndex,
                        aiMatrix4x4());
  ReadMeshRecursive(scene_->mRootNode);
  model_ = nullptr;
  return model;
}

Model ImportAsset(const ModelPipelineImportDefT& import_def) {
  AssetImporter importer;
  return importer.Import(import_def);
}

}  // namespace tool
}  // namespace lull
