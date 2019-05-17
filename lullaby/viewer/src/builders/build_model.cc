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

#include "lullaby/viewer/src/builders/build_model.h"

#include "lullaby/viewer/src/file_manager.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/model_pipeline/model_pipeline.h"

namespace lull {
namespace tool {

Model ImportFbx(const ModelPipelineImportDefT& import_def);
Model ImportAsset(const ModelPipelineImportDefT& import_def);

static std::string FindSourceFile(Registry* registry, const std::string& name) {
  const char* extensions[] = {
      ".fbx",
      ".obj",
      ".gltf",
      ".dae",
  };

  auto* file_manager = registry->Get<FileManager>();
  for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); ++i) {
    const std::string source = name + extensions[i];
    if (file_manager->ExistsWithExtension(source)) {
      return file_manager->GetRealPath(source);
    }
  }
  return "";
}

static void RegisterTextures(Registry* registry, ModelPipeline* pipeline) {
  const char* extensions[] = {
      ".png",
      ".webp",
  };

  auto* file_manager = registry->Get<FileManager>();
  for (int i = 0; i < sizeof(extensions) / sizeof(extensions[0]); ++i) {
    auto files = file_manager->FindAllFiles(extensions[i]);
    for (auto& texture : files) {
      pipeline->RegisterTexture(file_manager->GetRealPath(texture));
    }
  }
}

bool BuildModel(Registry* registry, const std::string& target,
                const std::string& out_dir) {
  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);

  if (GetExtensionFromFilename(target) != ".lullmodel") {
    LOG(ERROR) << "Target file must be a .lullmodel file: " << target;
    return false;
  }

  const std::string source = FindSourceFile(registry, name);
  if (source.empty()) {
    LOG(ERROR) << "Could not find source file for model: " << target;
    return false;
  }

  ModelPipeline pipeline;
  pipeline.SetModelDefSchema("model_pipeline_def.fbs");
  pipeline.RegisterImporter(ImportFbx, ".fbx");
  pipeline.RegisterImporter(ImportAsset, ".dae");
  pipeline.RegisterImporter(ImportAsset, ".gltf");
  pipeline.RegisterImporter(ImportAsset, ".obj");
  RegisterTextures(registry, &pipeline);
  std::vector<VertexAttributeUsage> attribs;
  if (!pipeline.ImportFile(source, attribs)) {
    LOG(ERROR) << "Unable to import file: " << source;
    return false;
  }

  const std::string outfile = out_dir + name + ".lullmodel";

  const ByteArray& flatbuffer = pipeline.GetLullModel();
  if (!SaveFile(flatbuffer.data(), flatbuffer.size(), outfile.c_str(), true)) {
    return false;
  }
  return true;
}

}  // namespace tool
}  // namespace lull
