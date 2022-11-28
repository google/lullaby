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

#include "redux/tools/model_pipeline/export.h"

#include "redux/data/asset_defs/model_asset_def_generated.h"
#include "redux/data/asset_defs/texture_asset_def_generated.h"
#include "redux/modules/flatbuffers/common.h"
#include "redux/modules/graphics/color.h"
#include "redux/tools/common/flatbuffer_utils.h"
#include "redux/tools/common/log_utils.h"
#include "redux/tools/model_pipeline/util.h"

namespace redux::tool {

static std::unique_ptr<fbs::HashStringT> MakeName(std::string_view name) {
  return std::make_unique<fbs::HashStringT>(CreateHashStringT(name));
}

static bool Requires32BitIndices(size_t num_vertices) {
  return num_vertices > std::numeric_limits<uint16_t>::max();
}

static fbs::Vec3f Vec3ToFbsVec3f(const vec3& in) {
  return fbs::Vec3f(in.x, in.y, in.z);
}

static fbs::Mat3x4f MatrixToFbsMat4x3f(const mat4& in) {
  const vec3 c0 = in.Column(0).xyz();
  const vec3 c1 = in.Column(1).xyz();
  const vec3 c2 = in.Column(2).xyz();
  const vec3 c3 = in.Column(3).xyz();
  return fbs::Mat3x4f(
      fbs::Vec3f(c0.x, c0.y, c0.z), fbs::Vec3f(c1.x, c1.y, c1.z),
      fbs::Vec3f(c2.x, c2.y, c2.z), fbs::Vec3f(c3.x, c3.y, c3.z));
}

template <typename T = Vertex>
class VertexBufferBuilder {
 public:
  void ClearBuffer() { buffer_.clear(); }

  template <typename Fn>
  void AddOp(const Fn& fn) {
    ops_.emplace_back([this, fn](const T& v) {
      const auto value = fn(v);
      const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
      buffer_.insert(buffer_.end(), ptr, ptr + sizeof(value));
    });
  }

  template <typename Fn>
  void AddVec2Op(const Fn& fn) {
    AddOp([&](const T& v) { return Vector<float, 2, false>(fn(v)); });
  }

  template <typename Fn>
  void AddVec3Op(const Fn& fn) {
    AddOp([&](const T& v) { return Vector<float, 3, false>(fn(v)); });
  }

  template <typename Fn>
  void AddVec4Op(const Fn& fn) {
    AddOp([&](const T& v) { return Vector<float, 4, false>(fn(v)); });
  }

  template <typename Fn>
  void AddColorOp(const Fn& fn) {
    AddOp([&](const T& v) { return Color4ub::FromVec4(fn(v)); });
  }

  void ApplyOps(const T& vertex) {
    for (Op& op : ops_) {
      op(vertex);
    }
  }

  std::vector<uint8_t> Release() {
    std::vector<uint8_t> out;
    buffer_.swap(out);
    return out;
  }

