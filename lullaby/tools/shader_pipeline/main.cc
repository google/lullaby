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

#include <fstream>
#include <iostream>
#include <string>

#include "lullaby/util/arg_parser.h"
#include "lullaby/util/logging.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/shader_pipeline/build_shader.h"

int main(int argc, const char** argv) {
  lull::ArgParser parser;
  parser.AddArg("vertex_sources").SetNumArgs(1).SetRequired();
  parser.AddArg("fragment_sources").SetNumArgs(1).SetRequired();
  parser.AddArg("schema").SetNumArgs(1).SetRequired();
  parser.AddArg("out").SetNumArgs(1).SetRequired();

  // Parse the arguments.
  if (!parser.Parse(argc, argv)) {
    LOG(ERROR) << "Failed to parse args:";
    const auto& errors = parser.GetErrors();
    for (const auto& error : errors) {
      LOG(ERROR) << error;
    }
    return -1;
  }

  // Create and populate build parms.
  lull::tool::ShaderBuildParams shader_build_params;
  shader_build_params.vertex_stages = parser.GetValues("vertex_sources");
  shader_build_params.fragment_stages = parser.GetValues("fragment_sources");
  shader_build_params.shader_schema_file_path = parser.GetString("schema");

  // Construct a shader json string.
  auto expected_json = lull::tool::BuildShaderJsonString(shader_build_params);

  if (!expected_json) {
    LOG(FATAL) << "Failed to build shader json: "
               << expected_json.GetError().GetErrorMessage();
    return -1;
  }

  std::string shader_json = *expected_json;

  // Construct flatbuffer binary.
  flatbuffers::DetachedBuffer fb =
      lull::tool::BuildFlatBufferFromShaderJsonString(shader_json,
                                                      shader_build_params);
  if (!fb.data()) {
    LOG(FATAL) << "Failed to create flatbuffer.";
    return -1;
  }

  // Write flatbuffer binary to output file.
  const std::string out_path(parser.GetString("out"));
  if (!lull::tool::SaveFile(fb.data(), fb.size(), out_path.c_str(),
                            /* binary = */ true)) {
    LOG(FATAL) << "Failed to save flatbuffer binary to file: " << out_path;
    return -1;
  }
  return 0;
}
