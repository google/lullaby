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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_MODEL_PIPELINE_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_MODEL_PIPELINE_H_

#include <string>
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "tools/model_pipeline/model.h"
#include "tools/model_pipeline/texture_info.h"
#include "tools/model_pipeline/texture_locator.h"

namespace lull {
namespace tool {

// Performs the controlling logic of importing data, processing it with
// additional properties, and exporting it to a LullModel object.
class ModelPipeline {
 public:
  ModelPipeline() {}

  // Function that imports an asset into a Model.
  using ImportFn =
      std::function<Model(const ModelPipelineImportDefT& import_def)>;

  // Registers a specific asset file type (based on its extension) with a
  // function that can be used to import that asset into a Model object.
  void RegisterImporter(ImportFn importer, string_view extension);

  // Registers an "external" texture that can be used as a texture that may
  // be referenced by an imported model.
  void RegisterTexture(const std::string& texture);

  // Sets the path to the model_def.fbs schema file to use for processing the
  // configuration file.
  void SetModelDefSchema(const std::string& filepath);

  // Imports model data using the specified |config|.
  bool Import(const ModelPipelineDef* config);

  // Imports model data from a single source asset.
  bool ImportFile(const std::string& source,
                  const std::vector<VertexAttributeUsage>& attribs);

  // Imports model data from the specified json string config. The contents of
  // the json string should be a ModelPipelineDef object.
  bool ImportUsingConfig(const std::string& json);

  // Returns the LullModel binary object.
  const ByteArray& GetLullModel() const { return lull_model_; }

  // Returns the ModelPipelineDef json string for the imported LullModel.
  const std::string& GetConfig() const { return config_; }

 private:
  bool Validate();
  bool Build();
  void GatherTextures();
  void SetUsage(const std::string& name, Model::Usage usage);

  ByteArray lull_model_;
  std::string schema_;
  std::string config_;
  ModelPipelineDefT settings_;
  TextureLocator locator_;
  std::unordered_map<std::string, ImportFn> importers_;
  std::unordered_map<std::string, Model> imported_models_;
  std::unordered_map<std::string, TextureInfo> imported_textures_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_MODEL_PIPELINE_H_
