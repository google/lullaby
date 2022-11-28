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

#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "redux/modules/base/filepath.h"
#include "redux/tools/common/axis_system.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/common/flatbuffer_utils.h"
#include "redux/tools/common/log_utils.h"
#include "redux/tools/model_pipeline/model.h"
#include "redux/tools/model_pipeline/model_pipeline.h"

ABSL_FLAG(std::string, input, "", "Input asset.");
ABSL_FLAG(std::string, config, "", "Input configuration file.");
ABSL_FLAG(std::string, output, "", "Exported rxmodel file.");
ABSL_FLAG(std::string, logfile, "", "Log file for export information.");

ABSL_FLAG(std::string, attribs, "",
          "List of attributes to export.");
ABSL_FLAG(std::vector<std::string>, textures, {},
          "Comma-separated list of external textures.");
ABSL_FLAG(std::vector<std::string>, meshes, {},
          "Comma-separated list of meshes from the asset to export.");
ABSL_FLAG(bool, merge_materials, true,
          "Merge materials when possible.");
ABSL_FLAG(float, cm_per_unit, 0.0f,
          "Defines the unit the pipeline expects positions to be in compared "
          "to centimeters (the standard unit for FBX). For example, 100.0 "
          "would be in contexts where world units are measured in meters, and "
          "2.54 would be for inches. Keep at 0 to leave asset units as-is.");
ABSL_FLAG(std::string, axis, "",
          "Specified which axes are up, front, and left.");
ABSL_FLAG(float, scale_multiplier, 1.0f,
          "Overall scale multiplier applied to the entire animation.");

namespace redux::tool {

ModelPtr ImportFbx(const ModelConfig& config);
ModelPtr ImportAsset(const ModelConfig& config);

std::vector<VertexUsage> AttributesFromString(std::string_view str) {
  std::vector<VertexUsage> attribs;
  for (auto c : str) {
    switch (c) {
      case 'p':
        attribs.push_back(VertexUsage::Position);
        break;
      case 'q':
        attribs.push_back(VertexUsage::Orientation);
        break;
      case 'n':
        attribs.push_back(VertexUsage::Normal);
        break;
      case 't':
        attribs.push_back(VertexUsage::Tangent);
        break;
      case 'c':
        attribs.push_back(VertexUsage::Color0);
        break;
      case 'u':
        attribs.push_back(VertexUsage::TexCoord0);
        break;
      case 'b':
        attribs.push_back(VertexUsage::BoneIndices);
        attribs.push_back(VertexUsage::BoneWeights);
        break;
      default:
        LOG(ERROR) << "Unknown attribute type: " << c;
        break;
    }
  }
  return attribs;
}

static int RunModelPipeline() {
  const std::string input = absl::GetFlag(FLAGS_input);
  const std::string config = absl::GetFlag(FLAGS_config);
  CHECK(!input.empty() || !config.empty())
      << "Must specify either 'config' or 'input' arguments.";
  CHECK(input.empty() || config.empty())
      << "Cannot specify both 'config' and 'input' arguments.";

  const std::string output = absl::GetFlag(FLAGS_output);
  CHECK(!output.empty()) << "Must specify output file.";

  LoggerOptions log_opts;
  log_opts.logfile = absl::GetFlag(FLAGS_logfile);
  Logger log(log_opts);

  ModelPipeline pipeline(log);
  pipeline.RegisterImporter(ImportFbx, ".fbx");
  pipeline.RegisterImporter(ImportAsset, ".dae");
  pipeline.RegisterImporter(ImportAsset, ".obj");
  pipeline.RegisterImporter(ImportAsset, ".gltf");
  pipeline.RegisterImporter(ImportAsset, ".glb");
  for (const std::string& texture : absl::GetFlag(FLAGS_textures)) {
    pipeline.RegisterTexture(texture);
  }

  pipeline.RegisterDirectory(std::string(GetDirectory(input)));

  DataContainer data;
  if (!input.empty()) {
    auto model_config = std::make_unique<ModelConfigT>();
    model_config->uri = input;
    model_config->cm_per_unit = absl::GetFlag(FLAGS_cm_per_unit);
    model_config->axis_system = ReadAxisSystem(absl::GetFlag(FLAGS_axis));
    model_config->scale = absl::GetFlag(FLAGS_scale_multiplier);
    model_config->merge_materials = absl::GetFlag(FLAGS_merge_materials);
    model_config->flip_texture_coordinates = true;
    model_config->attributes =
        AttributesFromString(absl::GetFlag(FLAGS_attribs));
    for (const std::string& mesh : absl::GetFlag(FLAGS_textures)) {
      model_config->target_meshes.push_back(mesh);
    }

    ConfigT config;
    config.sources.push_back(std::move(model_config));
    config.renderables.push_back(input);
    config.skeleton = input;
    config.collidable = input;

    auto fb = BuildFlatbuffer(config);
    data = pipeline.Build(*flatbuffers::GetRoot<Config>(fb.GetBytes()));
  }

  CHECK(SaveFile(data.GetBytes(), data.GetNumBytes(), output.c_str(), true))
      << "Failed to save to file: " << output;
  return 0;
}

}  // namespace redux::tool

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  return redux::tool::RunModelPipeline();
}
