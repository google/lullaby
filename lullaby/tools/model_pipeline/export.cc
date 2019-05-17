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

#include "lullaby/tools/model_pipeline/export.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <unordered_set>
#include "lullaby/util/filename.h"
#include "lullaby/util/flatbuffer_writer.h"
#include "lullaby/util/inward_buffer.h"
#include "lullaby/generated/model_def_generated.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "lullaby/tools/common/log.h"
#include "lullaby/tools/model_pipeline/export_options.h"
#include "lullaby/tools/model_pipeline/util.h"
#include "mathfu/io.h"
#include "openssl/sha.h"

namespace lull {
namespace tool {

static std::string DataToSha1HexString(const std::vector<uint8_t>& source) {
  SHA_CTX hasher;
  SHA1_Init(&hasher);

  if (!source.empty()) {
    SHA1_Update(&hasher, source.data(), source.size());
  }

  uint8_t digest[SHA_DIGEST_LENGTH];
  SHA1_Final(digest, &hasher);

  std::ostringstream result;
  result << std::hex << std::setfill('0') << std::nouppercase;
  for (auto& digit : digest) {
    result << std::setw(2) << static_cast<uint32_t>(digit);
  }

  return result.str();
}

static std::string GetTextureName(const TextureInfo& info,
                                  const ExportOptions options) {
  if (options.embed_textures || options.unique_texture_names) {
    if (info.data) {
      return DataToSha1HexString(*info.data.get());
    } else {
      return GenerateUniqueName(info.basename);
    }
  } else {
    return GetBasenameFromFilename(info.basename);
  }
}

template <typename T = Vertex>
struct VertexBuilder {
  template <typename Fn>
  void AddOp(Fn fn) {
    ops.emplace_back([fn](const T& vertex, std::vector<uint8_t>* vec) {
      auto value = fn(vertex);
      const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
      vec->insert(vec->end(), ptr, ptr + sizeof(value));
    });
  }

  void Build(const std::vector<T>& src, std::vector<uint8_t>* dst) {
    for (const T& vertex : src) {
      for (Op& op : ops) {
        op(vertex, dst);
      }
    }
  }