 private:
  using Op = std::function<void(const T&)>;
  std::vector<Op> ops_;
  std::vector<uint8_t> buffer_;
};

template <typename T>
static void AppendIndices(T* indices, const std::vector<size_t>& src) {
  indices->reserve(indices->size() + src.size());
  for (size_t index : src) {
    indices->emplace_back(index);
  }
}

static void ExportSkeleton(const Model& model, ModelSkeletonAssetDefT* out) {
  const auto& bones = model.GetBones();
  out->bone_names.reserve(bones.size());
  out->bone_parents.reserve(bones.size());
  out->bone_transforms.reserve(bones.size());

  for (const Bone& bone : bones) {
    const auto parent = CompactBoneIndex(bone.parent_bone_index);
    const auto transform = MatrixToFbsMat4x3f(bone.inverse_bind_transform);
    out->bone_names.push_back(bone.name);
    out->bone_parents.push_back(parent);
    out->bone_transforms.push_back(transform);
  }
}

void ExportMaterial(const Material& material, MaterialAssetDefT* out) {
  out->name = MakeName(material.name);

  out->properties = std::make_unique<fbs::VarTableDefT>();
  for (const auto& prop : material.properties) {
    auto pair = std::make_unique<fbs::KeyVarPairDefT>();
    pair->key =
        std::make_unique<fbs::HashStringT>(CreateHashStringT(prop.first));

    const TypeId property_type = prop.second.GetTypeId();
    if (property_type == GetTypeId<bool>()) {
      const auto value = prop.second.ValueOr<bool>(false);

      fbs::DataBoolT data;
      data.value = value;
      pair->value.Set(std::move(data));
    } else if (property_type == GetTypeId<int>()) {
      const auto value = prop.second.ValueOr<int>(0);

      fbs::DataIntT data;
      data.value = value;
      pair->value.Set(std::move(data));
    } else if (property_type == GetTypeId<float>()) {
      const auto value = prop.second.ValueOr<float>(0.0f);

      fbs::DataFloatT data;
      data.value = value;
      pair->value.Set(std::move(data));
    } else if (property_type == GetTypeId<std::string>()) {
      const auto value = prop.second.ValueOr(std::string());

      fbs::DataStringT data;
      data.value = std::move(value);
      pair->value.Set(std::move(data));
    } else if (property_type == GetTypeId<vec2>()) {
      auto value = prop.second.ValueOr(vec2(0.0f, 0.0f));

      fbs::DataVec2fT data;
      data.value = std::make_unique<fbs::Vec2f>(value.x, value.y);
      pair->value.Set(std::move(data));
    } else if (property_type == GetTypeId<vec3>()) {
      auto value = prop.second.ValueOr(vec3(0.0f, 0.0f, 0.0f));

      fbs::DataVec3fT data;
      data.value = std::make_unique<fbs::Vec3f>(value.x, value.y, value.z);
      pair->value.Set(std::move(data));
    } else if (property_type == GetTypeId<vec4>()) {
      auto value = prop.second.ValueOr(vec4(0.0f, 0.0f, 0.0f, 0.0f));

      fbs::DataVec4fT data;
      data.value =
          std::make_unique<fbs::Vec4f>(value.x, value.y, value.z, value.w);
      pair->value.Set(std::move(data));
    } else {
      LOG(FATAL) << "Unknown property type: " << property_type;
    }
    out->properties->values.push_back(std::move(pair));
  }

  for (const auto& texture : material.textures) {
    auto def = std::make_unique<MaterialTextureAssetDefT>();
    def->name = MakeName(texture.second.export_name);
    for (int i = 0; i < TextureUsage::kNumChannels; ++i) {
      def->usage.push_back(texture.second.usage.channel[i]);
    }
    out->textures.push_back(std::move(def));
  }
}

void ExportTexture(const TextureInfo& info, ModelTextureAssetDefT* out) {
  out->name = MakeName(info.export_name);

  out->texture = std::make_unique<TextureAssetDefT>();
  out->texture->wrap_s = info.wrap_s;
  out->texture->wrap_t = info.wrap_t;
  out->texture->premultiply_alpha = info.premultiply_alpha;
  out->texture->generate_mipmaps = info.generate_mipmaps;

  if (info.data) {
    const auto span = info.data->GetByteSpan();
    const char* data = reinterpret_cast<const char*>(span.data());
    out->texture->image = std::make_unique<ImageAssetDefT>();
    out->texture->image->format = info.format;
    out->texture->image->size =
        std::make_unique<fbs::Vec2i>(info.size.x, info.size.y);
    out->texture->image->data.assign(data, data + span.size());
  } else {
    // TODO: add support for exporting textures as separate files.
    LOG(FATAL) << "Unable to embed texture: " << info.export_name;
  }
}

void ExportModelInstance(const Model& model, ModelInstanceAssetDefT* out) {
  const size_t num_vertices = model.GetVertices().size();
  CHECK_GT(num_vertices, 0);

  VertexBufferBuilder<Vertex> builder;
  VertexBufferBuilder<Vertex::Blend> blender;
  std::vector<ModelVertexAttributeAssetDef> format;

  std::vector<uint16_t> mesh_to_shader_bones;
  std::vector<uint16_t> shader_to_mesh_bones;
  GatherBoneIndexMaps(model.GetBones(), model.GetVertices(),
                      &mesh_to_shader_bones, &shader_to_mesh_bones);

  const Vertex::Attrib attribs = model.GetAttribs();
  if (attribs.Any(Vertex::kAttribBit_Position)) {
    builder.AddVec3Op([](const Vertex& v) { return v.position; });
    blender.AddVec3Op([](const Vertex::Blend& v) { return v.position; });
    format.emplace_back(VertexUsage::Position, VertexType::Vec3f);
  }
  if (attribs.Any(Vertex::kAttribBit_Normal)) {
    builder.AddVec3Op([](const Vertex& v) { return v.normal; });
    blender.AddVec3Op([](const Vertex::Blend& v) { return v.normal; });
    format.emplace_back(VertexUsage::Normal, VertexType::Vec3f);
  }
  if (attribs.Any(Vertex::kAttribBit_Tangent)) {
    builder.AddVec4Op([](const Vertex& v) { return v.tangent; });
    blender.AddVec4Op([](const Vertex::Blend& v) { return v.tangent; });
    format.emplace_back(VertexUsage::Tangent, VertexType::Vec4f);
  }
  if (attribs.Any(Vertex::kAttribBit_Orientation)) {
    builder.AddVec4Op([](const Vertex& v) { return v.orientation; });
    blender.AddVec4Op([](const Vertex::Blend& v) { return v.orientation; });
    format.emplace_back(VertexUsage::Orientation, VertexType::Vec4f);
  }
  if (attribs.Any(Vertex::kAttribBit_Color0)) {
    builder.AddColorOp([](const Vertex& v) { return v.color0; });
    format.emplace_back(VertexUsage::Color0, VertexType::Vec4ub);
  }
  if (attribs.Any(Vertex::kAttribBit_Color1)) {
    builder.AddColorOp([](const Vertex& v) { return v.color1; });
    format.emplace_back(VertexUsage::Color1, VertexType::Vec4ub);
  }
  if (attribs.Any(Vertex::kAttribBit_Color2)) {
    builder.AddColorOp([](const Vertex& v) { return v.color2; });
    format.emplace_back(VertexUsage::Color2, VertexType::Vec4ub);
  }
  if (attribs.Any(Vertex::kAttribBit_Color3)) {
    builder.AddColorOp([](const Vertex& v) { return v.color3; });
    format.emplace_back(VertexUsage::Color3, VertexType::Vec4ub);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv0)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv0; });
    format.emplace_back(VertexUsage::TexCoord0, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv1)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv1; });
    format.emplace_back(VertexUsage::TexCoord1, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv2)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv2; });
    format.emplace_back(VertexUsage::TexCoord2, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv3)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv3; });
    format.emplace_back(VertexUsage::TexCoord3, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv4)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv4; });
    format.emplace_back(VertexUsage::TexCoord4, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv5)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv5; });
    format.emplace_back(VertexUsage::TexCoord5, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv6)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv6; });
    format.emplace_back(VertexUsage::TexCoord6, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Uv7)) {
    builder.AddVec2Op([](const Vertex& v) { return v.uv7; });
    format.emplace_back(VertexUsage::TexCoord7, VertexType::Vec2f);
  }
  if (attribs.Any(Vertex::kAttribBit_Influences)) {
    builder.AddOp([&mesh_to_shader_bones](const Vertex& v) {
      // Helper struct to allow us to bundle both the bone weights and indices
      // into a single object that can be written into the vertex buffer.
      static const int kMaxInfluences = 4;
      struct Influences {
        uint16_t indices[kMaxInfluences] = {0xffff, 0xffff, 0xffff, 0xffff};
        float weights[kMaxInfluences] = {0, 0, 0, 0};
      };
      Influences influences;
      CompactInfluences(v.influences, mesh_to_shader_bones, influences.indices,
                        influences.weights, kMaxInfluences);
      return influences;
    });

    format.emplace_back(VertexUsage::BoneIndices, VertexType::Vec4us);
    format.emplace_back(VertexUsage::BoneWeights, VertexType::Vec4f);
  }

  // Build the vertex buffer data.
  out->vertices = std::make_unique<ModelVertexBufferAssetDefT>();

  builder.ClearBuffer();
  for (const Vertex& v : model.GetVertices()) {
    builder.ApplyOps(v);
  }

  out->vertices->data = builder.Release();
  out->vertices->vertex_format = std::move(format);
  out->vertices->interleaved = true;
  out->vertices->num_vertices = static_cast<uint32_t>(num_vertices);

  const bool is_num_vertices_long = Requires32BitIndices(num_vertices);

  // The drawables are used for the index buffer, the index ranges, and the
  // materials.
  size_t range_index = 0;
  out->indices = std::make_unique<ModelIndexBufferAssetDefT>();
  for (const Drawable& drawable : model.GetDrawables()) {
    if (is_num_vertices_long) {
      AppendIndices(&out->indices->data32, drawable.indices);
    } else {
      AppendIndices(&out->indices->data16, drawable.indices);
    }

    auto part = std::make_unique<ModelInstancePartAssetDefT>();

    const uint32_t start = range_index;
    const uint32_t end = start + drawable.indices.size();
    range_index = end;
    part->range = std::make_unique<ModelIndexRangeAssetDef>(start, end);

    const fbs::Vec3f min = Vec3ToFbsVec3f(drawable.min_position);
    const fbs::Vec3f max = Vec3ToFbsVec3f(drawable.max_position);
    part->bounding_box = std::make_unique<fbs::Boxf>(min, max);

    part->material = std::make_unique<MaterialAssetDefT>();
    ExportMaterial(drawable.material, part->material.get());

    out->parts.push_back(std::move(part));
  }

  // Build the shader bone mapping for the base model.
  out->shader_to_mesh_bones = std::move(shader_to_mesh_bones);

  const Vertex& v = model.GetVertices()[0];
  for (size_t i = 0; i < v.blends.size(); ++i) {
    auto blend = std::make_unique<ModelBlendShapeAssetDefT>();
    blend->name = MakeName(v.blends[i].name);

    blender.ClearBuffer();
    for (const Vertex& v : model.GetVertices()) {
      blender.ApplyOps(v.blends[i]);
    }
    blend->vertices->data = blender.Release();
    blend->vertices->vertex_format = out->vertices->vertex_format;
    blend->vertices->interleaved = out->vertices->interleaved;
    blend->vertices->num_vertices = out->vertices->num_vertices;
    out->blend_shapes.push_back(std::move(blend));
  }
}

