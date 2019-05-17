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

#if !defined(_WINDOWS) && !defined(_WIN32)
#include <unistd.h>
#endif // !defined(_WINDOWS) && !defined(_WIN32)

#include "flatbuffers/util.h"
#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/common/log.h"
#include "lullaby/tools/model_pipeline/export_options.h"
#include "lullaby/tools/model_pipeline/model_pipeline.h"

namespace lull {
namespace tool {

Model ImportFbx(const ModelPipelineImportDefT& import_def);
Model ImportAsset(const ModelPipelineImportDefT& import_def);

int Run(int argc, const char** argv) {
  ArgParser args;
  args.AddArg("input").SetNumArgs(1).SetDescription("Asset file to process.");
  args.AddArg("config-json")
      .SetNumArgs(1)
      .SetDescription("Config file to process.");
  args.AddArg("output")
      .SetRequired()
      .SetNumArgs(1)
      .SetDescription("Mesh file to save.");
  args.AddArg("outdir")
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
  args.AddArg("log")
      .SetDescription("Write a log file to the output directory. The log file"
                      " will be named the same as the output file with the"
                      " extension changed to '.log'.");
  args.AddArg("discrete-textures")
      .SetDescription("Don't embed textures in the lullmodel. The dependent"
                      " textures will be copied to the output directory beside"
                      " the lullmodel.");
  args.AddArg("use-relative-paths")
      .SetDescription(
          "Paths embeded within the lullmodel will use relative paths.");

  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    return -1;
  }

  const std::string output(args.GetString("output"));
  std::string out_dir = GetDirectoryFromFilename(output);
  if (args.IsSet("outdir")) {
    out_dir = std::string(args.GetString("outdir"));
  }
  if (!CreateFolder(out_dir.c_str())) {
    LOG(ERROR) << "Could not create directory: " << out_dir;
    return -1;
  }

  if (args.IsSet("log")) {
    const std::string log_path = RemoveExtensionFromFilename(output) + ".log";
    LogOpen(log_path.c_str());
  }

  const std::string mesh_name = RemoveDirectoryAndExtensionFromFilename(
      output);

  std::string ext = "lullmodel";
  if (args.IsSet("ext")) {
    ext = std::string(args.GetString("ext"));
  }
  const std::string outfile = JoinPath(out_dir, mesh_name + "." + ext);

#if !defined(_WINDOWS) && !defined(_WIN32)
  char buff[1024];
  if (getcwd(buff, 1024)) {
    LogWrite("working directory: %s\n", buff);
  }
#endif
  if (args.IsSet("input")) {
    const std::string input(args.GetString("input"));
    LogWrite("input:             %s\n", input.c_str());
  }
  LogWrite("output:            %s\n\n", outfile.c_str());

  ModelPipeline pipeline;
  pipeline.RegisterImporter(ImportFbx, ".fbx");
  pipeline.RegisterImporter(ImportAsset, ".dae");
  pipeline.RegisterImporter(ImportAsset, ".gltf");
  pipeline.RegisterImporter(ImportAsset, ".obj");

  std::string temp(args.GetString("textures"));
  while (true) {
    auto pos = temp.find(';');
    if (pos == std::string::npos) {
      break;
    }
    pipeline.RegisterTexture(temp.substr(0, pos));
    temp = temp.substr(pos + 1);
  }
  if (args.IsSet("schema")) {
    pipeline.SetModelDefSchema(std::string(args.GetString("schema")));
  }

  std::vector<VertexAttributeUsage> attribs;
  if (args.IsSet("attrib")) {
    std::string attrib(args.GetString("attrib"));
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

  ExportOptions options;
  options.embed_textures = !args.IsSet("discrete-textures");
  options.relative_path = args.IsSet("use-relative-paths");
  if (args.IsSet("config-json")) {
    const string_view json = args.GetString("config-json");
    if (!pipeline.ImportUsingConfig(std::string(json))) {
      return -1;
    }
  } else {
    const std::string source(args.GetString("input"));
    const std::string search_path = GetDirectoryFromFilename(source);
    pipeline.RegisterDirectory(search_path);
    if (!pipeline.ImportFile(source, attribs, options)) {
      return -1;
    }
  }

  const ByteArray& flatbuffer = pipeline.GetLullModel();

  if (!SaveFile(flatbuffer.data(), flatbuffer.size(), outfile.c_str(), true)) {
    LOG(ERROR) << "Unable to save model.";
    return -1;
  }

  // If we are not embedding the textures in the lullmodel, copy the textures
  // into the output directory.
  if (!options.embed_textures) {
    for (const auto& iter : pipeline.GetImportedTextures()) {
      const std::string& src_texture = iter.second.abs_path;
      const std::string dst_texture =
          JoinPath(out_dir, GetBasenameFromFilename(src_texture));
      if (CopyFile(dst_texture.c_str(), src_texture.c_str())) {
        LogWrite("Copied %s to %s\n", src_texture.c_str(), dst_texture.c_str());
      } else {
        LogWrite("Failed to copy %s to %s\n", src_texture.c_str(),
                 dst_texture.c_str());
      }
    }
  }

  if (args.IsSet("save-config")) {
    const std::string& config = pipeline.GetConfig();
    const std::string outfile = JoinPath(out_dir, mesh_name + ".jsonnet");
    if (!SaveFile(config.c_str(), config.size(), outfile.c_str(), false)) {
      LOG(ERROR) << "Unable to save renderable.";
      return -1;
    }
  }

  LogClose();

  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
