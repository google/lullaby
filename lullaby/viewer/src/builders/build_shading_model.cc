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

#include "lullaby/viewer/src/builders/build_shading_model.h"

#include "ion/base/logging.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/shader_pipeline/build_shader.h"
#include "lullaby/viewer/src/file_manager.h"

namespace lull {
namespace tool {

bool BuildShadingModel(Registry* registry, const std::string& target,
                       const std::string& out_dir) {
  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);

  // Create and populate build parms.
  lull::tool::ShaderBuildParams shader_build_params;
  std::vector<string_view> vertex_sources, fragment_sources;

  if (name == "pbr") {
    vertex_sources.push_back("vertex_skinning.jsonnet");
    vertex_sources.push_back("vertex_position.jsonnet");
    vertex_sources.push_back("normal_vertex.jsonnet");
    vertex_sources.push_back("vertex_texture.jsonnet");
    vertex_sources.push_back("vertex_color.jsonnet");
    vertex_sources.push_back("view_direction_vertex.jsonnet");
    vertex_sources.push_back("tangent_bitangent_normal_vertex.jsonnet");

    fragment_sources.push_back("base_color_fragment.jsonnet");
    fragment_sources.push_back("occlusion_roughness_metallic_fragment.jsonnet");
    fragment_sources.push_back("normal_fragment.jsonnet");
    fragment_sources.push_back(
        "lullshaders/pbr_indirect_light_fragment.jsonnet");
    fragment_sources.push_back("pbr_fragment.jsonnet");
    fragment_sources.push_back("lullshaders/emissive_fragment.jsonnet");
    fragment_sources.push_back("lullshaders/apply_gamma_fragment.jsonnet");
  } else if (name == "unlit") {
    vertex_sources.push_back("vertex_position_multiview.jsonnet");
    vertex_sources.push_back("vertex_position.jsonnet");
    vertex_sources.push_back("vertex_color.jsonnet");
    vertex_sources.push_back("vertex_texture.jsonnet");

    fragment_sources.push_back("fragment_white.jsonnet");
    fragment_sources.push_back("fragment_color.jsonnet");
    fragment_sources.push_back("fragment_texture.jsonnet");
    fragment_sources.push_back("fragment_uniform_color.jsonnet");
  } else if (name == "phong") {
    vertex_sources.push_back("vertex_position_multiview.jsonnet");
    vertex_sources.push_back("vertex_position.jsonnet");
    vertex_sources.push_back("vertex_color.jsonnet");
    vertex_sources.push_back("vertex_texture.jsonnet");
    vertex_sources.push_back("vertex_normal.jsonnet");

    fragment_sources.push_back("fragment_white.jsonnet");
    fragment_sources.push_back("fragment_color.jsonnet");
    fragment_sources.push_back("fragment_texture.jsonnet");
    fragment_sources.push_back("fragment_uniform_color.jsonnet");
    fragment_sources.push_back("fragment_phong.jsonnet");
  } else {
    LOG(ERROR) << "Viewer lullshader builder: Unknown lullshader: " << target;
    return false;
  }

  shader_build_params.vertex_stages = vertex_sources;
  shader_build_params.fragment_stages = fragment_sources;
  shader_build_params.shader_schema_file_path = "shader_def.fbs";

  // Construct a shader json string.
  auto expected_json =
      lull::tool::BuildShaderJsonString(shader_build_params);

  if (!expected_json) {
    LOG(ERROR) << "Failed to build shader json: "
               << expected_json.GetError().GetErrorMessage();
    return false;
  }

  std::string shader_json = *expected_json;

  // Construct flatbuffer binary.
  flatbuffers::DetachedBuffer fb =
      lull::tool::BuildFlatBufferFromShaderJsonString(shader_json,
                                                      shader_build_params);
  if (!fb.data()) {
    LOG(ERROR) << "Failed to create flatbuffer.";
    return false;
  }

  // Write flatbuffer binary to output file.
  std::string out_path = out_dir + name + ".lullshader";
  if (!lull::tool::SaveFile(fb.data(), fb.size(), out_path.c_str(),
                            /* binary = */ true)) {
    LOG(ERROR) << "Failed to save flatbuffer binary to file: " << out_path;
    return false;
  }
  return true;
}

}  // namespace tool
}  // namespace lull