void LogResults(const ModelAssetDefT& out, Logger& log) {
  log("version: ", out.version);
  log("bounds:");
  log("  min: ", out.bounding_box->min());
  log("  max: ", out.bounding_box->max());

  const auto& names = out.skeleton->bone_names;
  const auto& parents = out.skeleton->bone_parents;
  CHECK(names.size() == parents.size());

  log("skeleton: ", (out.skeleton ? "" : "na/"));
  log("  bones: ", names.size());
  const uint16_t kInvalidBone = CompactBoneIndex(Bone::kInvalidBoneIndex);
  for (size_t i = 0; i < names.size(); ++i) {
    const uint16_t id = parents[i];
    const char* parent_name = id == kInvalidBone ? "n/a" : names[id].c_str();
    log("    ", names[i], " (", parent_name, ")");
    log("      ", out.skeleton->bone_transforms[i]);
  }

  log("textures: ", out.textures.size());
  for (const auto& texture : out.textures) {
    log("  ", *texture->name, ":");
    log("    uri: ", texture->uri);
    if (texture->texture) {
      if (texture->texture->image) {
        log("    data: ", texture->texture->image->data.size(), " bytes");
        log("    size: ", *texture->texture->image->size);
        log("    format: ", ToString(texture->texture->image->format));
      }
      log("    type: ", ToString(texture->texture->target_type));
      log("    min_filter: ", ToString(texture->texture->min_filter));
      log("    mag_filter: ", ToString(texture->texture->mag_filter));
      log("    wrap_r: ", ToString(texture->texture->wrap_r));
      log("    wrap_s: ", ToString(texture->texture->wrap_s));
      log("    wrap_t: ", ToString(texture->texture->wrap_t));
      log("    premul alpha: ", texture->texture->premultiply_alpha);
      log("    mipmaps: ", texture->texture->generate_mipmaps);
      log("    rgbm: ", texture->texture->is_rgbm);
    }
  }

  log("lods: ", out.lods.size());
  for (size_t i = 0; i < out.lods.size(); ++i) {
    const auto& lod = out.lods[i];
    log("  ", i);

    log("    vertex format:");
    for (const auto& attrib : lod->vertices->vertex_format) {
      log("      ", ToString(attrib.type()), " ", ToString(attrib.usage()));
    }
    log("    vertices: ", lod->vertices->num_vertices);
    log("      bytes: ", lod->vertices->data.size());

    if (!lod->indices->data16.empty()) {
      log("    index format: U16");
      log("    indices: ", lod->indices->data16.size());
      log("      bytes: ", lod->indices->data16.size() * sizeof(uint16_t));
    } else {
      log("    index format: U32");
      log("    indices: ", lod->indices->data32.size());
      log("      bytes: ", lod->indices->data32.size() * sizeof(uint32_t));
    }

    log("    shader_bones: ", lod->shader_to_mesh_bones.size());

    log("    drawables: ", lod->parts.size());
    for (const auto& part : lod->parts) {
      log("      ", part->name);
      // part->range
      // part->bounds
      if (part->material) {
        log("      name: ", part->material->name);
        log("      textures:");
        for (const auto& texture : part->material->textures) {
          log("        ", texture->name);
          for (const auto& usage : texture->usage) {
            log("          ", ToString(usage));
          }
        }
        log("      properties:");
        for (const auto& property : part->material->properties->values) {
          log("        ", property->key, ": ", property->value);
        }
      }
    }
  }
}