  using Op = std::function<void(const T&, std::vector<uint8_t>*)>;
  std::vector<Op> ops;
};

void ExportCollidable(const Model& model, ModelPipelineCollidableDefT* config) {
  config->source = model.GetImportDef().name;
}

void ExportSkeleton(const Model& model, SkeletonDefT* out,
                    ModelPipelineSkeletonDefT* config) {
  const auto& bones = model.GetBones();
  out->bone_names = GatherBoneNames(bones);
  out->bone_transforms = GatherBoneTransforms(bones);
  out->bone_parents = GatherParentBoneIndices(bones);

  config->source = model.GetImportDef().name;
}

void ExportModelInstance(const Model& model, ModelInstanceDefT* out,
                         ModelPipelineRenderableDefT* config,
                         const ExportOptions options) {
  out->interleaved = true;
  out->num_vertices = static_cast<uint32_t>(model.GetVertices().size());

  std::vector<uint8_t> mesh_to_shader_bones;
  GatherBoneIndexMaps(model.GetBones(), model.GetVertices(),
                      &mesh_to_shader_bones, &out->shader_to_mesh_bones);

  VertexBuilder<Vertex> builder;
  VertexBuilder<Vertex::Blend> blend_builder;

  if (model.CheckAttrib(Vertex::kAttribBit_Position)) {
    builder.AddOp([](const Vertex& vertex) {
      return mathfu::vec3_packed(vertex.position);
    });

    blend_builder.AddOp([](const Vertex::Blend& blend) {
      return mathfu::vec3_packed(blend.position);
    });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Position;
    attr.type = VertexAttributeType_Vec3f;
    out->vertex_attributes.emplace_back(attr);
    out->blend_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Color0)) {
    builder.AddOp(
        [](const Vertex& vertex) { return Color4ub::FromVec4(vertex.color0); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Color;
    attr.type = VertexAttributeType_Vec4ub;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Color1)) {
    builder.AddOp(
        [](const Vertex& vertex) { return Color4ub::FromVec4(vertex.color1); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Color;
    attr.type = VertexAttributeType_Vec4ub;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Color2)) {
    builder.AddOp(
        [](const Vertex& vertex) { return Color4ub::FromVec4(vertex.color2); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Color;
    attr.type = VertexAttributeType_Vec4ub;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Color3)) {
    builder.AddOp(
        [](const Vertex& vertex) { return Color4ub::FromVec4(vertex.color3); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Color;
    attr.type = VertexAttributeType_Vec4ub;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv0)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv0); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv1)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv1); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv2)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv2); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv3)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv3); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv4)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv4); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv5)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv5); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv6)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv6); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Uv7)) {
    builder.AddOp(
        [](const Vertex& vertex) { return mathfu::vec2_packed(vertex.uv7); });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_TexCoord;
    attr.type = VertexAttributeType_Vec2f;
    out->vertex_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Normal)) {
    builder.AddOp([](const Vertex& vertex) {
      return mathfu::vec3_packed(vertex.normal);
    });
    blend_builder.AddOp([](const Vertex::Blend& blend) {
      return mathfu::vec3_packed(blend.normal);
    });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Normal;
    attr.type = VertexAttributeType_Vec3f;
    out->vertex_attributes.emplace_back(attr);
    out->blend_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Tangent)) {
    builder.AddOp([](const Vertex& vertex) {
      return mathfu::vec4_packed(vertex.tangent);
    });
    blend_builder.AddOp([](const Vertex::Blend& blend) {
      return mathfu::vec4_packed(blend.tangent);
    });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Tangent;
    attr.type = VertexAttributeType_Vec4f;
    out->vertex_attributes.emplace_back(attr);
    out->blend_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Orientation)) {
    builder.AddOp([](const Vertex& vertex) {
      return mathfu::vec4_packed(vertex.orientation);
    });
    blend_builder.AddOp([](const Vertex::Blend& blend) {
      return mathfu::vec4_packed(blend.orientation);
    });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_Orientation;
    attr.type = VertexAttributeType_Vec4f;
    out->vertex_attributes.emplace_back(attr);
    out->blend_attributes.emplace_back(attr);
  }
  if (model.CheckAttrib(Vertex::kAttribBit_Influences)) {
    builder.AddOp([&mesh_to_shader_bones](const Vertex& vertex) {
      // Helper struct to allow us to bundle both the bone weights and indices
      // into a single object that can be written into the vertex buffer.
      static const int kMaxInfluences = 4;
      struct Influences8ub {
        uint8_t indices[kMaxInfluences] = {0xff, 0xff, 0xff, 0xff};
        uint8_t weights[kMaxInfluences] = {0, 0, 0, 0};
      };
      Influences8ub influences;
      CompactInfluences(vertex.influences, mesh_to_shader_bones,
                        influences.indices, influences.weights, kMaxInfluences);
      return influences;
    });

    VertexAttributeT attr;
    attr.usage = VertexAttributeUsage_BoneIndices;
    attr.type = VertexAttributeType_Vec4ub;
    out->vertex_attributes.emplace_back(attr);

    attr.usage = VertexAttributeUsage_BoneWeights;
    attr.type = VertexAttributeType_Vec4ub;
    out->vertex_attributes.emplace_back(attr);
  }

  // Reflect the generated attributes back to the config.
  LogWrite("    Vertex format:\n");
  for (auto& vertex_attribute : out->vertex_attributes) {
    LogWrite("      %s %s\n",
             EnumNameVertexAttributeType(vertex_attribute.type),
             EnumNameVertexAttributeUsage(vertex_attribute.usage));
    config->attributes.emplace_back(vertex_attribute.usage);
  }

  const std::vector<Vertex>& vertices = model.GetVertices();
  builder.Build(vertices, &out->vertex_data);

  LogWrite("    Vertex count: %d [%d bytes]\n", out->num_vertices,
           out->vertex_data.size());

  const size_t num_blends = vertices[0].blends.size();
  if (num_blends > 0) {
    out->blend_shapes.reserve(num_blends + 0);

    for (size_t i = 0; i < num_blends; ++i) {
      std::vector<Vertex::Blend> blend_vertices;
      blend_vertices.reserve(vertices.size());
      for (const Vertex& v : vertices) {
        blend_vertices.push_back(v.blends[i]);
      }

      BlendShapeT blend_shape;
      blend_shape.name = Hash(vertices[0].blends[i].name);
      blend_builder.Build(blend_vertices, &blend_shape.vertex_data);
      out->blend_shapes.emplace_back(blend_shape);
    }
  } else {
    out->blend_attributes.clear();
  }

  for (const Drawable& drawable : model.GetDrawables()) {
    LogWrite("    Materials:\n");

    ModelIndexRangeT range;
    if (model.GetVertices().size() > std::numeric_limits<uint16_t>::max()) {
      range.start = static_cast<int>(out->indices32.size());
      for (size_t index : drawable.indices) {
        out->indices32.emplace_back(static_cast<uint32_t>(index));
      }
      range.end = static_cast<int>(out->indices32.size());
      LogWrite("      Index32 count: %d [%d bytes]\n",
               (range.end - range.start),
               out->indices32.size() * sizeof(uint32_t));
    } else {
      range.start = static_cast<int>(out->indices16.size());
      for (size_t index : drawable.indices) {
        out->indices16.emplace_back(static_cast<uint16_t>(index));
      }
      range.end = static_cast<int>(out->indices16.size());
      LogWrite("      Index16 count: %d [%d bytes]\n",
               (range.end - range.start),
               out->indices16.size() * sizeof(uint16_t));
    }
    out->ranges.emplace_back(range);

    out->aabbs.emplace_back();
    auto& aabb = out->aabbs.back();
    aabb.min_position = drawable.min_position_;
    aabb.max_position = drawable.max_position_;

    out->materials.emplace_back();
    MaterialDefT& material_def = out->materials.back();
    material_def.name = drawable.material.name;
    LogWrite("      Name: %s\n", material_def.name.c_str());

    LogWrite("      Properties: \n");
    for (auto& prop : drawable.material.properties) {
      KeyVariantPairDefT pair;
      pair.key = prop.first;
      pair.hash_key = Hash(prop.first);
      LogWrite("        %s, 0x%08x, ", pair.key.c_str(), pair.hash_key);

      const TypeId property_type = prop.second.GetTypeId();
      if (property_type == GetTypeId<bool>()) {
        const auto value = prop.second.ValueOr<bool>(false);
        pair.value.set<DataBoolT>()->value = value;
        LogWrite("%s", (value) ? "true" : "false");
      } else if (property_type == GetTypeId<int>()) {
        const auto value = prop.second.ValueOr<int>(0);
        pair.value.set<DataIntT>()->value = value;
        LogWrite("%d", value);
      } else if (property_type == GetTypeId<float>()) {
        const auto value = prop.second.ValueOr<float>(0.f);
        pair.value.set<DataFloatT>()->value = value;
        LogWrite("%f", value);
      } else if (property_type == GetTypeId<double>()) {
        const auto value = static_cast<float>(prop.second.ValueOr<double>(0.0));
        pair.value.set<DataFloatT>()->value = value;
        LogWrite("%f", value);
      } else if (property_type == GetTypeId<std::string>()) {
        const auto value = prop.second.ValueOr(std::string());
        pair.value.set<DataStringT>()->value = value;
        LogWrite("%s", value.c_str());
      } else if (property_type == GetTypeId<mathfu::vec2>()) {
        const auto value = prop.second.ValueOr(mathfu::vec2(0.f, 0.f));
        pair.value.set<DataVec2T>()->value = value;
        LogWrite("%f, %f", value.x, value.y);
      } else if (property_type == GetTypeId<mathfu::vec3>()) {
        const auto value = prop.second.ValueOr(mathfu::vec3(0.f, 0.f, 0.f));
        pair.value.set<DataVec3T>()->value = value;
        LogWrite("%f, %f, %f", value.x, value.y, value.z);
      } else if (property_type == GetTypeId<mathfu::vec4>()) {
        const auto value =
            prop.second.ValueOr(mathfu::vec4(0.f, 0.f, 0.f, 0.f));
        pair.value.set<DataVec4T>()->value = value;
        LogWrite("%f, %f, %f, %f", value.x, value.y, value.z, value.w);
      } else if (property_type == GetTypeId<mathfu::mat4>()) {
        LOG(ERROR) << "Matrix properties are currently unsupported.";
      } else {
        LOG(ERROR) << "Unknown property type: " << property_type;
      }
      LogWrite("\n");
      material_def.properties.values.emplace_back(pair);
    }

    LogWrite("      Textures:\n");
    for (auto& texture : drawable.material.textures) {
      if (!texture.second.basename.empty()) {
        auto name = GetTextureName(texture.second, options);
        LogWrite("        name: %s\n", name.c_str());

        // If there is only one usage for a texture, then we set it in the usage
        // MaterialTextureDefT field, otherwise we use the usage_per_channel
        // field.
        MaterialTextureDefT material_texture;
        material_texture.name = name;
        DCHECK_GT(texture.second.usages.size(), 0);
        if (texture.second.usages.size() == 1) {
          material_texture.usage = texture.second.usages[0];
        } else if (texture.second.usages.size() > 1) {
          material_texture.usage = MaterialTextureUsage_Unused;
          material_texture.usage_per_channel = texture.second.usages;
        }
        material_def.textures.push_back(std::move(material_texture));
      }
    }
    LogWrite("\n");

    ModelPipelineMaterialDefT pipeline_material_def;
    pipeline_material_def.material = material_def;
    config->materials.emplace_back(pipeline_material_def);
  }

  for (auto& config_material : config->materials) {
    for (auto& prop : config_material.material.properties.values) {
      prop.hash_key = 0;
    }
  }

  config->source = model.GetImportDef().name;
}

