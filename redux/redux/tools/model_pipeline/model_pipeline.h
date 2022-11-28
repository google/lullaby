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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_MODEL_PIPELINE_H_
#define REDUX_TOOLS_MODEL_PIPELINE_MODEL_PIPELINE_H_

#include <memory>
#include <string>

#include "absl/types/span.h"
#include "redux/tools/common/log_utils.h"
#include "redux/tools/model_pipeline/model.h"
#include "redux/tools/model_pipeline/texture_locator.h"

namespace redux::tool {

// Performs the controlling logic of importing data, processing it with
// additional properties, and exporting it to a ReduxModel object.
class ModelPipeline {
 public:
  explicit ModelPipeline(Logger& log) : log_(log) {}

  // Function that imports an asset into a Model.
  using ImportFn = std::function<ModelPtr(const ModelConfig&)>;

  // Registers a specific asset file type (based on its extension) with a
  // function that can be used to import that asset into a Model object.
  void RegisterImporter(ImportFn importer, std::string_view extension);

  // Registers an "external" texture that can be used as a texture that may
  // be referenced by an imported model.
  void RegisterTexture(const std::string& texture);

  // References a directory where we can look for content, e.g. textures.
  void RegisterDirectory(const std::string& directory);

  // Imports model data using the specified |config|.
  DataContainer Build(const Config& config);

 private:
  ImportFn GetImporter(std::string_view uri) const;
  ModelPtr GetImportedModel(std::string_view name) const;

  absl::flat_hash_map<std::string, ImportFn> importers_;
  absl::flat_hash_map<std::string, ModelPtr> imported_models_;
  TextureLocator locator_;
  Logger& log_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_MODEL_PIPELINE_H_
