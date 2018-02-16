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

#include "tools/model_pipeline/model_pipeline.h"

#include "flatbuffers/idl.h"
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/flatbuffer_writer.h"
#include "lullaby/util/inward_buffer.h"
#include "tools/common/file_utils.h"
#include "tools/model_pipeline/export.h"
#include "tools/model_pipeline/model.h"

namespace lull {
namespace tool {

static void PrepareParser(flatbuffers::Parser* parser,
                          const std::string& schema) {
  std::string text;
  if (!flatbuffers::LoadFile(schema.c_str(), false, &text)) {
    LOG(FATAL) << "Could not open model_pipeline_def.fbs file";
  }
  parser->Parse(text.c_str());
  if (!parser->SetRootType("lull.ModelPipelineDef")) {
     LOG(FATAL) << "Failed to resolve root type lull.ModelPipelineDef";
  }
}

static ByteArray ToFlatbuffer(ModelPipelineDefT* def) {
  InwardBuffer buffer(4096);
  WriteFlatbuffer(def, &buffer);
  const size_t length = buffer.BackSize();
  const uint8_t* data = static_cast<const uint8_t*>(buffer.BackAt(length));
  return ByteArray(data, data + length);
}

static std::string ToString(ModelPipelineDefT* def,
                          const std::string& schema) {
  InwardBuffer buffer(4096);
  WriteFlatbuffer(def, &buffer);

  flatbuffers::Parser parser;
  PrepareParser(&parser, schema);

  std::string out;
  flatbuffers::GenerateText(parser, buffer.BackAt(buffer.BackSize()), &out);
  return out;
}

static flatbuffers::DetachedBuffer FromString(const std::string& json,
                          const std::string& schema) {
  flatbuffers::Parser parser;
  PrepareParser(&parser, schema);
  parser.Parse(json.c_str());
  return parser.builder_.ReleaseBufferPointer();
}

static void ApplyDef(TextureInfo* texture, const TextureDef* def) {
  if (flatbuffers::IsFieldPresent(def, TextureDef::VT_FILE)) {
    texture->source = def->file()->str();
  }
  if (flatbuffers::IsFieldPresent(def, TextureDef::VT_WRAP_S)) {
    texture->wrap_s = def->wrap_s();
  }
  if (flatbuffers::IsFieldPresent(def, TextureDef::VT_WRAP_T)) {
    texture->wrap_t = def->wrap_t();
  }
  if (flatbuffers::IsFieldPresent(def, TextureDef::VT_PREMULTIPLY_ALPHA)) {
    texture->premultiply_alpha = def->premultiply_alpha();
  }
  if (flatbuffers::IsFieldPresent(def, TextureDef::VT_GENERATE_MIPMAPS)) {
    texture->generate_mipmaps = def->generate_mipmaps();
  }
}

static void ApplyDef(Material* material, const MaterialTextureDef* def) {
  if (!def->name()) {
    return;
  }
  const std::string name = def->name()->str();
  if (name.empty()) {
    return;
  }

  if (!flatbuffers::IsFieldPresent(def, MaterialTextureDef::VT_USAGE)) {
    return;
  }

  for (auto iter = material->textures.begin(); iter != material->textures.end();
       ++iter) {
    if (iter->second.usage == def->usage()) {
      TextureInfo info = iter->second;
      material->textures.erase(iter);
      material->textures[name] = info;
      break;
    }
  }
  material->textures[name].usage = def->usage();
}

static void ApplyDef(Material* material, const MaterialDef* def) {
  if (def->properties() && def->properties()->values()) {
    const auto* properties = def->properties()->values();
    for (const KeyVariantPairDef* pair : *properties) {
      if (!pair->key()) {
        continue;
      }
      const std::string key = pair->key()->str();

      Variant var;
      if (VariantFromFbVariant(pair->value_type(), pair->value(), &var)) {
        material->properties.emplace(key, std::move(var));
      } else {
        material->properties.erase(key);
      }
    }
  }
  if (def->textures()) {
    for (const auto texture : *def->textures()) {
      ApplyDef(material, texture);
    }
  }
}

static void ApplyDef(Model* model, const ModelPipelineRenderableDef* def) {
  if (def->materials()) {
    for (const MaterialDef* material_def : *def->materials()) {
      if (!material_def->name()) {
        continue;
      }

      const std::string name = material_def->name()->str();
      Material* material = model->GetMutableMaterial(name);
      if (material) {
        ApplyDef(material, material_def);
      }
    }
  }
  if (def->attributes()) {
    int uv_count = 0;
    int color_count = 0;
    Vertex::Attrib requested = 0;
    for (const int usage : *def->attributes()) {
      switch (usage) {
        case VertexAttributeUsage_Position:
          requested = SetBit(requested, Vertex::kAttribBit_Position);
          break;
        case VertexAttributeUsage_Color:
          requested = SetBit(requested, Vertex::kAttribBit_Color0 + color_count);
          ++color_count;
          break;
        case VertexAttributeUsage_TexCoord:
          requested = SetBit(requested, Vertex::kAttribBit_Uv0 + uv_count);
          ++uv_count;
          break;
        case VertexAttributeUsage_Normal:
          requested = SetBit(requested, Vertex::kAttribBit_Normal);
          break;
        case VertexAttributeUsage_Tangent:
          requested = SetBit(requested, Vertex::kAttribBit_Tangent);
          break;
        case VertexAttributeUsage_Orientation:
          requested = SetBit(requested, Vertex::kAttribBit_Orientation);
          break;
        case VertexAttributeUsage_BoneIndices:
          requested = SetBit(requested, Vertex::kAttribBit_Influences);
          break;
        case VertexAttributeUsage_BoneWeights:
          requested = SetBit(requested, Vertex::kAttribBit_Influences);
          break;
        default:
          LOG(ERROR) << "Unknown vertex attribute usage: " << usage;
          break;
      }
    }

    const Vertex::Attrib available = model->GetAttributes();
    model->DisableAttribute(Vertex::kAttribAllBits);
    model->EnableAttribute(available & requested);
  }
}

void ModelPipeline::RegisterImporter(ImportFn importer, string_view extension) {
  importers_[extension.to_string()] = std::move(importer);
}

void ModelPipeline::RegisterTexture(const std::string& texture) {
  locator_.RegisterTexture(texture);
}

void ModelPipeline::SetModelDefSchema(const std::string& filepath) {
  schema_ = filepath;
}

bool ModelPipeline::ImportFile(
    const std::string& source,
    const std::vector<VertexAttributeUsage>& attribs) {
  ModelPipelineDefT config;

  ModelPipelineImportDefT import_def;
  import_def.file = source;
  import_def.name = RemoveDirectoryAndExtensionFromFilename(source);

  config.sources.emplace_back(import_def);
  config.renderables.emplace_back();
  config.renderables.back().source = import_def.name;
  config.renderables.back().attributes = attribs;
  config.collidable.source = import_def.name;
  config.skeleton.source = import_def.name;

  const ByteArray buffer = ToFlatbuffer(&config);
  return Import(flatbuffers::GetRoot<ModelPipelineDef>(buffer.data()));
}

bool ModelPipeline::ImportUsingConfig(const std::string& json) {
  flatbuffers::DetachedBuffer buffer = FromString(json, schema_);
  return Import(flatbuffers::GetRoot<ModelPipelineDef>(buffer.data()));
}

bool ModelPipeline::Import(const ModelPipelineDef* config) {
  if (config->sources()) {
    for (const auto* source : *config->sources()) {
      ModelPipelineImportDefT import;
      ReadFlatbuffer(&import, source);

      const std::string extension = GetExtensionFromFilename(import.file);
      auto iter = importers_.find(extension);
      if (iter != importers_.end()) {
        imported_models_.emplace(import.name, iter->second(import));
      }
    }
  }

  if (config->renderables()) {
    for (const auto* renderable : *config->renderables()) {
      SetUsage(renderable->source()->str(), Model::kForRendering);
    }
  }
  if (config->collidable()) {
    SetUsage(config->collidable()->source()->str(), Model::kForCollision);
  }
  if (config->skeleton()) {
    SetUsage(config->skeleton()->source()->str(), Model::kForSkeleton);
  }

  if (config->renderables()) {
    for (const auto* renderable : *config->renderables()) {
      auto iter = imported_models_.find(renderable->source()->str());
      if (iter != imported_models_.end()) {
        ApplyDef(&iter->second, renderable);
      }
    }
  }

  GatherTextures();

  if (config->textures()) {
    for (const auto* texture : *config->textures()) {
      if (texture->name()) {
        auto iter = imported_textures_.find(texture->name()->str());
        if (iter != imported_textures_.end()) {
          ApplyDef(&iter->second, texture);
        }
      }
    }
  }

  if (!Validate()) {
    return false;
  }
  return Build();
}

bool ModelPipeline::Validate() {
  for (const auto& iter : imported_models_) {
    const Model& model = iter.second;
    if (!model.IsValid()) {
      LOG(ERROR) << "Unable to import model: " << model.GetImportDef().name;
      return false;
    }
  }
  for (const auto& iter : imported_textures_) {
    if (!FileExists(iter.second.source.c_str())) {
      LOG(ERROR) << "Unable to find texture: " << iter.second.source;
      return false;
    }
  }
  return true;
}

bool ModelPipeline::Build() {
  ModelPipelineDefT data;
  lull_model_ = ExportModel(imported_models_, imported_textures_, &data);
  config_ = ToString(&data, schema_);
  return true;
}

void ModelPipeline::SetUsage(const std::string& name, Model::Usage usage) {
  auto iter = imported_models_.find(name);
  if (iter != imported_models_.end()) {
    iter->second.SetUsage(usage);
  } else {
    LOG(ERROR) << "No such asset: " << name;
  }
}

void ModelPipeline::GatherTextures() {
  for (const auto& iter : imported_models_) {
    const Model& model = iter.second;
    if (!model.CheckUsage(Model::kForRendering)) {
      continue;
    }

    for (const Drawable& drawable : model.GetDrawables()) {
      for (const auto& texture : drawable.material.textures) {
        const std::string name = locator_.FindTexture(texture.first);
        if (!name.empty()) {
          texture.second.source = name;
          imported_textures_.emplace(texture.first, texture.second);
        }
      }
    }
  }
}

}  // namespace tool
}  // namespace lull