void ExportTexture(const TextureInfo& info, TextureDefT* out,
                   TextureDefT* config, const ExportOptions options) {
  out->file = info.basename;
  out->name = GetTextureName(info, options);
  out->wrap_s = info.wrap_s;
  out->wrap_t = info.wrap_t;
  out->premultiply_alpha = info.premultiply_alpha;
  out->generate_mipmaps = info.generate_mipmaps;

  *config = *out;
  config->file = info.abs_path;

  if (options.embed_textures) {
    if (info.data) {
      out->data = *info.data;
    } else {
      const std::string& texture_path =
          (options.relative_path) ? info.basename : info.abs_path;
      std::ifstream file(texture_path, std::ios::binary);
      CHECK(file) << "Unable to open texture file: " << texture_path;
      file.seekg(0, std::ios::end);
      out->data.resize(file.tellg());
      file.seekg(0, std::ios::beg);
      file.read(reinterpret_cast<char*>(out->data.data()), out->data.size());
      out->file = GetBasenameFromFilename(out->file);
    }
  }

  LogWrite("  %s: \n", out->name.c_str());
  if (options.embed_textures) {
    LogWrite("    size: %d bytes\n", out->data.size());
  } else {
    LogWrite("    file: %s\n", out->file.c_str());
  }
  LogWrite("    mipmaps: %s\n", (out->generate_mipmaps) ? "yes" : "no");
  LogWrite("    premul. alpha: %s\n", (out->premultiply_alpha) ? "yes" : "no");
}

