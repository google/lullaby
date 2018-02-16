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

#include "flatbuffers/util.h"
#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "tools/common/file_utils.h"
#include "tools/model_pipeline/model_pipeline.h"

namespace lull {
namespace tool {

Model ImportFbx(const ModelPipelineImportDefT& import_def);
Model ImportAsset(const ModelPipelineImportDefT& import_def);

int Run(int argc, const char** argv) {
  ArgParser args;
  args.AddArg("input")
      .SetNumArgs(1)
      .SetDescription("FBX file to process.");
  args.AddArg("config-json")
      .SetNumArgs(1)
      .SetDescription("config file to process.");
  args.AddArg("output")
      .SetRequired()
      .SetNumArgs(1)
      .SetDescription("Mesh file to save.");
  args.AddArg("outdir")
      .SetRequired()
      .SetNumArgs(1)
      .SetDescription("Location (path) to save file.");
  args.AddArg("textures")
      .SetNumArgs(1)
      .SetDescription("List of semi-colon delimited textures.");
  args.AddArg("attrib")
      .SetNumArgs(1)
      .SetDescription("A list of characters describing the vertex attributes to"
                      "be exported. \n"
                      "p - 3D position coordinates\n"
                      "q - Quaternion orientation\n"
                      "n - 3D normal\n"
                      "t - 3D tangent + handedness\n"
                      "c - 32-bit RGBA color\n"
                      "u - 2D texture coordinates (uvs)\n"
                      "b - Bone influences (indices and weights)");
  args.AddArg("schema")
      .SetNumArgs(1)
      .SetDescription("Path to the model_pipeline_def.fbs schema file.");
  args.AddArg("ext")
      .SetNumArgs(1)
      .SetDescription("Extension to use for the output file.");
  args.AddArg("save-config")
      .SetDescription("Export a config file.");

  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      LOG(ERROR) << err;
    }
    LOG(ERROR) << args.GetUsage();
    return -1;
  }

  ModelPipeline pipeline;
  pipeline.RegisterImporter(ImportFbx, ".fbx");
  pipeline.RegisterImporter(ImportAsset, ".dae");
  pipeline.RegisterImporter(ImportAsset, ".gltf");
  pipeline.RegisterImporter(ImportAsset, ".obj");

  std::string temp = args.GetString("textures").to_string();
  while (true) {
    auto pos = temp.find(';');
    if (pos == std::string::npos) {
      break;
    }
    pipeline.RegisterTexture(temp.substr(0, pos));
    temp = temp.substr(pos + 1);
  }
  if (args.IsSet("schema")) {
    pipeline.SetModelDefSchema(args.GetString("schema").to_string());
  }

  std::vector<VertexAttributeUsage> attribs;
  if (args.IsSet("attrib")) {
    auto attrib = args.GetString("attrib").to_string();
    for (auto c : attrib) {
      switch (c) {
        case 'p':
          attribs.push_back(VertexAttributeUsage_Position);
          break;
        case 'q':
          attribs.push_back(VertexAttributeUsage_Orientation);
          break;
        case 'n':
          attribs.push_back(VertexAttributeUsage_Normal);
          break;
        case 't':
          attribs.push_back(VertexAttributeUsage_Tangent);
          break;
        case 'c':
          attribs.push_back(VertexAttributeUsage_Color);
          break;
        case 'u':
          attribs.push_back(VertexAttributeUsage_TexCoord);
          break;
        case 'b':
          attribs.push_back(VertexAttributeUsage_BoneIndices);
          attribs.push_back(VertexAttributeUsage_BoneWeights);
          break;
        default:
          LOG(ERROR) << "Unknown attribute type: " << c;
          break;
      }
    }
  }

  if (args.IsSet("config-json")) {
    const string_view json = args.GetString("config-json");
    if (!pipeline.ImportUsingConfig(json.to_string())) {
      return -1;
    }
  } else {
    const string_view source = args.GetString("input");
    if (!pipeline.ImportFile(source.to_string(), attribs)) {
      return -1;
    }
  }

  // Create the output folder if necessary.
  const std::string out_dir = args.GetString("outdir").to_string();
  if (!CreateFolder(out_dir.c_str())) {
    LOG(ERROR) << "Could not create directory: " << out_dir;
    return -1;
  }

  std::string ext = "lullmodel";
  if (args.IsSet("ext")) {
    ext = args.GetString("ext").to_string();
  }

  const std::string mesh_name = RemoveDirectoryAndExtensionFromFilename(
      args.GetString("output").to_string());

  const ByteArray& flatbuffer = pipeline.GetLullModel();
  const std::string outfile = out_dir + "/" + mesh_name + "." + ext;

  if (!SaveFile(flatbuffer.data(), flatbuffer.size(), outfile.c_str(), true)) {
    LOG(ERROR) << "Unable to save model.";
    return -1;
  }

  if (args.IsSet("save-config")) {
    const std::string& config = pipeline.GetConfig();
    const std::string outfile = out_dir + "/" + mesh_name + ".jsonnet";
    if (!SaveFile(config.c_str(), config.size(), outfile.c_str(), false)) {
      LOG(ERROR) << "Unable to save renderable.";
      return -1;
    }
  }

  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
