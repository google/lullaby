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
#include "redux/modules/base/logging.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/common/json_utils.h"
#include "redux/tools/common/jsonnet_utils.h"
#include "redux/tools/shader_pipeline/shader_pipeline.h"

ABSL_FLAG(std::string, src, "", "Input shader file.");
ABSL_FLAG(std::string, out, "", "Location of file to be saved.");

namespace redux::tool {
namespace {

struct ShaderList {
  std::vector<ShaderAsset> shaders;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(shaders, ConstHash("shaders"));
  }
};

int RunShaderPipeline() {
  const std::string src = absl::GetFlag(FLAGS_src);
  const std::string jsonnet = LoadFileAsString(src.c_str());
  const std::string json = JsonnetToJson(jsonnet.c_str(), src.c_str());
  const std::string name = std::string(RemoveDirectoryAndExtension(src));
  ShaderList shaders = ReadJson<ShaderList>(json.c_str());
  DataContainer data = BuildShader(name, shaders.shaders);

  const std::string& out_file = absl::GetFlag(FLAGS_out);
  CHECK(SaveFile(data.GetBytes(), data.GetNumBytes(), out_file.c_str(), true))
      << "Failed to save to file: " << out_file;
  return 0;
}

}  // namespace
}  // namespace redux::tool

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  return redux::tool::RunShaderPipeline();
}