ByteArray ExportModel(
    const std::unordered_map<std::string, Model>& models,
    const std::unordered_map<std::string, TextureInfo>& textures,
    const ExportOptions options, ModelPipelineDefT* out_config) {
  ModelDefT model_def;
  ModelPipelineDefT pipeline_def;

  // Export model bounding box.
  mathfu::vec3 min_position(std::numeric_limits<float>::max());
  mathfu::vec3 max_position(std::numeric_limits<float>::lowest());
  for (const auto& iter : models) {
    const Model& model = iter.second;
    min_position = mathfu::vec3::Min(min_position, model.GetMinPosition());
    max_position = mathfu::vec3::Max(max_position, model.GetMaxPosition());
  }
  model_def.bounding_box.min = min_position;
  model_def.bounding_box.max = max_position;

  for (const auto& iter : models) {
    const Model& model = iter.second;
    pipeline_def.sources.emplace_back(model.GetImportDef());
  }

  // Export model lods.
  LogWrite("Render Models:\n");
  for (const auto& iter : models) {
    const Model& model = iter.second;
    if (model.CheckUsage(Model::kForRendering)) {
      const int level = model.GetLodLevel();
      model_def.lods.resize(level + 1);
      LogWrite("  Model %s:\n", pipeline_def.renderables.size());
      pipeline_def.renderables.emplace_back();
      ExportModelInstance(model, &model_def.lods[level],
                          &pipeline_def.renderables.back(), options);
    }
  }

  // Export collidables.
  for (const auto& iter : models) {
    const Model& model = iter.second;
    if (model.CheckUsage(Model::kForCollision)) {
      ExportCollidable(model, &pipeline_def.collidable);
      break;
    }
  }

  // Export model skeleton.
  for (const auto& iter : models) {
    const Model& model = iter.second;
    if (model.CheckUsage(Model::kForSkeleton)) {
      ExportSkeleton(model, &model_def.skeleton, &pipeline_def.skeleton);
      break;
    }
  }

  LogWrite("Exported Textures: %s\n",
           (options.embed_textures) ? "Embeded" : "Loose");
  // Export textures.
  for (const auto& iter : textures) {
    if (!iter.second.basename.empty()) {
      model_def.textures.emplace_back();
      pipeline_def.textures.emplace_back();
      ExportTexture(iter.second, &model_def.textures.back(),
                    &pipeline_def.textures.back(), options);
    }
  }
  LogWrite("\n");

  // Export the pipeline config.
  if (out_config) {
    *out_config = pipeline_def;
  }

  // Convert exported data into a flatbuffer.
  InwardBuffer buffer(4096);
  WriteFlatbuffer(&model_def, &buffer);
  const size_t length = buffer.BackSize();
  const uint8_t* data = static_cast<const uint8_t*>(buffer.BackAt(length));
  return ByteArray(data, data + length);
}

}  // namespace tool
}  // namespace lull
