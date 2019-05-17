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

#include "lullaby/tools/model_pipeline/model_pipeline.h"

#include "flatbuffers/idl.h"
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/flatbuffer_writer.h"
#include "lullaby/util/inward_buffer.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/model_pipeline/export.h"
#include "lullaby/tools/model_pipeline/model.h"

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

static std::string ToString(ModelPipelineDefT* def, const std::string& schema) {
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
    std::string file = def->file()->str();
    texture->basename = GetBasenameFromFilename(file);
    texture->abs_path = file;
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

  auto found_matching_usage = false;
  for (auto iter = material->textures.begin(); iter != material->textures.end();
       ++iter) {
    auto& usages = iter->second.usages;
    if (std::find(usages.begin(), usages.end(), def->usage()) != usages.end()) {
      TextureInfo info = std::move(iter->second);
      material->textures.erase(iter);
      material->textures.emplace(name, std::move(info));
      found_matching_usage = true;
      break;
    }
  }

  if (!found_matching_usage) {
    material->textures[name].usages.push_back(def->usage());
  }
}

static void ApplyDef(Material* material, const ModelPipelineMaterialDef* def) {
  if (def->name_override() && def->name_override()->Length()) {
    material->name = def->name_override()->str();
  }
  if (const MaterialDef* material_def = def->material()) {
    if (material_def->properties() && material_def->properties()->values()) {
      const auto* properties = material_def->properties()->values();
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
    if (material_def->textures()) {
      for (const auto texture : *material_def->textures()) {
        ApplyDef(material, texture);
      }
    }
  }
}

static void ApplyDef(Model* model, const ModelPipelineRenderableDef* def) {
  if (def->materials()) {
    for (const ModelPipelineMaterialDef* pipeline_material_def :
         *def->materials()) {
      if (const lull::MaterialDef* material_def =
              pipeline_material_def->material()) {
        if (!material_def->name()) {
          continue;
        }

        const std::string name = material_def->name()->str();
        Material* material = model->GetMutableMaterial(name);
        if (material) {
          ApplyDef(material, pipeline_material_def);
        }
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
          requested =
              SetBit(requested, Vertex::kAttribBit_Color0 << color_count);
          ++color_count;
          break;
        case VertexAttributeUsage_TexCoord:
          requested = SetBit(requested, Vertex::kAttribBit_Uv0 << uv_count);
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
  importers_[std::string(extension)] = std::move(importer);
}

void ModelPipeline::RegisterTexture(const std::string& texture) {
  locator_.RegisterTexture(texture);
}

void ModelPipeline::RegisterTextureWithData(const std::string& texture,
                                            const TextureDataPtr& data) {
  imported_textures_with_data_[texture] = data;
}

void ModelPipeline::RegisterDirectory(const std::string& directory) {
  locator_.RegisterDirectory(directory);
}

void ModelPipeline::SetModelDefSchema(const std::string& filepath) {
  schema_ = filepath;
}

bool ModelPipeline::ImportFile(const std::string& source,
                               const std::vector<VertexAttributeUsage>& attribs,
                               const ExportOptions options) {
  ModelPipelineDefT config;

  ModelPipelineImportDefT import_def;
  import_def.file = source;
  import_def.name = RemoveDirectoryAndExtensionFromFilename(source);
  import_def.flip_texture_coordinates = true;

  config.sources.emplace_back(import_def);
  config.renderables.emplace_back();
  config.renderables.back().source = import_def.name;
  config.renderables.back().attributes = attribs;
  config.collidable.source = import_def.name;
  config.skeleton.source = import_def.name;

  const ByteArray buffer = ToFlatbuffer(&config);
  return Import(flatbuffers::GetRoot<ModelPipelineDef>(buffer.data()), options);
}

bool ModelPipeline::ImportUsingConfig(const std::string& json) {
  flatbuffers::DetachedBuffer buffer = FromString(json, schema_);
  return Import(flatbuffers::GetRoot<ModelPipelineDef>(buffer.data()));
}

bool ModelPipeline::Import(const ModelPipelineDef* config,
                           const ExportOptions options) {
  if (config->sources()) {
    for (const auto* source : *config->sources()) {
      ModelPipelineImportDefT import;
      ReadFlatbuffer(&import, source);

      std::string extension = GetExtensionFromFilename(import.file);
      // Convert extension to lower case (e.g. .FBX -> .fbx).
      std::transform(extension.begin(), extension.end(), extension.begin(),
                     [](unsigned char c) { return tolower(c); });
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

  if (options.look_for_unlinked_textures) {
    LookForUnlinkedTextures(config);
  }
  GatherTextures(config);

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

  if (!Validate(options)) {
    return false;
  }
  return Build(options);
}

bool ModelPipeline::Validate(const ExportOptions& options) {
  for (const auto& iter : imported_models_) {
    const Model& model = iter.second;
    if (!model.IsValid()) {
      LOG(ERROR) << "Unable to import model: " << model.GetImportDef().name;
      return false;
    }
  }
  // If we are embedding textures, we need to verify that the textures exist
  // so we can read them for embed.
  if (options.embed_textures) {
    for (const auto& iter : imported_textures_) {
      if (!FileExists(iter.second.abs_path.c_str()) && !iter.second.data) {
        LOG(ERROR) << "Unable to find texture: " << iter.second.abs_path;
        return false;
      }
    }
  }
  return true;
}

bool ModelPipeline::Build(const ExportOptions options) {
  lull_model_ =
      ExportModel(imported_models_, imported_textures_, options, &config_);
  for (const auto& pair : imported_models_) {
    const auto& imported_paths = pair.second.GetImportedPaths();
    opened_file_paths_.insert(opened_file_paths_.end(), imported_paths.begin(),
                              imported_paths.end());
  }

  // Only adds non-empty texture paths. A texture path will be empty if it is
  // embedded.
  for (const auto& pair : imported_textures_) {
    if (!pair.second.abs_path.empty()) {
      opened_file_paths_.insert(opened_file_paths_.end(), pair.second.abs_path);
    }
  }

  return true;
}

std::string ModelPipeline::GetConfig() { return ToString(&config_, schema_); }

void ModelPipeline::SetUsage(const std::string& name, Model::Usage usage) {
  auto iter = imported_models_.find(name);
  if (iter != imported_models_.end()) {
    iter->second.SetUsage(usage);
  } else {
    LOG(ERROR) << "No such asset: " << name;
  }
}

std::string ModelPipeline::TryFindTexturePath(const ModelPipelineDef* config,
                                              const std::string& name_in) {
  // Allow the config to specify texture paths; we fall back to
  // texture locator if the config doesn't specify
  std::string name = LocalizePath(name_in);
  if (config->textures()) {
    for (const auto* texture : *config->textures()) {
      if (texture->name() && texture->name()->str() == name) {
        if (texture->file()) {
          return texture->file()->str();
        }
      }
    }
  }
  return locator_.FindTexture(name);
}

void ModelPipeline::LookForUnlinkedTextures(const ModelPipelineDef* config) {
  for (auto& iter : imported_models_) {
    Model& model = iter.second;
    const auto drawables = model.GetDrawables();
    for (int drawable_index = 0; drawable_index < drawables.size();
         drawable_index++) {
      const Drawable& drawable = drawables[drawable_index];
      if (drawable.material.textures.empty()) {
        // Saving throw for untextured materials: see if the pipeline knows
        // about a texture with the same basename as the material. The
        // extension we add doesn't limit our search space to that file type;
        // we add it because callee expects a relative path to an image file.
        // This only works for the base map.
        const std::string texture_name = drawable.material.name + ".png";
        const std::string texture_path =
            TryFindTexturePath(config, texture_name);
        if (!texture_path.empty()) {
          const std::string basename = GetBasenameFromFilename(texture_path);
          TextureInfo info;
          info.basename = basename;
          info.abs_path = texture_path;
          info.generate_mipmaps = true;
          auto* material = model.GetMutableMaterial(drawable_index);
          material->textures.insert({basename, info});
          material->properties["DiffuseColor"] = mathfu::vec3(1.f, 1.f, 1.f);
        }
      }
    }
  }
}

void ModelPipeline::GatherTextures(const ModelPipelineDef* config) {
  for (const auto& iter : imported_models_) {
    const Model& model = iter.second;
    if (!model.CheckUsage(Model::kForRendering)) {
      continue;
    }

    for (const Drawable& drawable : model.GetDrawables()) {
      for (const auto& texture : drawable.material.textures) {
        auto pair = imported_textures_with_data_.find(
            RemoveExtensionFromFilename(texture.first));
        if (pair != imported_textures_with_data_.end()) {
          texture.second.basename = GetBasenameFromFilename(texture.first);
          texture.second.data = pair->second;
          imported_textures_.emplace(texture.first, texture.second);
          continue;
        }

        // Allows textures with embedded data to be designated as imported.
        if (texture.second.data && !texture.second.data->empty()) {
          texture.second.basename = texture.first;
          imported_textures_.emplace(texture.first, texture.second);
          continue;
        }

        std::string texture_path = TryFindTexturePath(config, texture.first);
        if (!texture_path.empty()) {
          texture.second.basename = GetBasenameFromFilename(texture_path);
          texture.second.abs_path = texture_path;
          imported_textures_.emplace(texture.first, texture.second);
        } else {
          missing_texture_names_.push_back(texture.first);
        }
      }
    }
  }
}

}  // namespace tool
}  // namespace lull
