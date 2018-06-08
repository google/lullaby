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

#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/anim_pipeline/anim_pipeline.h"
#include "lullaby/tools/common/file_utils.h"

namespace lull {
namespace tool {

int Run(int argc, const char** argv) {
  ArgParser args;
  args.AddArg("input").SetNumArgs(1).SetDescription("Asset file to process.");
  args.AddArg("output").SetRequired().SetNumArgs(1).SetDescription(
      "Anim file to save.");
  args.AddArg("outdir").SetRequired().SetNumArgs(1).SetDescription(
      "Location (path) to save file.");
  args.AddArg("ext").SetNumArgs(1).SetDescription(
      "Extension to use for the output file.");

  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    return -1;
  }

  AnimPipeline pipeline;

  const string_view source = args.GetString("input");
  if (!pipeline.ImportFile(source.to_string())) {
    return -1;
  }

  // Create the output folder if necessary.
  const std::string out_dir = args.GetString("outdir").to_string();
  if (!CreateFolder(out_dir.c_str())) {
    LOG(ERROR) << "Could not create directory: " << out_dir;
    return -1;
  }

  std::string ext = "lullanim";
  if (args.IsSet("ext")) {
    ext = args.GetString("ext").to_string();
  }

  const std::string anim_name = RemoveDirectoryAndExtensionFromFilename(
      args.GetString("output").to_string());

  const ByteArray& flatbuffer = pipeline.GetLullAnim();
  const std::string outfile = out_dir + "/" + anim_name + "." + ext;

  if (!SaveFile(flatbuffer.data(), flatbuffer.size(), outfile.c_str(), true)) {
    LOG(ERROR) << "Unable to save animation.";
    return -1;
  }

  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
