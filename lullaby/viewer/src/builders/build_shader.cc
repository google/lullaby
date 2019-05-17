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

#include "lullaby/viewer/src/builders/build_shader.h"

#include "lullaby/viewer/src/file_manager.h"
#include "/shader_pipeline/shader_pipeline.h"
#include "lullaby/util/filename.h"

namespace lull {
namespace tool {

bool BuildShader(Registry* registry, const std::string& target,
                 const std::string& out_dir) {
  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);

  if (GetExtensionFromFilename(target) != ".fplshader") {
    LOG(ERROR) << "Target file must be a .fplshader file: " << target;
    return false;
  }

  fplbase::ShaderPipelineArgs args;
  args.fragment_shader = name + ".glslf";
  args.vertex_shader = name + ".glslv";
  args.output_file = out_dir + name + ".fplshader";

  // We don't have access to the BUILD rules that determine how shaders are
  // assembled, so we apply the same logic here explicitly.  Ideally this
  // information will be embedded into a config file for shaders once the
  // new shader pipeline is working.
  if (name == "texture") {
    args.defines.push_back((char*)"TEX_COORD");
    args.defines.push_back((char*)"UV_BOUNDS");
    args.defines.push_back(nullptr);
  }
  if (name == "skinned_texture") {
    args.fragment_shader = "texture.glslf";
    args.defines.push_back((char*)"TEX_COORD");
    args.defines.push_back((char*)"UV_BOUNDS");
    args.defines.push_back(nullptr);
  }
  if (name == "pbr") {
    args.defines.push_back((char*)"TEX_COORD");
    args.defines.push_back(nullptr);
  }
  
  const int result = fplbase::RunShaderPipeline(args);
  if (result != 0) {
    LOG(ERROR) << "Error building shader: " << target;
    return false;
  }
  return true;
}

}  // namespace tool
}  // namespace lull
