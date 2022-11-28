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

#include "redux/tools/model_pipeline/model_pipeline.h"

#include "redux/modules/base/filepath.h"
#include "redux/tools/model_pipeline/export.h"

namespace redux::tool {

static std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return tolower(c); });
  return result;
}

void ModelPipeline::RegisterImporter(ImportFn importer,
                                     std::string_view extension) {
  importers_[ToLower(extension)] = std::move(importer);
}

void ModelPipeline::RegisterTexture(const std::string& texture) {
  locator_.RegisterTexture(texture);
}

void ModelPipeline::RegisterDirectory(const std::string& directory) {
  locator_.RegisterDirectory(directory);
}

ModelPtr ModelPipeline::GetImportedModel(std::string_view name) const {
  auto iter = imported_models_.find(std::string(name));
  CHECK(iter != imported_models_.end());
  ModelPtr model = iter->second;
  CHECK(model);
  return model;
}

ModelPipeline::ImportFn ModelPipeline::GetImporter(std::string_view uri) const {
  const std::string ext = ToLower(GetExtension(uri));
  auto iter = importers_.find(ext);
  return iter != importers_.end() ? iter->second : nullptr;
}

DataContainer ModelPipeline::Build(const Config& config) {
  CHECK(config.sources());
  for (const auto src : *config.sources()) {
    CHECK(src && src->uri()) << "Source requires uri.";
    std::string_view uri = src->uri()->c_str();

    ImportFn importer = GetImporter(uri);
    CHECK(importer) << "Unable to find importer for: " << uri;

    ModelPtr model = importer(*src);
    CHECK(model) << "Unable to import model: " << uri;

    model->Finish(src, [&](std::string_view uri) {
      return locator_.FindTexture(std::string(uri));
    });

    imported_models_[std::string(uri)] = model;
  }

  std::vector<ModelPtr> renderables;
  CHECK(config.renderables());
  for (const auto name : *config.renderables()) {
    CHECK(name) << "Renderable must specify a name.";
    renderables.emplace_back(GetImportedModel(name->c_str()));
  }

  ModelPtr skeleton;
  if (config.skeleton()) {
    skeleton = GetImportedModel(config.skeleton()->c_str());
  }

  ModelPtr collidable;
  if (config.collidable()) {
    collidable = GetImportedModel(config.collidable()->c_str());
  }

  return ExportModel(renderables, skeleton, collidable, log_);
}

}  // namespace redux::tool