DataContainer ExportModel(absl::Span<const ModelPtr> lods, ModelPtr skeleton,
                          ModelPtr collidable, Logger& log) {
  ModelAssetDefT model_def;
  model_def.version = 1;

  // Export model bounding box.
  vec3 min_position(std::numeric_limits<float>::max());
  vec3 max_position(std::numeric_limits<float>::lowest());
  for (const ModelPtr& model : lods) {
    min_position = Min(min_position, model->GetMinPosition());
    max_position = Max(max_position, model->GetMaxPosition());
  }
  if (skeleton) {
    min_position = Min(min_position, skeleton->GetMinPosition());
    max_position = Max(max_position, skeleton->GetMaxPosition());
  }
  if (collidable) {
    min_position = Min(min_position, collidable->GetMinPosition());
    max_position = Max(max_position, collidable->GetMaxPosition());
  }
  model_def.bounding_box = std::make_unique<fbs::Boxf>(
      Vec3ToFbsVec3f(min_position), Vec3ToFbsVec3f(max_position));

  // Export skeleton.
  if (skeleton) {
    model_def.skeleton = std::make_unique<ModelSkeletonAssetDefT>();
    ExportSkeleton(*skeleton, model_def.skeleton.get());
  }

  // Export LODs.
  for (const ModelPtr& model : lods) {
    auto lod = std::make_unique<ModelInstanceAssetDefT>();
    ExportModelInstance(*model, lod.get());
    model_def.lods.emplace_back(std::move(lod));
  }

  // Gather texture for export.
  absl::flat_hash_map<std::string, const TextureInfo*> textures;
  for (const ModelPtr& model : lods) {
    for (const Drawable& drawable : model->GetDrawables()) {
      for (const auto& iter : drawable.material.textures) {
        const TextureInfo& texture = iter.second;
        textures.emplace(texture.export_name, &texture);
      }
    }
  }

  // Export textures.
  for (const auto& iter : textures) {
    auto texture = std::make_unique<ModelTextureAssetDefT>();
    ExportTexture(*iter.second, texture.get());
    model_def.textures.emplace_back(std::move(texture));
  }

  LogResults(model_def, log);

  return BuildFlatbuffer(model_def);
}

}  // namespace redux::tool
